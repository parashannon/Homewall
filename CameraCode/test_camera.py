import cv2

# Use the device path with the V4L2 backend
cap = cv2.VideoCapture('/dev/video0', cv2.CAP_V4L2)

if not cap.isOpened():
    print("Error: Could not open /dev/video0")
    exit()

ret, frame = cap.read()
if ret:
    cv2.imwrite('debug_frame.jpg', frame)
    print("Frame captured successfully!")
else:
    print("Error: Failed to capture frame.")

cap.release()
