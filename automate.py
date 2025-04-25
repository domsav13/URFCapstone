import serial
import time

# Adjust to your actual serial port (COMx on Windows, /dev/ttyUSBx or /dev/ttyACMx on Linux)
SERIAL_PORT = '/dev/ttyACM0'  
BAUD_RATE = 9600

# Commands you want to send (can be any pattern of WASD + Enter)
COMMANDS = ['w', 'a', 's', 'd', 'o']  # Add/remove as needed

# Delay between key presses
DELAY_BETWEEN_COMMANDS = 1.0  # seconds

def main():
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        time.sleep(2)  # Wait for Arduino to reset

        while True:
            for cmd in COMMANDS:
                ser.write(f"{cmd}\n".encode('utf-8'))  # send char + newline (like Enter key)
                print(f"Sent: {cmd}")
                time.sleep(DELAY_BETWEEN_COMMANDS)

if __name__ == '__main__':
    main()
