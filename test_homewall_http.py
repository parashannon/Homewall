import json
import urllib.request

URL = "http://192.168.4.46:8080/api/recent-climbs?limit=5"

try:
    with urllib.request.urlopen(URL, timeout=5) as response:
        payload = json.load(response)

    print(f"Received {payload['count']} climbs:\n")
    for climb in payload["climbs"]:
        print(
            f"Level {climb['level']}: {climb['name']} "
            f"({climb['timestamp']})"
        )

except Exception as error:
    print("Request failed:", error)
