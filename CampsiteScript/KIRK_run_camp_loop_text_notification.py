import time
import os
from datetime import datetime, timedelta
import smtplib
from email.mime.text import MIMEText
from gmx_email import send_gmx_email  # Import the function from gmx_email.py

# Define the path to the script that you want to run
script_path = "D:\\Stuff\\PythonStuff\\recreation-gov-campsite-checker-master\\camping.py"

# Define the text that you are searching for in the script's output
target_text = "there are campsites available"

# 233116 Kirk Creek
# 231959 Plaskett Creek
# 232472 JT indian cove
# 272300 JT Jumbo rocks
# 10056207 JT Ryan

# Define the start date for checking campsites
#next_saturday = (datetime.today() + timedelta(days=(5 - datetime.today().weekday()) % 7)).strftime('%Y-%m-%d')
#start_date = input(f"Enter start date ({next_saturday} is the next Saturday by default): ") or next_saturday
#run_command = f"python camping2.py --start-date {start_date} --end-date {datetime.strptime(start_date, '%Y-%m-%d') + timedelta(days=1):%Y-%m-%d} --show-campsite-info --parks 233116 231959 > output.txt"


# Define the start and end dates
#start_date = input("Enter start date (YYYY-MM-DD): ")
#end_date = input("Enter end date (YYYY-MM-DD): ")


start_date="2026-06-01"
end_date="2026-07-01";
black_list=["69843"];

# Convert start and end dates to datetime objects
start_date = datetime.strptime(start_date, "%Y-%m-%d")
end_date = datetime.strptime(end_date, "%Y-%m-%d")

# Calculate the next Saturday from the start date
next_saturday = (start_date + timedelta(days=(5 - start_date.weekday()) % 7))

# Initialize an empty array to store the run_commands
run_commands = []

# Iterate over each weekend between the start and end dates
current_date = next_saturday


siteid="233116"

while current_date <= end_date:
    # Generate the run_command for the current weekend
    # command = f"python camping2.py --start-date {(current_date + timedelta(days=-1)).strftime('%Y-%m-%d')} --end-date {(current_date + timedelta(days=1)).strftime('%Y-%m-%d')} --show-campsite-info --parks 233116 231959 > output.txt"
    command = f"python camping2.py --start-date {(current_date + timedelta(days=-1)).strftime('%Y-%m-%d')} --end-date \
    {(current_date + timedelta(days=2)).strftime('%Y-%m-%d')} --show-campsite-info --parks "+ siteid+ " > output.txt"
    
    # Append the command to the run_commands array
    run_commands.append(command)
    
    # Move to the next weekend
    current_date += timedelta(days=7)

# Print the array of run_commands
for i in range(0,len(run_commands)-1):
    print(run_commands[i])

send_gmx_email(
        gmx_username = "shannon.eilers@gmail.com",    # Your GMX email address
        gmx_password = "jwcdoriktahqfhll",      # GMX password (or app password if 2FA is enabled)
        to_address   = "4352321110@vtext.com",          # Recipient's email address
        subject="Wish me Luck",
        body=command
    )



i_weekend=0;

start_time = time.time()
lastime = 0
interval_time = 180/len(run_commands);
printflag = 0

while True:
    # Run the script and save output to a file
    if time.time()-lastime > interval_time:
        lastime = time.time()
        printflag = 0
        print("->", end='')
        time.sleep(1)
        run_command=run_commands[i_weekend];
        os.system(run_command)
        i_weekend=i_weekend+1;
        if i_weekend== len(run_commands):
            i_weekend=0;
        # Check if the target text is in the output

        with open("output.txt", "r") as f:
            output = f.read()
            now = datetime.now()
            current_time = now.strftime("%H:%M:%S")
            if target_text in output:
                # Play a sound if the target text is found
                
                print("Campsites Found !!!! ", current_time)
                ind1 = output.index("* Site")
                ind2 = output.index(" is available on the following dates:")
                camp_number = output[ind1+7:ind2]
                if camp_number in black_list:
                    print('site found but on blacklist')
                else:				
                    os.system("start ding-sound-effect_2.mp3")
                    print(output)
                    link = f"https://www.recreation.gov/camping/campsites/{camp_number}"
                    os.system("start chrome.exe --new-window " + link)
                    # os.system(f"start chrome.exe --new-window https://www.recreation.gov/camping/campsites/{camp_number}")
                    send_gmx_email(
                        #gmx_username="seilersjunker@gmx.com",
                        #gmx_password="xiljwsvesczmvgun",
                        # ouxcxkdbrsmjfaet
                        #to_address="4352321110@vtext.com",
                        
                        gmx_username = "shannon.eilers@gmail.com",    # Your GMX email address
                        gmx_password = "jwcdoriktahqfhll",      # GMX password (or app password if 2FA is enabled)
                        to_address   = "4352321110@vtext.com",          # Recipient's email address
                        subject="SITE FOUND",
                        body="site found:" + link + '!'
                        )
            else:
                print(f"No luck at {current_time} for {run_command[18:-12]}", flush=True)
    else:
        t_left = interval_time - (time.time()-lastime)
        if printflag == 1:
            print('\b\b\b\b\b', end='', flush=True)
        print(f"{t_left:5.0f}", end='', flush=True)
        printflag = 1

    # Wait for 10 minutes before running the script again
    time.sleep(1) # 10 minutes expressed in seconds
