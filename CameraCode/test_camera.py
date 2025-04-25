import cv2
import time

successful_devices = []

# Adjust the range if needed (e.g., range(8) for /dev/video0 to /dev/video7)
for i in range(35):
    device = f'/dev/video{i}'
    print(f"\nTesting device: {device}")
    
    # Try opening the device with the V4L2 backend
    cap = cv2.VideoCapture(device, cv2.CAP_V4L2)
    if not cap.isOpened():
        print(f"Error: Could not open {device}")
        continue

    # Optionally, set a resolution
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    # Give the camera some time to initialize
    time.sleep(0.5)
    
    ret, frame = cap.read()
    print(f"Attempt to read from {device}: ret = {ret}")
    if ret and frame is not None:
        print(f"Success: Device {device} captured a frame.")
        cv2.imwrite(f'debug_frame_{i}.jpg', frame)
        successful_devices.append(device)
    else:
        print(f"Error: Device {device} failed to capture a frame.")
    
    cap.release()

if successful_devices:
    print("\nSuccessful devices:")
    for dev in successful_devices:
        print(f" - {dev}")
else:
    print("\nNo devices succeeded in capturing a frame.")
