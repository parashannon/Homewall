#!/usr/bin/env python3
"""HomeWall single-move calibration (Python 3.9+, standard library only).

Quick start:
  python3 calibrate_homewall.py

Defaults: ./test_cases.json (created if absent), /dev/ttyACM0,
./homewall_ratings.csv, climber Shannon, shuffled order with seed 20261009.
Existing cases are never overwritten. No monitor log is read or required.

Other commands:
  python3 calibrate_homewall.py --write-example test_cases.json
  python3 calibrate_homewall.py test_cases.json --dry-run --log practice.csv
  python3 calibrate_homewall.py test_cases.json --port /dev/ttyACM0 --shuffle

Keep the HTTP serial monitor running; it owns serial settings and reads.
Uses scratch problem slot 98 in RAM. Sends via system printf with a write-only
serial descriptor; never reads, flushes, or configures the serial port.
No acknowledgment checks. Waits 2.5 seconds after commands for firmware parsing.
Ensure wall flip is OFF before starting and visually check displayed moves;
flip state cannot be detected in write-only mode. Avoid other wall commands.
No cloud commands are sent. Avoid changing the wall from another controller.

JSON schema: {"schema_version":1,"cases":[{"id":"T001", "starts":[405],
 "target":504,"feet":[104,105],"rails":[],"instructions":"...","tags":[]}]}
Hold references: row*100+column, or {"hold":405,"hard":true} for hard use.
1-2 start holds, exactly one target, up to 20 total entries. Rows 1-15 are
physical holds. Rails: left, right, heel, kickboard. Each case also requires difficulty_score and matching_levels; these and
score_model are logged but never printed during a session. Pilot predictions
are base-only and omit all foot, rail, gaston and crossover adjustments.
Matching levels use score windows and target rating, not full geometry.
Extra case fields are preserved in the log. Foot-only usage and prescribed hand must be enforced by
the climber; firmware cannot enforce these or detect a gaston. Foot LEDs use
normal firmware color, cyan=start, pink=target. Matching/finishing requirement
must be consistent across sessions. Rate only the move, not getting into it.

Ratings 1-10: your HomeWall difficulty; 11: impossible for you under the stated
conditions. Commands: r repeat display, s skip, q quit. Optional note follows
rating, e.g. '7 awkward left foot'. CSV is appended, flushed and fsynced per
rating/skip. Resume ignores already rated identical cases for the same climber;
skipped cases are offered again. Changed cases are distinct by content hash.
--repeat-rated allows another complete pass. --dry-run never opens serial.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import random
import re
import sys
import subprocess
import time
import uuid
from datetime import datetime, timezone

RAILS = {'left':1602, 'right':1610, 'heel':1601, 'kickboard':1606}
# Snapshot of physical BASIC-use ratings from October 9 homewall_var.h.
BASIC_CODES = {102: 7, 104: 3, 105: 4, 106: 37, 107: 4, 108: 3, 110: 7, 201: 34, 202: 16, 203: 7, 204: 1, 205: 4, 206: 7, 207: 4, 208: 1, 209: 7, 210: 46, 211: 34, 301: 6, 302: 34, 303: 44, 304: 47, 305: 37, 306: 2, 307: 37, 308: 17, 309: 14, 310: 34, 311: 6, 401: 5, 402: 6, 403: 37, 404: 2, 405: 6, 406: 6, 407: 6, 408: 2, 409: 37, 410: 6, 411: 5, 501: 16, 502: 6, 503: 16, 504: 7, 505: 44, 506: 7, 507: 14, 508: 7, 509: 46, 510: 6, 511: 46, 601: 6, 602: 13, 603: 106, 604: 6, 605: 5, 606: 36, 607: 5, 608: 6, 609: 106, 610: 43, 611: 6, 701: 1, 702: 8, 703: 44, 704: 3, 705: 7, 706: 3, 707: 7, 708: 3, 709: 14, 710: 8, 711: 1, 801: 7, 802: 6, 803: 7, 804: 3, 805: 48, 806: 6, 807: 18, 808: 3, 809: 7, 810: 6, 811: 7, 901: 7, 902: 7, 903: 13, 904: 6, 905: 36, 906: 37, 907: 36, 908: 6, 909: 43, 910: 7, 911: 7, 1001: 6, 1002: 7, 1003: 27, 1004: 5, 1005: 8, 1006: 102, 1007: 8, 1008: 5, 1009: 57, 1010: 7, 1011: 6, 1101: 1, 1102: 6, 1103: 3, 1104: 7, 1105: 27, 1106: 6, 1107: 57, 1108: 7, 1109: 3, 1110: 6, 1111: 1, 1201: 34, 1202: 2, 1203: 102, 1204: 7, 1205: 44, 1206: 8, 1207: 14, 1208: 7, 1209: 102, 1210: 2, 1211: 34, 1301: 7, 1302: 7, 1303: 5, 1304: 27, 1305: 7, 1306: 101, 1307: 7, 1308: 57, 1309: 5, 1310: 7, 1311: 7, 1401: 6, 1402: 15, 1403: 58, 1404: 8, 1405: 17, 1406: 4, 1407: 47, 1408: 8, 1409: 28, 1410: 45, 1411: 6, 1502: 7, 1503: 2, 1504: 3, 1505: 6, 1506: 6, 1507: 6, 1508: 3, 1509: 2, 1510: 7}
LEVEL_MIN = [0,0,0,0,105,250,600,1100,1465,1880,2490]
LEVEL_MAX = [0,205,265,340,485,665,1110,1790,2275,2825,3650]
LEVEL_WORST = [0,4,5,6,7,8,9,9,9,9,9]

def prediction(start, target):
    a=BASIC_CODES[start]%10; b=BASIC_CODES[target]%10
    dr=target//100-start//100; dc=target%100-start%100
    q=3*dr*dr//2+dc*dc
    score=int(max(q,2)*(((a+3)/2)**2+((b+3)/2)**2)+30*((b-1)/2)**2-60)
    return dict(difficulty_score=score, score_model='base_only_no_feet_direction_or_rail_modifiers',
        matching_levels=[lv for lv in range(1,11) if LEVEL_MIN[lv]<=score<=LEVEL_MAX[lv] and b<=LEVEL_WORST[lv]],
        level_match_rule='score_window_and_target_hold_limit_only; not full geometric eligibility',
        from_rating=a,to_rating=b,weighted_distance_squared=q,
        level_min=LEVEL_MIN,level_max=LEVEL_MAX,level_worst=LEVEL_WORST)

FIELDS = ['timestamp_utc','session_id','climber','case_number','case_id',
          'case_sha256','dataset_sha256','status','rating','notes','elapsed_seconds',
          'dry_run','seed','command','difficulty_score','matching_levels','score_model','case_json']

def canonical(obj):
    return json.dumps(obj, sort_keys=True, separators=(',',':'), ensure_ascii=True)

def digest(obj):
    return hashlib.sha256(canonical(obj).encode()).hexdigest()

def hold_ref(ref):
    if isinstance(ref, dict):
        if set(ref)-{'hold','hard'}: raise ValueError('Unknown hold-reference field')
        value=ref.get('hold'); hard=ref.get('hard',False)
        if type(hard) is not bool: raise ValueError('hard must be true/false')
    else: value=ref; hard=False
    if type(value) is not int: raise ValueError('Hold must be an integer')
    row,col=divmod(value,100)
    if not (1<=row<=15 and 1<=col<=11):
        raise ValueError(f'Invalid physical hold {value}; use rails for special features')
    return value, value+(20 if hard else 0)

def encode(case):
    if not isinstance(case,dict) or not isinstance(case.get('id'),str) or not case['id'].strip():
        raise ValueError('Every case needs a nonempty string id')
    starts=case.get('starts',[]);feet=case.get('feet',[]);rails=case.get('rails',[])
    if not isinstance(starts,list) or not 1<=len(starts)<=2:
        raise ValueError('Each case requires 1-2 starts')
    if not isinstance(feet,list) or not isinstance(rails,list):
        raise ValueError('feet and rails must be arrays')
    refs=[hold_ref(x) for x in starts]+[hold_ref(case.get('target'))]+[hold_ref(x) for x in feet]
    if len({x[0] for x in refs})!=len(refs):
        raise ValueError('A physical hold cannot have two roles in one case')
    if any(not isinstance(x,str) or x not in RAILS for x in rails) or len(set(rails))!=len(rails):
        raise ValueError('Invalid or duplicate rail')
    values=[-hold_ref(x)[1] for x in starts]
    values += [10000+hold_ref(case['target'])[1]]
    values += [hold_ref(x)[1] for x in feet]+[RAILS[x] for x in rails]
    if len(values)>20: raise ValueError('Maximum 20 entries per case')
    values += [0]*(20-len(values))
    # The actual firmware accepts :X followed directly by 20 integers, NOT X98-.
    # A newline (no trailing comma) terminates the final value in its parser.
    command=':X'+','.join(map(str,values))+'\n'
    if len(command.encode())>=256: raise ValueError('Command exceeds firmware buffer')
    return values, command

def load_cases(path):
    data=json.loads(Path(path).read_text())
    if not isinstance(data,dict) or data.get('schema_version')!=1:
        raise ValueError('Expected schema_version 1')
    cases=data.get('cases')
    if not isinstance(cases,list) or not cases: raise ValueError('cases must be a nonempty array')
    ids=set()
    for c in cases:
        encode(c)
        if c['id'] in ids: raise ValueError('Duplicate case id: '+c['id'])
        ids.add(c['id'])
        score=c.get('difficulty_score')
        levels=c.get('matching_levels')
        if type(score) not in (int,float) or not isinstance(levels,list) or any(type(v) is not int or not 1<=v<=10 for v in levels):
            raise ValueError('Each case needs numeric difficulty_score and matching_levels (array of levels 1-10)')
    return cases,digest(data)

def example_cases():
    # Physical coordinates from the uploaded October 9 homewall_var.h.
    # Fixed broad-range prompts, NOT validated grade labels or guaranteed gastons.
    pairs=[(405,504),(405,606),(504,703),(604,805),(703,1003),
           (704,903),(805,1003),(904,1105),(1004,1403),(1006,1408)]
    cases=[]
    for j,(start,target) in enumerate(pairs):
        r,c=divmod(start,100)
        # Row 2 columns 4/8 and row 4 columns 4/8 are real holds on this board.
        frow=2 if r<=7 else 4
        variants=[('two_feet',[frow*100+4,frow*100+8],[]),
                  ('one_foot',[frow*100+4],[]),
                  ('left_rail',[],['left']),('no_feet',[],[])]
        for label,feet,rails in variants:
            cases.append({'id':f'T{len(cases)+1:03d}', 'starts':[start],
                'target':target,'feet':feet,'rails':rails,
                **prediction(start,target),
                'tags':[label,'directional_candidate' if j in (2,4,5,6,7,8,9) else 'baseline'],
                'instructions': 'Start matched on cyan. Move your right hand to pink and hold for 2 seconds; left hand stays on cyan. Use only listed feet/rails for feet, no unlit holds or floor. Establish the start with assistance if needed. Record any changed beta in the note.'})
    return {'schema_version':1,'description':'40 fixed pilot cases. Review physical usability first; labels are not predicted grades. Edit hand/beta to prescribe gastons. Rail conditions may be impractical on some moves.', 'cases':cases}

class Wall:
    def __init__(self, port, command_delay):
        self.port = port
        self.command_delay = command_delay
        if command_delay < 0:
            raise ValueError('--command-delay must be nonnegative')
        self.send(':P98\n')

    def send(self, command):
        # Write only. The HTTP monitor owns serial configuration and reads.
        fd = os.open(self.port, os.O_WRONLY | os.O_NOCTTY | os.O_NONBLOCK)
        try:
            subprocess.run(['printf', '%s', command], stdout=fd,
                           check=True, timeout=3)
        finally:
            os.close(fd)
        # Firmware uses readString(), so separate commands in time.
        time.sleep(self.command_delay)

    def display(self, values, command):
        self.send(command)

    def close(self):
        pass

def main():
    ap=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('cases',nargs='?',default='test_cases.json');ap.add_argument('--write-example',metavar='FILE')
    ap.add_argument('--port',default='/dev/ttyACM0')
    ap.add_argument('--command-delay',type=float,default=2.5,
                    help='Seconds to wait after each write (default: 2.5)')
    ap.add_argument('--log',default='homewall_ratings.csv');ap.add_argument('--climber',default='Shannon')
    ap.add_argument('--shuffle',dest='shuffle',action='store_true',default=True)
    ap.add_argument('--no-shuffle',dest='shuffle',action='store_false',help='Use input-file order')
    ap.add_argument('--seed',type=int,default=20261009)
    ap.add_argument('--repeat-rated',action='store_true');ap.add_argument('--dry-run',action='store_true')
    ap.add_argument('--validate',action='store_true',help='Validate cases and exit without serial or logging')
    args=ap.parse_args()
    if args.write_example:
        with open(args.write_example,'x') as f:json.dump(example_cases(),f,indent=2);f.write('\n')
        print('Wrote 40 pilot cases:',args.write_example);return
    if args.cases=='test_cases.json' and not Path(args.cases).exists():
        with open(args.cases,'x',encoding='utf-8') as f:
            json.dump(example_cases(),f,indent=2);f.write('\n')
        print('Created test_cases.json with 40 pilot cases.')
    cases,dataset=load_cases(args.cases)
    if args.validate: print(f'Validated {len(cases)} cases.');return
    log=Path(args.log)
    if log.resolve()==Path(args.cases).resolve():raise ValueError('Log cannot overwrite input')
    log.parent.mkdir(parents=True,exist_ok=True)
    session=str(uuid.uuid4());wall=None
    import fcntl
    with log.open('a+',newline='',encoding='utf-8') as f:
        fcntl.flock(f,fcntl.LOCK_EX|fcntl.LOCK_NB)
        f.seek(0);reader=csv.DictReader(f);rows=list(reader)
        if reader.fieldnames and reader.fieldnames!=FIELDS:raise ValueError('Existing log has a different schema; choose another --log')
        completed={(r['case_id'],r['case_sha256']) for r in rows if r['status']=='rated' and r['climber']==args.climber and r['dry_run']==str(args.dry_run)}
        queue=[(i+1,c) for i,c in enumerate(cases) if args.repeat_rated or (c['id'],digest(c)) not in completed]
        if args.shuffle:random.Random(args.seed).shuffle(queue)
        f.seek(0,2);writer=csv.DictWriter(f,fieldnames=FIELDS)
        if f.tell()==0:writer.writeheader();f.flush();os.fsync(f.fileno())
        if not queue:print('All cases already rated for this climber. Use --repeat-rated for another pass.');return
        print(f'{len(queue)} cases remaining. 1-10=difficulty, 11=impossible; r=repeat, s=skip, q=quit.')
        if not args.dry_run:print('Uses slot 98. Keep HTTP serial monitor running; avoid other wall commands. No acknowledgment checks. Ensure flip is OFF; visually check each move. Last case stays displayed on exit.')
        try:
            if not args.dry_run:wall=Wall(args.port,args.command_delay)
            for progress,(number,case) in enumerate(queue,1):
                values,command=encode(case)
                print(f'\nTest {number}: {case["id"]} ({progress}/{len(queue)})')
                print('Cyan starts:',case['starts'],' Pink target:',case['target'])
                print('Feet:',case.get('feet',[]),' Rails:',case.get('rails',[]))
                print(case.get('instructions',''))
                if wall:wall.display(values,command)
                else:print('DRY RUN:',command.strip())
                started=time.monotonic()
                while True:
                    answer=input('Rating 1-11 [optional note], r/s/q: ').strip()
                    parts=answer.split(maxsplit=1);token=parts[0].lower() if parts else ''
                    if token=='q':return
                    if token=='r':
                        if wall:wall.display(values,command)
                        else:print(command.strip())
                        continue
                    rating=int(token) if token.isdigit() else None
                    if token!='s' and (rating is None or not 1<=rating<=11):
                        print('Enter 1-11, r, s, or q.');continue
                    row=dict(timestamp_utc=datetime.now(timezone.utc).isoformat(),session_id=session,
                        climber=args.climber,case_number=number,case_id=case['id'],case_sha256=digest(case),
                        dataset_sha256=dataset,status='skipped' if token=='s' else 'rated',
                        rating='' if token=='s' else rating,notes=parts[1] if len(parts)>1 else '',
                        elapsed_seconds=round(time.monotonic()-started,2),dry_run=str(args.dry_run),
                        seed=args.seed,command=command.strip(),
                        difficulty_score=case.get('difficulty_score',''),
                        matching_levels=canonical(case.get('matching_levels',[])),
                        score_model=case.get('score_model','unspecified'),case_json=canonical(case))
                    writer.writerow(row);f.flush();os.fsync(f.fileno());print('Saved.');break
            print('Session complete:',log)
        finally:
            if wall:wall.close()

if __name__=='__main__':
    try: main()
    except (KeyboardInterrupt,EOFError): print('\nStopped; completed ratings are saved.')
    except Exception as exc:
        print(f'Error: {exc}',file=sys.stderr);sys.exit(1)
