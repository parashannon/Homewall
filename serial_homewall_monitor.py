import serial
import datetime
import time
import subprocess
import re
from random_word import RandomWords
r = RandomWords()

def get_current_date():
    return datetime.datetime.now().strftime("%Y-%m-%d")

def get_output_filename():
    return f'homewall_output_{get_current_date()}.txt'

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

# Initialize the output file
output_file = get_output_filename()

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
                if current_date != output_file[-14:-4]:
                    # Close the current file
                    file.close()

                    # Get the new output file name
                    output_file = get_output_filename()

                    # Open the new output file in append mode
                    file = open(output_file, 'a')

                # Read data from the serial port
                data = ser.readline().decode('utf-8').strip()
                
                # Check if the received line starts with "grw"
                if data.startswith("grw"):
                    word1 = r.get_random_word()
                    word2 = r.get_random_word()

                    # Keep generating random words until the total number of characters is less than or equal to 18
                    while len(word1) + len(word2) + 1 > 17:  # Adding 1 for the space between words
                        word1 = r.get_random_word()
                        word2 = r.get_random_word()

                    # Send the generated words over the serial port
                    ser.write(f"{word1} {word2}\n".encode())
                    print(f"{word1} {word2}")
                    time.sleep(0.005)
                    file.write(f"{timestamp} - {word1} {word2}\n")

                # Get the current timestamp
                timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                # Write data and timestamp to the file
                file.write(f"{timestamp} - {data}\n")
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
