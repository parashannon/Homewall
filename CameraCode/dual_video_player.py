import tkinter as tk
from tkinter import filedialog
import vlc

class VideoPlayer:
    def __init__(self, parent):
        self.parent = parent
        self.instance = vlc.Instance()
        self.player = self.instance.media_player_new()

        # Create a canvas to embed the VLC video output
        self.canvas = tk.Canvas(parent, bg='black')
        self.canvas.pack(expand=True, fill="both")
        
        # Wait for the canvas to be created so we can get its window ID
        self.parent.update_idletasks()
        self.set_video_output()

    def set_video_output(self):
        # For Linux, use the X window ID
        win_id = self.canvas.winfo_id()
        self.player.set_xwindow(win_id)

    def load(self, filepath):
        media = self.instance.media_new(filepath)
        self.player.set_media(media)
        self.play()  # Optionally, start playing immediately

    def play(self):
        self.player.play()

    def pause(self):
        self.player.pause()

    def stop(self):
        self.player.stop()

def load_video(player):
    filepath = filedialog.askopenfilename(filetypes=[("Video Files", "*.mp4 *.avi *.mov")])
    if filepath:
        player.load(filepath)

def create_controls(frame, player):
    controls_frame = tk.Frame(frame)
    controls_frame.pack(pady=5)
    tk.Button(controls_frame, text="Play", command=player.play).pack(side="left", padx=5)
    tk.Button(controls_frame, text="Pause", command=player.pause).pack(side="left", padx=5)
    tk.Button(controls_frame, text="Load Video", command=lambda: load_video(player)).pack(side="left", padx=5)
    return controls_frame

# Create main window
root = tk.Tk()
root.title("Dual Video Player with VLC")

# Create left and right frames
left_frame = tk.Frame(root)
left_frame.pack(side="left", expand=True, fill="both", padx=10, pady=10)

right_frame = tk.Frame(root)
right_frame.pack(side="right", expand=True, fill="both", padx=10, pady=10)

# Create video player instances
left_player = VideoPlayer(left_frame)
create_controls(left_frame, left_player)

right_player = VideoPlayer(right_frame)
create_controls(right_frame, right_player)

def play_both():
    left_player.play()
    right_player.play()

# Global button to play both videos simultaneously
tk.Button(root, text="Play Both Videos", command=play_both).pack(pady=10)

root.mainloop()
