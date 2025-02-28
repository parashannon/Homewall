import cv2
import time
from picamera2 import Picamera2, Preview
import traceback

def main():
    try:
        print("Initializing Picamera2...")
        picam2 = Picamera2()

        # Create a preview configuration.
        # You can try using create_still_configuration() as an alternative.
        config = picam2.create_preview_configuration(main={"size": (640, 480)})
        print("Camera configuration:")
        print(config)
        picam2.configure(config)
        
        print("Starting camera...")
        picam2.start()
        
        # Give the camera a longer time to settle.
        time.sleep(2)
        
        # Capture a frame
        print("Capturing a frame...")
        frame = picam2.capture_array()
        
        if frame is None:
            print("Error: No frame returned by capture_array()")
        else:
            print("Frame captured successfully. Shape:", frame.shape)
            # Save frame to file for inspection
            cv2.imwrite("debug_capture.jpg", frame)
            print("Frame saved as debug_capture.jpg")
            
        # Display the frame using OpenCV (if applicable)
        cv2.imshow("Picamera2 Debug", frame)
        print("Displaying frame. Press any key to exit.")
        cv2.waitKey(0)
        cv2.destroyAllWindows()
        
        picam2.stop()
    except Exception as e:
        print("An exception occurred:")
        traceback.print_exc()

if __name__ == "__main__":
    main()
