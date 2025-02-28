import cv2
import numpy as np
from tflite_runtime.interpreter import Interpreter

# --- Configuration ---
MODEL_PATH = '1.tflite'
INPUT_SIZE = 192  # MoveNet Lightning uses 192x192 input

# List of keypoints for visualization (indices correspond to the model output order)
KEYPOINT_DICT = {
    0: 'nose', 1: 'left_eye', 2: 'right_eye', 3: 'left_ear', 4: 'right_ear',
    5: 'left_shoulder', 6: 'right_shoulder', 7: 'left_elbow', 8: 'right_elbow',
    9: 'left_wrist', 10: 'right_wrist', 11: 'left_hip', 12: 'right_hip',
    13: 'left_knee', 14: 'right_knee', 15: 'left_ankle', 16: 'right_ankle'
}

# Skeleton connections (pairs of keypoint indices) for drawing lines
EDGES = [
    (0, 1), (0, 2), (1, 3), (2, 4),
    (0, 5), (0, 6), (5, 7), (7, 9),
    (6, 8), (8, 10), (5, 11), (6, 12),
    (11, 13), (13, 15), (12, 14), (14, 16)
]

# --- Load the TFLite Model ---
interpreter = Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# --- Preprocessing function ---
def preprocess_frame(frame):
    # Resize and pad the frame to maintain aspect ratio if needed
    h, w, _ = frame.shape
    # Convert BGR (OpenCV) to RGB
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    # Resize to model input size
    resized_frame = cv2.resize(rgb_frame, (INPUT_SIZE, INPUT_SIZE))
    # Normalize pixel values to [0, 1]
    input_data = np.expand_dims(resized_frame.astype(np.float32) / 255.0, axis=0)
    return input_data

# --- Postprocessing function ---
def draw_keypoints(frame, keypoints, threshold=0.3):
    h, w, _ = frame.shape
    for idx, keypoint in enumerate(keypoints):
        y, x, conf = keypoint
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

# --- Video Capture Setup ---
cap = cv2.VideoCapture(0)  # Use device 0; change if using a different camera

if not cap.isOpened():
    print("Error: Could not open video capture.")
    exit()

print("Starting video stream. Press 'q' to quit.")

while True:
    ret, frame = cap.read()
    if not ret:
        print("Error: Failed to capture frame")
        break

    # Preprocess frame for model input
    input_data = preprocess_frame(frame)

    # Set tensor and invoke inference
    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()
    # Output shape is typically [1, 1, 17, 3] for MoveNet: (y, x, confidence)
    keypoints = interpreter.get_tensor(output_details[0]['index'])[0][0]

    # Draw keypoints and skeleton on original frame
    output_frame = draw_keypoints(frame.copy(), keypoints)

    # Display the output frame
    cv2.imshow('MoveNet Pose Estimation', output_frame)

    # Press 'q' key to exit
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
