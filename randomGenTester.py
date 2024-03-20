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
            if phrase.lower() in line.lower():
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


words = default_words

# Initialize the output file
output_file = 'random_gen_output.txt' #get_output_filename()
start_time = time.time()  # Record the start time
print(start_time)
new_open=1
status=1
iLvL=1
iProblem=1;
# Open the output file in append mode
with open(output_file, 'a') as file:

  
    while iLvL<11:
        time.sleep(0.25)
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
        if ser.is_open:
            if new_open==1:
                new_open=0;
        
                       
            if status==1 and time.time()-start_time > 2: 
                ser.write(f":R{iLvL}\n".encode())
                ser.flush()
                status=2
                print(f"Sending Request{iProblem}\n")
                print(f":R{iLvL}\n")

            # Read data from the serial port
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8').strip()
                 # Print the data and timestamp to the console (optional)
                print(f"{data}")

                # Check if the received line starts with "grw"
                if data.startswith("grw"):

                    word1 = "hello"
                    word2 = "world"

                    # Send the generated words over the serial port
                    # time.sleep(0.005)
                    ser.write(f"{word1} {word2}\n".encode())
                    ser.flush()
                    print(f"Generated: {word1} {word2}\n")
                try: 
                    integers = [int(x.strip()) for x in data.split(",")]    
                    if len(integers) == 20:
                        # Prompt the user for an integer to prepend to the line
                        prepend_integer = iLvL
                        file.write(f"{prepend_integer}, {data}\n")
                        file.flush()  # Ensure data is written to the file immediately
                        status=1
                        iProblem=iProblem+1
                        print(f"Problem: {iProblem}")
                        time.sleep(0.5)
                except: 
                    print("Not integers")

                if iProblem > 500:
                    iProblem=1
                    iLvL=iLvL+1
        

