import re
from pathlib import Path

def parse_last_day_climbs(filepath):
    path = Path(filepath)
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()

    # Pattern for lines like:
    # 2025-04-24 19:25:24 - grw
    line_pattern = re.compile(r"^(\d{4}-\d{2}-\d{2}) \d{2}:\d{2}:\d{2} - (.*)$")
    level_pattern = re.compile(r"Level:\s*(\d+)")

    parsed = []
    all_dates = []

    # First pass: parse date + message from each valid log line
    for line in lines:
        m = line_pattern.match(line)
        if m:
            date_str = m.group(1)
            message = m.group(2).strip()
            parsed.append((date_str, message, line))
            all_dates.append(date_str)
        else:
            parsed.append((None, None, line))

    if not all_dates:
        return None, []

    # Find the most recent date in the file
    last_day = max(all_dates)

    climbs = []

    # Second pass: find "grw" entries on the last day
    for i in range(len(parsed) - 2):
        date_str, message, _ = parsed[i]

        if date_str == last_day and message == "grw":
            next_date, next_message, _ = parsed[i + 1]
            level_date, level_message, _ = parsed[i + 2]

            # Make sure the name and level lines are also from the same day
            if next_date == last_day and level_date == last_day:
                name = next_message
                level_match = level_pattern.search(level_message)

                if level_match:
                    level = level_match.group(1)
                else:
                    level = None

                climbs.append({
                    "name": name,
                    "level": level,
                    "raw_level_line": level_message
                })

    return last_day, climbs


if __name__ == "__main__":
    # Change this to your actual file name
    filename = "output.txt"

    last_day, climbs = parse_last_day_climbs(filename)

    if last_day is None:
        print("No valid dated log lines found.")
    else:
        print(f"Last day in file: {last_day}")
        print()

        if not climbs:
            print("No climbs found for that day.")
        else:
            for climb in climbs:
                print(f"{climb['name']} -> Level {climb['level']}")