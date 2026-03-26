import re
from pathlib import Path

def parse_last_day_climbs():
    filepath = Path("all_homewall_serial_output.txt")

    lines = filepath.read_text(encoding="utf-8", errors="ignore").splitlines()

    line_pattern = re.compile(r"^(\d{4}-\d{2}-\d{2}) \d{2}:\d{2}:\d{2} - (.*)$")
    level_pattern = re.compile(r"Level:\s*(\d+)")

    # --- Step 1: find the last valid date by scanning backwards ---
    last_day = None
    for line in reversed(lines):
        m = line_pattern.match(line)
        if m:
            last_day = m.group(1)
            break

    if not last_day:
        print("No valid dated lines found.")
        return

    climbs = []

    i = len(lines) - 1

    # --- Step 2: walk backwards only within that day ---
    while i >= 2:
        m = line_pattern.match(lines[i])

        if not m:
            i -= 1
            continue

        date_str = m.group(1)

        # Stop once we leave the last day
        if date_str != last_day:
            break

        message = m.group(2).strip()

        # Look for "grw"
        if message == "grw":
            # Next two lines forward are name + level
            if i + 2 < len(lines):
                m_name = line_pattern.match(lines[i + 1])
                m_level = line_pattern.match(lines[i + 2])

                if m_name and m_level:
                    name = m_name.group(2).strip()
                    level_line = m_level.group(2)

                    level_match = level_pattern.search(level_line)
                    level = level_match.group(1) if level_match else "UNKNOWN"

                    climbs.append((name, level))

        i -= 1

    # We collected backwards → reverse to chronological order
    climbs.reverse()

    print(f"Last day in file: {last_day}\n")

    if not climbs:
        print("No climbs found.")
    else:
        for name, level in climbs:
            print(f"{name} -> Level {level}")


if __name__ == "__main__":
    parse_last_day_climbs()