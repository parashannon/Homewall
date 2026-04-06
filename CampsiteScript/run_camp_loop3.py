import time
import os
from datetime import datetime, timedelta

# Predefined park options
park_options = {
    1: {"name": "Joshua Tree", "parks": ["232472", "272300", "10056207"]},
    2: {"name": "Boulder Basin", "parks": ["232022"]},
    3: {"name": "Red Rock", "parks": ["10056208"]},
    4: {"name": "Kirk Creek", "parks": ["233116"]},
    5: {"name": "Plaskett Creek", "parks": ["231959"]},
    6: {"name": "Custom List", "parks": None},  # User will define custom parks
}

# Prompt the user to select a park or custom list
print("Select a camping location:")
for idx, option in park_options.items():
    print(f"{idx}. {option['name']}")

choice = int(input("Enter the number corresponding to your choice: "))

if choice not in park_options:
    print("Invalid choice. Exiting.")
    exit()

selected_option = park_options[choice]

if choice == 6:  # Custom list option
    custom_parks = input("Enter park IDs separated by spaces: ")
    selected_option["parks"] = custom_parks.split()

# Extract the selected park IDs
selected_parks = " ".join(selected_option["parks"])

# Define the start and end dates
start_date = input("Enter start date (YYYY-MM-DD): ")
end_date = input("Enter end date (YYYY-MM-DD): ")

# Convert start and end dates to datetime objects
start_date = datetime.strptime(start_date, "%Y-%m-%d")
end_date = datetime.strptime(end_date, "%Y-%m-%d")

# Calculate the next Saturday from the start date
next_saturday = start_date + timedelta(days=(5 - start_date.weekday()) % 7)

# Initialize an empty array to store the run_commands
run_commands = []

# Iterate over each weekend between the start and end dates
current_date = next_saturday
while current_date <= end_date:
    # Generate the run_command for the current weekend
    command = (
        f"python camping2.py --start-date {(current_date).strftime('%Y-%m-%d')} "
        f"--end-date {(current_date + timedelta(days=1)).strftime('%Y-%m-%d')} "
        f"--show-campsite-info --parks {selected_parks} > output.txt"
    )
    # Append the command to the run_commands array
    run_commands.append(command)
    # Move to the next weekend
    current_date += timedelta(days=7)

# Print the array of run_commands
for command in run_commands:
    print(command)

# Loop for checking campsites
i_weekend = 0
start_time = time.time()
lastime = 0
interval_time = 180 / len(run_commands)
printflag = 0
target_text = "there are campsites available"

while True:
    # Run the script and save output to a file
    if time.time() - lastime > interval_time:
        lastime = time.time()
        printflag = 0
        print("->", end='')
        time.sleep(1)
        run_command = run_commands[i_weekend]
        os.system(run_command)
        i_weekend += 1
        if i_weekend == len(run_commands):
            i_weekend = 0
        # Check if the target text is in the output
        with open("output.txt", "r") as f:
            output = f.read()
            now = datetime.now()
            current_time = now.strftime("%H:%M:%S")
            if target_text in output:
                print("Campsites Found !!!! ", current_time)
                os.system("start ding-sound-effect_2.mp3")
                print(output)
                ind1 = output.index("* Site")
                ind2 = output.index(" is available on the following dates:")
                camp_number = output[ind1 + 7:ind2]
                os.system(f"start chrome.exe --new-window https://www.recreation.gov/camping/campsites/{camp_number}")
            else:
                print(f"No luck at {current_time} for {run_command[18:-12]}", flush=True)
    else:
        t_left = interval_time - (time.time() - lastime)
        if printflag == 1:
            print('\b\b\b\b\b', end='', flush=True)
        print(f"{t_left:5.0f}", end='', flush=True)
        printflag = 1

    # Wait for 10 minutes before running the script again
    time.sleep(1)  # Adjust as necessary
