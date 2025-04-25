import serial
import time
import sys
import tty
import termios

SERIAL_PORT = '/dev/ttyACM0'  # Update if needed
BAUD_RATE = 9600

# Accepted keys to send to Arduino
VALID_KEYS = ['w', 'a', 's', 'd', 'o', 'm', 'n']

def getch():
    """Read a single keypress without waiting for Enter (Linux/macOS/RPi only)"""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def main():
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            time.sleep(2)  # Allow Arduino time to reset
            print("Ready. Use keys: w/a/s/d/o/m/n — Press 'q' to quit.")

            while True:
                key = getch()

                if key in VALID_KEYS:
                    ser.write(f"{key}\n".encode('utf-8'))
                    print(f"Sent: {key}")
                elif key == 'q':
                    print("Exiting.")
                    break
                else:
                    print(f"Ignored: {key}")

    except serial.SerialException as e:
        print(f"Serial error: {e}")

if __name__ == '__main__':
    main()
