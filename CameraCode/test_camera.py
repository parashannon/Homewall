import cv2
import time

device = '/dev/video0'
backend = cv2.CAP_V4L2
print(f"Attempting to open device {device} using backend {backend}")

cap = cv2.VideoCapture(device, backend)

# Check if the device was opened successfully
if not cap.isOpened():
    print("Error: Could not open the device.")
    # Try to get backend name if available (requires OpenCV 4.5+)
    try:
        backend_name = cap.getBackendName()
        print("Backend Name:", backend_name)
    except Exception as e:
        print("Could not get backend name:", e)
    exit()

# Optionally, set a desired resolution
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# Print out current capture properties
width = cap.get(cv2.CAP_PROP_FRAME_WIDTH)
height = cap.get(cv2.CAP_PROP_FRAME_HEIGHT)
fps = cap.get(cv2.CAP_PROP_FPS)
print(f"Capture Properties: Width={width}, Height={height}, FPS={fps}")

# Try multiple times to capture a frame, with a brief pause between attempts
max_attempts = 5
frame_captured = False
for attempt in range(max_attempts):
    ret, frame = cap.read()
    print(f"Attempt {attempt + 1}: ret = {ret}")
    if ret and frame is not None:
        cv2.imwrite('debug_frame.jpg', frame)
        print("Frame captured successfully and saved as debug_frame.jpg")
        frame_captured = True
        break
    else:
        print("No frame captured, retrying...")
        time.sleep(0.5)  # wait a bit before next attempt

if not frame_captured:
    print("Error: Failed to capture any frame after several attempts.")

cap.release()
