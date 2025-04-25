import cv2
import numpy as np
import time
from picamera2 import Picamera2
from tflite_runtime.interpreter import Interpreter

# ----------------------------
# Configuration and Constants
# ----------------------------
MODEL_PATH = "4.tflite"
INPUT_SIZE = 192  # Model expects 192x192 input

# Define keypoints (for labeling, if desired) and the skeletal connections (edges)
KEYPOINT_DICT = {
    0: 'nose', 1: 'left_eye', 2: 'right_eye', 3: 'left_ear', 4: 'right_ear',
    5: 'left_shoulder', 6: 'right_shoulder', 7: 'left_elbow', 8: 'right_elbow',
    9: 'left_wrist', 10: 'right_wrist', 11: 'left_hip', 12: 'right_hip',
    13: 'left_knee', 14: 'right_knee', 15: 'left_ankle', 16: 'right_ankle'
}
EDGES = [
    (0, 1), (0, 2), (1, 3), (2, 4),
    (0, 5), (0, 6), (5, 7), (7, 9),
    (6, 8), (8, 10), (5, 11), (6, 12),
    (11, 13), (13, 15), (12, 14), (14, 16)
]

# ----------------------------
# Helper Functions
# ----------------------------
def preprocess_frame(frame):
    # Convert BGR (from Picamera2) to RGB
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    # Resize the frame to 192x192
    resized_frame = cv2.resize(rgb_frame, (192, 192))
    # Add one batch dimension to get shape (1, 192, 192, 3)
    input_data = np.expand_dims(resized_frame.astype(np.uint8), axis=0)
    return input_data



def draw_keypoints(frame, keypoints, threshold=0.3):
    """Draw detected keypoints and skeleton on the frame."""
    h, w, _ = frame.shape
    # Draw keypoints
    for idx, kp in enumerate(keypoints):
        y, x, conf = kp
        if conf > threshold:
            cx, cy = int(x * w), int(y * h)
            cv2.circle(frame, (cx, cy), 4, (0, 255, 0), -1)
    # Draw skeleton lines
    for edge in EDGES:
        p1, p2 = edge
        if keypoints[p1][2] > threshold and keypoints[p2][2] > threshold:
            x1, y1 = int(keypoints[p1][1] * w), int(keypoints[p1][0] * h)
            x2, y2 = int(keypoints[p2][1] * w), int(keypoints[p2][0] * h)
            cv2.line(frame, (x1, y1), (x2, y2), (255, 0, 0), 2)
    return frame

# ----------------------------
# Initialize TFLite Interpreter for MoveNet
# ----------------------------
print("Loading MoveNet model...")
interpreter = Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("Input details:", input_details)
print("Output details:", output_details)

# ----------------------------
# Initialize Picamera2
# ----------------------------
print("Initializing Picamera2...")
picam2 = Picamera2()
# Create a preview configuration with desired resolution
config = picam2.create_preview_configuration(main={"size": (640, 480)})
picam2.configure(config)
picam2.start()

# Allow the camera some time to adjust
time.sleep(2)
print("Camera started. Running pose estimation... Press 'q' to quit.")

# ----------------------------
# Main Loop: Capture, Inference, and Display
# ----------------------------
while True:
    # Capture a frame from the camera as a NumPy array
    frame = picam2.capture_array()
    if frame is None:
        print("Warning: No frame captured.")
        continue

    # Preprocess the frame for the model
    input_data = preprocess_frame(frame)
    print("Expected input shape:", input_details[0]['shape'])

    # Run inference
    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()
    # The model outputs shape [1, 1, 17, 3]: (y, x, confidence) for each keypoint
    keypoints = interpreter.get_tensor(output_details[0]['index'])[0][0]

    # Draw the keypoints and skeleton on a copy of the original frame
    output_frame = draw_keypoints(frame.copy(), keypoints)

    # Display the result using OpenCV
    cv2.imshow("Pose Estimation", output_frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# ----------------------------
# Cleanup
# ----------------------------
picam2.stop()
cv2.destroyAllWindows()
