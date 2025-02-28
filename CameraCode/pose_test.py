import cv2
from picamera2 import Picamera2
import time

# Initialize and configure the camera for preview mode
picam2 = Picamera2()
config = picam2.create_preview_configuration(main={"size": (640, 480)})
picam2.configure(config)
picam2.start()
time.sleep(1)  # Allow time for the camera to adjust

print("Camera started. Press 'c' to capture an image, 'q' to quit.")

while True:
    # Capture the current frame as a NumPy array
    frame = picam2.capture_array()

    # Display the frame in an OpenCV window
    cv2.imshow("Picamera2 Video", frame)
    key = cv2.waitKey(1) & 0xFF

    if key == ord('c'):
        # Save the current frame as an image file
        cv2.imwrite("capture.jpg", frame)
        print("Image captured and saved as capture.jpg")
    elif key == ord('q'):
        break

# Clean up
picam2.stop()
cv2.destroyAllWindows()
