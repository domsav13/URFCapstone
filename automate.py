import serial
import time
import sys
import tty
import termios

SERIAL_PORT = '/dev/ttyACM0'  # Change this if needed
BAUD_RATE = 9600

def getch():
    """
    Reads a single keypress (like getchar, non-blocking, no Enter needed).
    Only works in terminals (Linux/macOS/RPi).
    """
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def main():
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            time.sleep(2)
            print("Ready. Press WASD or 'o'. Press 'q' to quit.")

            while True:
                key = getch()

                if key in ['w', 'a', 's', 'd', 'o']:
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
