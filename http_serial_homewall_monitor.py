import serial
import datetime
import time
import subprocess
import re
import random
import threading
import json
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


def get_current_date():
    return datetime.datetime.now().strftime("%Y-%m-%d")


def search_and_return_integers(filename, phrase):
    pattern = re.compile(r"(?<=- )(-?\d+(?:\s*,\s*-?\d+){19})")

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


TIMESTAMPED_LINE_PATTERN = re.compile(
    r"^(?P<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) - (?P<message>.*)$"
)
LEVEL_PATTERN = re.compile(r"^Level:\s*(?P<level>\d+)\b")


def get_recent_generated_climbs(filename=OUTPUT_FILE, limit=5):
    """
    Parse generated climbs from sequences like:

        timestamp - grw
        timestamp - arbitrary berlin
        timestamp - Level: 7 Max Diff: ...

    Results are newest first.
    """
    if limit < 1:
        return []

    path = Path(filename)
    if not path.exists():
        return []

    climbs = []
    pending = None

    with path.open("r", encoding="utf-8", errors="replace") as log_file:
        for raw_line in log_file:
            match = TIMESTAMPED_LINE_PATTERN.match(raw_line.rstrip("\r\n"))
            if not match:
                continue

            timestamp = match.group("timestamp")
            message = match.group("message").strip()

            if message == "grw":
                pending = {
                    "timestamp": timestamp,
                    "name": None,
                }
                continue

            if pending is None:
                continue

            if pending["name"] is None:
                if message:
                    pending["name"] = message
                continue

            level_match = LEVEL_PATTERN.match(message)
            if level_match:
                climbs.append(
                    {
                        "name": pending["name"],
                        "level": int(level_match.group("level")),
                        "timestamp": pending["timestamp"],
                    }
                )
                pending = None

    return climbs[-limit:][::-1]


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
            self.send_json(
                200,
                {
                    "status": "ok",
                    "service": "homewall",
                    "recent_climbs_endpoint": "/api/recent-climbs?limit=5",
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
    words = load_words()
    ser = serial.Serial()
    new_open = True

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

                    if data.startswith("ilookup:"):
                        phrase = data[len("ilookup:"):]
                        phrase = extract_last_two_words(phrase)
                        result = search_and_return_integers(OUTPUT_FILE, phrase)
                        print("Finding:", phrase)

                        if result:
                            data_to_send = ":X" + result
                            ser.write(data_to_send.encode("utf-8"))
                            ser.flush()
                            print(data_to_send)
                        else:
                            print("Phrase or problem not found.")

                    timestamp = datetime.datetime.now().strftime(
                        "%Y-%m-%d %H:%M:%S"
                    )
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
