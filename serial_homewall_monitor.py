import serial
import datetime
import time
import subprocess
import re
import random


def get_current_date():
    return datetime.datetime.now().strftime("%Y-%m-%d")

def get_output_filename():
    return f'homewall_output_{get_current_date()}.txt'
    
    
    import re

def search_and_return_integers(filename, phrase):
    pattern = re.compile(r'(?<=- )(-?\d+(?:\s*,\s*-?\d+){19})')
    with open(filename, 'r') as file:
        found_phrase = False
        for line in file:
            if phrase in line:
                found_phrase = True
            elif found_phrase:
                match = pattern.search(line)
                if match:
                    return match.group(1)
    return None
    
def extract_last_two_words(phrase):
    # Remove any characters that are not letters or spaces from the phrase
    cleaned_phrase = ''.join(filter(lambda x: x.isalpha() or x.isspace(), phrase))
    # Split the cleaned phrase into words
    words = cleaned_phrase.split()
    # Return the last two words joined by a space, or the entire phrase if it has less than two words
    return ' '.join(words[-2:]) if len(words) >= 2 else cleaned_phrase


def get_serial_port_name():
    latest_ttyacm = None

    # Run dmesg and filter with grep to get the serial port name attached to 1-1.4
    dmesg_output = subprocess.check_output(['dmesg | grep "cdc_acm 1-1.4:1.0:"'], shell=True).decode('utf-8')
    lines = dmesg_output.split('\n')
    line=lines[-2]
    print(line)
    pattern= re.compile(r'tty([^\s:]+)')
    match = pattern.search(line)
    print(match)
    latest_ttyacm='/dev/'+match.group(0)
    print(latest_ttyacm)


    return latest_ttyacm

# Set the serial port parameters
baud_rate = 115200  # Change this to your desired baud rate

# Open the serial port
ser = serial.Serial()

# Default words
default_words = ["hello", "world"]

# Try to read words from the text file, use default words if it fails
try:
    with open("WordList.txt", "r") as f:
        words = f.read().splitlines()
except:
    print("Failed to read WordList.txt. Using default words.")
    words = default_words

# Initialize the output file
output_file = 'all_homewall_serial_output.txt' #get_output_filename()

# 
new_open=1;

# Open the output file in append mode
with open(output_file, 'a') as file:


    
    
 

    try:
        while True:
            try:
                # Check if the serial port is open
                if not ser.is_open:
                    # Attempt to reopen the serial port
                    new_open=1;
                    serial_port_name = get_serial_port_name()
                    if serial_port_name:
                        ser = serial.Serial(serial_port_name, baud_rate)
                        print(f"Serial port reopened: {serial_port_name}")
 
                    else:
                        print("Serial port not found. Retrying in 10 seconds.")
                if ser.is_open:
                    if new_open==1:
                        new_open=0;
                        ser.write(f":V\n".encode())
                
                
                # Check if it's a new day
                current_date = get_current_date()
                #if current_date != output_file[-14:-4]:
                #    # Close the current file
                #    file.close()

                #   # Get the new output file name
                #    output_file = get_output_filename()

                #    # Open the new output file in append mode
                #    file = open(output_file, 'a')
                
                # test the file search


                # Read data from the serial port
                data = ser.readline().decode('utf-8').strip()
                
                # Check if the received line starts with "grw"
                if data.startswith("grw"):
                    # time.sleep(0.005)
                    word1 = random.choice(words)
                    word2 = random.choice(words)

                    # Keep generating random words until the total number of characters is less than or equal to 18
                    while len(word1) + len(word2) + 1 > 16:  # Adding 1 for the space between words
                        word1 = random.choice(words)
                        word2 = random.choice(words)

                    # Send the generated words over the serial port
                    # time.sleep(0.005)
                    ser.write(f"{word1} {word2}\n".encode())
                    ser.flush()
                    print(f"Generated: {word1} {word2}\n")
                    
                if data.startswith("ilookup:"):
                    phrase=data[len("ilookup:"):]
                    
                    phrase=extract_last_two_words(phrase)
                    result = search_and_return_integers(output_file, phrase)
                    print('Finding:' + phrase)
                    if result:
                        time.sleep(0.005)
                        data_to_send = ":X" + data
                        ser.write(data_to_send.encode())
                        print(data_to_send)
                        ser.flush()
                    else:
                        print("Phrase or problem not found.")

                # Get the current timestamp
                timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                # Write data and timestamp to the file
                file.write(f"{timestamp} - {data}\n")
                if data.startswith("grw"):
                    file.write(f"{timestamp} - {word1} {word2}\n")
                
                file.flush()  # Ensure data is written to the file immediately

                # Print the data and timestamp to the console (optional)
                print(f"{timestamp} - {data}")
                


            except:
                # Handle serial port exception
                try:
                    print("Serial port error. Retrying")
                    
                    serial_port_name = get_serial_port_name()
                    if serial_port_name:
                        ser = serial.Serial(serial_port_name, baud_rate)
                        print(f"Serial port reopened: {serial_port_name}")
                    else:
                        print("Serial port not found. Retrying in 10 seconds.")
                except:
                    print("Could not connect")
                    ser.close()
                time.sleep(20)

    except KeyboardInterrupt:
        print("Program terminated by user.")
    finally:
        # Close the serial port
        ser.close()
        # Close the last file
        file.close()
