import serial
import datetime
import time

def get_current_date():
    return datetime.datetime.now().strftime("%Y-%m-%d")

def get_output_filename():
    return f'homewall_output_{get_current_date()}.txt'

# Set the serial port parameters
serial_port = '/dev/ttyACM1'  # Change this to your serial port
baud_rate = 115200  # Change this to your desired baud rate

# Open the serial port
ser = serial.Serial()

# Initialize the output file
output_file = get_output_filename()

# Open the output file in append mode
with open(output_file, 'a') as file:
    try:
        while True:
            # Check if the serial port is open
            if not ser.is_open:
                try:
                    # Attempt to reopen the serial port
                    ser = serial.Serial(serial_port, baud_rate)
                    print("Serial port reopened.")
                except serial.SerialException:
                    print("Failed to reopen serial port. Retrying in 10 seconds.")
                    time.sleep(10)
                    continue  # Retry the loop

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

            # Get the current timestamp
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            # Write data and timestamp to the file
            file.write(f"{timestamp} - {data}\n")
            file.flush()  # Ensure data is written to the file immediately

            # Print the data and timestamp to the console (optional)
            print(f"{timestamp} - {data}")

    except KeyboardInterrupt:
        print("Program terminated by user.")
    finally:
        # Close the serial port
        ser.close()
        # Close the last file
        file.close()
