import serial
import datetime
import time
import subprocess
import re
import random
import threading
import json
from collections import deque
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

BAUD_RATE = 115200
OUTPUT_FILE = "all_homewall_serial_output.txt"
WORD_LIST_FILE = "WordList.txt"

# The Pi's fixed LAN address is 192.168.4.46.
# Bind to all interfaces so the service is reachable at:
# http://192.168.4.46:8080
HTTP_HOST = "0.0.0.0"
HTTP_PORT = 8080

# ---------------------------------------------------------------------------
# LITE CACHE SETTINGS
# ---------------------------------------------------------------------------
# Keep more history in RAM than the API currently allows so the API can be
# expanded later without changing the startup parser.
RECENT_CLIMBS_CACHE_SIZE = 1000

# In-memory caches. These are rebuilt from the existing log once at startup,
# then updated directly as new serial data arrives.
recent_climbs = deque(maxlen=RECENT_CLIMBS_CACHE_SIZE)
problem_lookup = {}

# Protects the caches because the HTTP server runs in another thread.
cache_lock = threading.Lock()

# Matches a logged problem line containing exactly 20 comma-separated integers.
PROBLEM_VALUES_PATTERN = re.compile(
    r"(?<=- )(-?\d+(?:\s*,\s*-?\d+){19})"
)

TIMESTAMPED_LINE_PATTERN = re.compile(
    r"^(?P<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) - (?P<message>.*)$"
)
LEVEL_PATTERN = re.compile(r"^Level:\s*(?P<level>\d+)\b")


def get_current_date():
    return datetime.datetime.now().strftime("%Y-%m-%d")


# ---------------------------------------------------------------------------
# BACKWARD-COMPATIBLE DISK LOOKUP
# ---------------------------------------------------------------------------
# This is retained as a fallback for old/odd log entries that were not indexed
# during startup. Normal generated-climb lookups should now be served from RAM.
def search_and_return_integers(filename, phrase):
    pattern = PROBLEM_VALUES_PATTERN

    with open(filename, "r", encoding="utf-8", errors="replace") as file:
        found_phrase = False

        for line in file:
            if phrase.lower() in line.lower():
                found_phrase = True
            elif found_phrase:
                match = pattern.search(line)
                if match:
                    return match.group(1)

    return None


def extract_last_two_words(phrase):
    cleaned_phrase = "".join(
        character
        for character in phrase
        if character.isalpha() or character.isspace()
    )
    words = cleaned_phrase.split()
    return " ".join(words[-2:]) if len(words) >= 2 else cleaned_phrase


def get_serial_port_name():
    command = 'dmesg | grep "cdc_acm 1-1.1:1.0:"'
    dmesg_output = subprocess.check_output(
        command,
        shell=True,
        stderr=subprocess.STDOUT,
    ).decode("utf-8", errors="replace")

    lines = [line for line in dmesg_output.splitlines() if line.strip()]
    if not lines:
        raise FileNotFoundError("No matching Arduino entry found in dmesg.")

    line = lines[-1]
    print(line)

    match = re.search(r"tty([^\s:]+)", line)
    if not match:
        raise FileNotFoundError("Matching dmesg line did not contain a tty device.")

    serial_port = "/dev/" + match.group(0)
    print("Using serial port:", serial_port)
    return serial_port


# ---------------------------------------------------------------------------
# ONE-TIME STARTUP CACHE BUILD
# ---------------------------------------------------------------------------
def load_caches_from_log(filename=OUTPUT_FILE):
    """
    Scan the existing append-only log ONCE at startup.

    Builds:
      1. recent_climbs: deque of the newest generated climbs
      2. problem_lookup: name -> newest stored 20-integer problem string

    After startup, normal API requests and generated-climb lookups do not need
    to rescan the full log.
    """
    path = Path(filename)

    if not path.exists():
        print(f"No existing log found at {filename}; starting with empty caches.")
        return

    local_recent = deque(maxlen=RECENT_CLIMBS_CACHE_SIZE)
    local_lookup = {}

    pending = None
    pending_problem_name = None

    started = time.monotonic()

    with path.open("r", encoding="utf-8", errors="replace") as log_file:
        for raw_line in log_file:
            raw_line = raw_line.rstrip("\r\n")

            timestamp_match = TIMESTAMPED_LINE_PATTERN.match(raw_line)
            if not timestamp_match:
                continue

            timestamp = timestamp_match.group("timestamp")
            message = timestamp_match.group("message").strip()

            # A newly generated climb starts with "grw".
            if message == "grw":
                pending = {
                    "timestamp": timestamp,
                    "name": None,
                    "level_recorded": False,
                }
                pending_problem_name = None
                continue

            if pending is not None:
                # The monitor writes the generated two-word name immediately
                # after the grw line.
                if pending["name"] is None:
                    if message:
                        pending["name"] = message
                        pending_problem_name = message.lower()
                    continue

                # Cache the climb as soon as its level line appears.
                if not pending["level_recorded"]:
                    level_match = LEVEL_PATTERN.match(message)
                    if level_match:
                        local_recent.append(
                            {
                                "name": pending["name"],
                                "level": int(level_match.group("level")),
                                "timestamp": pending["timestamp"],
                            }
                        )
                        pending["level_recorded"] = True

                # Later in the same generated-climb output, the Arduino prints
                # the final 20-integer problem. Index it under the generated name.
                problem_match = PROBLEM_VALUES_PATTERN.search(raw_line)
                if problem_match and pending_problem_name:
                    local_lookup[pending_problem_name] = problem_match.group(1)
                    pending = None
                    pending_problem_name = None
                    continue

    with cache_lock:
        recent_climbs.clear()
        recent_climbs.extend(local_recent)

        problem_lookup.clear()
        problem_lookup.update(local_lookup)

    elapsed = time.monotonic() - started
    print(
        f"Startup cache loaded: {len(recent_climbs)} recent climbs, "
        f"{len(problem_lookup)} named problems in {elapsed:.2f} s."
    )


# ---------------------------------------------------------------------------
# FAST RAM-BASED RECENT CLIMB LOOKUP
# ---------------------------------------------------------------------------
def get_recent_generated_climbs(filename=OUTPUT_FILE, limit=5):
    """
    Return generated climbs newest-first from RAM.

    'filename' is retained in the function signature for backward compatibility,
    but normal calls no longer read the file.
    """
    if limit < 1:
        return []

    with cache_lock:
        cached = list(recent_climbs)

    return cached[-limit:][::-1]


def get_problem_by_name(phrase):
    """
    Fast lookup from the in-memory name index.

    Falls back to the original disk scan if the name is not indexed, preserving
    compatibility with older or unusual log entries.
    """
    key = phrase.strip().lower()

    with cache_lock:
        result = problem_lookup.get(key)

    if result:
        return result

    # Compatibility fallback. This should be uncommon after startup indexing.
    return search_and_return_integers(OUTPUT_FILE, phrase)


class HomeWallRequestHandler(BaseHTTPRequestHandler):
    def send_json(self, status_code, payload):
        encoded = json.dumps(payload, indent=2).encode("utf-8")

        self.send_response(status_code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(encoded)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(encoded)

    def do_GET(self):
        request = urlparse(self.path)

        if request.path in ("/", "/health"):
            with cache_lock:
                cached_climb_count = len(recent_climbs)
                indexed_problem_count = len(problem_lookup)

            self.send_json(
                200,
                {
                    "status": "ok",
                    "service": "homewall-lite",
                    "recent_climbs_endpoint": "/api/recent-climbs?limit=5",
                    "cached_climbs": cached_climb_count,
                    "indexed_problems": indexed_problem_count,
                },
            )
            return

        if request.path == "/api/recent-climbs":
            query = parse_qs(request.query)

            try:
                limit = int(query.get("limit", ["5"])[0])
            except ValueError:
                self.send_json(400, {"error": "limit must be an integer"})
                return

            limit = max(1, min(limit, 100))
            climbs = get_recent_generated_climbs(OUTPUT_FILE, limit)

            self.send_json(
                200,
                {
                    "count": len(climbs),
                    "climbs": climbs,
                },
            )
            return

        self.send_json(404, {"error": "not found"})

    def log_message(self, format_string, *args):
        print(
            f"{datetime.datetime.now():%Y-%m-%d %H:%M:%S} - HTTP - "
            + format_string % args
        )


def run_http_server():
    server = ThreadingHTTPServer((HTTP_HOST, HTTP_PORT), HomeWallRequestHandler)
    print(f"HTTP server available at http://192.168.4.46:{HTTP_PORT}")
    server.serve_forever()


def load_words():
    defaults = ["hello", "world"]

    try:
        with open(WORD_LIST_FILE, "r", encoding="utf-8") as word_file:
            words = [line.strip() for line in word_file if line.strip()]
        return words if words else defaults
    except OSError:
        print("Failed to read WordList.txt. Using default words.")
        return defaults


def main():
    # -----------------------------------------------------------------------
    # LITE: Read the historical log once before serving HTTP requests.
    # -----------------------------------------------------------------------
    load_caches_from_log(OUTPUT_FILE)

    words = load_words()
    ser = serial.Serial()
    new_open = True

    # Tracks the generated climb currently arriving over serial so the RAM
    # caches can be updated directly without re-reading the log.
    pending_live_climb = None
    pending_live_problem_name = None

    http_thread = threading.Thread(target=run_http_server, daemon=True)
    http_thread.start()

    with open(OUTPUT_FILE, "a", encoding="utf-8") as file:
        try:
            while True:
                try:
                    if not ser.is_open:
                        new_open = True
                        serial_port_name = get_serial_port_name()

                        if serial_port_name:
                            ser = serial.Serial(
                                serial_port_name,
                                BAUD_RATE,
                                timeout=2,
                            )
                            print(f"Serial port opened: {serial_port_name}")

                    if ser.is_open and new_open:
                        new_open = False
                        ser.write(b":V\n")
                        ser.flush()

                    data = ser.readline().decode(
                        "utf-8",
                        errors="replace",
                    ).strip()

                    if not data:
                        continue

                    timestamp = datetime.datetime.now().strftime(
                        "%Y-%m-%d %H:%M:%S"
                    )

                    if data.startswith("grw"):
                        word1 = random.choice(words)
                        word2 = random.choice(words)

                        while len(word1) + len(word2) + 1 > 16:
                            word1 = random.choice(words)
                            word2 = random.choice(words)

                        generated_name = f"{word1} {word2}"
                        ser.write((generated_name + "\n").encode("utf-8"))
                        ser.flush()
                        print(f"Generated: {generated_name}")

                        # LITE: remember this climb in RAM as it is generated.
                        pending_live_climb = {
                            "timestamp": timestamp,
                            "name": generated_name,
                            "level_recorded": False,
                        }
                        pending_live_problem_name = generated_name.lower()

                    # -------------------------------------------------------------------
                    # LITE: Add the new climb to the deque as soon as Level is reported.
                    # -------------------------------------------------------------------
                    level_match = LEVEL_PATTERN.match(data)
                    if (
                        level_match
                        and pending_live_climb is not None
                        and not pending_live_climb["level_recorded"]
                    ):
                        climb = {
                            "name": pending_live_climb["name"],
                            "level": int(level_match.group("level")),
                            "timestamp": pending_live_climb["timestamp"],
                        }

                        with cache_lock:
                            recent_climbs.append(climb)

                        pending_live_climb["level_recorded"] = True

                    # -------------------------------------------------------------------
                    # LITE: Capture the final 20-value problem and index it by name.
                    # -------------------------------------------------------------------
                    problem_match = PROBLEM_VALUES_PATTERN.search(
                        f"{timestamp} - {data}"
                    )
                    if problem_match and pending_live_problem_name:
                        with cache_lock:
                            problem_lookup[pending_live_problem_name] = (
                                problem_match.group(1)
                            )

                        pending_live_climb = None
                        pending_live_problem_name = None

                    if data.startswith("ilookup:"):
                        phrase = data[len("ilookup:"):]
                        phrase = extract_last_two_words(phrase)
                        result = get_problem_by_name(phrase)
                        print("Finding:", phrase)

                        if result:
                            data_to_send = ":X" + result
                            ser.write(data_to_send.encode("utf-8"))
                            ser.flush()
                            print(data_to_send)
                        else:
                            print("Phrase or problem not found.")

                    # Keep the original append-only log format unchanged.
                    file.write(f"{timestamp} - {data}\n")

                    if data.startswith("grw"):
                        file.write(f"{timestamp} - {generated_name}\n")

                    file.flush()
                    print(f"{timestamp} - {data}")

                except Exception as error:
                    print(f"Serial port error: {error}")

                    try:
                        ser.close()
                    except Exception:
                        pass

                    time.sleep(20)

        except KeyboardInterrupt:
            print("Program terminated by user.")

        finally:
            try:
                ser.close()
            except Exception:
                pass


if __name__ == "__main__":
    main()
