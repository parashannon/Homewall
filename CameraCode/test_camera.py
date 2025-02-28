import cv2

cap = cv2.VideoCapture(0)  # Try other indices if 0 doesn't work
if not cap.isOpened():
    print("Error: Could not open camera.")
    exit()

ret, frame = cap.read()
if ret:
    cv2.imwrite('test_frame.jpg', frame)
    print("Frame captured and saved as test_frame.jpg")
else:
    print("Error: Failed to capture frame")
cap.release()
