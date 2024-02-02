import serial
import datetime
import time
import subprocess

def get_current_date():
    return datetime.datetime.now().strftime("%Y-%m-%d")

def get_output_filename():
    return f'homewall_output_{get_current_date()}.txt'

def get_serial_port_name():
    latest_ttyacm = None

    try:
        # Run dmesg and filter with grep to get the serial port name attached to 1-1.4
        dmesg_output = subprocess.check_output(['dmesg | grep "USB ACM device"'], shell=True).decode('utf-8')
        lines = dmesg_output.split('\n')

        for line in lines:
            if "usb 1-1.4" in line and "cdc_acm" in line:
                parts = line.split()
                for i, part in enumerate(parts):
                    print(part)
                    if part == "cdc_acm" and i < len(parts) - 1:
                        latest_ttyacm = parts[i + 1]
                        print(latest_ttyacm)
    except subprocess.CalledProcessError as e:
        print(f"Error running dmesg: {e}")

    return latest_ttyacm

# Set the serial port parameters
baud_rate = 115200  # Change this to your desired baud rate

# Open the serial port
ser = serial.Serial()

# Initialize the output file
output_file = get_output_filename()

# Open the output file in append mode
with open(output_file, 'a') as file:
    last_error_time = None

    try:
        while True:
            try:
                # Check if the serial port is open
                if not ser.is_open:
                    # Attempt to reopen the serial port
                    serial_port_name = get_serial_port_name()

                    if serial_port_name:
                        ser = serial.Serial(serial_port_name, baud_rate)
                        print(f"Serial port reopened: {serial_port_name}")
                        last_error_time = None  # Reset error time on successful connection
                    else:
                        print("Serial port not found. Retrying in 10 seconds.")

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

            except serial.SerialException:
                # Handle serial port exception
                if last_error_time is None or time.time() - last_error_time >= 10:
                    print("Serial port error. Retrying in 10 seconds.")
                    last_error_time = time.time()
                time.sleep(1)

    except KeyboardInterrupt:
        print("Program terminated by user.")
    finally:
        # Close the serial port
        ser.close()
        # Close the last file
        file.close()
