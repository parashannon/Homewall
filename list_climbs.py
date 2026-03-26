import re
from pathlib import Path

def parse_last_day_climbs():
    filepath = Path("all_homewall_serial_output.txt")

    lines = filepath.read_text(encoding="utf-8", errors="ignore").splitlines()

    line_pattern = re.compile(r"^(\d{4}-\d{2}-\d{2}) (\d{2}:\d{2}:\d{2}) - (.*)$")
    level_pattern = re.compile(r"Level:\s*(\d+)")

    # --- Find last day ---
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

    # --- Walk backwards through last day only ---
    while i >= 2:
        m = line_pattern.match(lines[i])

        if not m:
            i -= 1
            continue

        date_str, time_str, message = m.groups()

        if date_str != last_day:
            break

        if message.strip() == "grw":
            if i + 2 < len(lines):
                m_name = line_pattern.match(lines[i + 1])
                m_level = line_pattern.match(lines[i + 2])

                if m_name and m_level:
                    name = m_name.group(3).strip()

                    level_line = m_level.group(3)
                    time_of_climb = m_level.group(2)  # <-- extract time here

                    level_match = level_pattern.search(level_line)
                    level = level_match.group(1) if level_match else "UNKNOWN"

                    climbs.append((name, level, time_of_climb))

        i -= 1

    climbs.reverse()

    print(f"Last day in file: {last_day}\n")

    if not climbs:
        print("No climbs found.")
    else:
        for name, level, t in climbs:
            print(f"{name} -> Level {level} @ {t}")


if __name__ == "__main__":
    parse_last_day_climbs()