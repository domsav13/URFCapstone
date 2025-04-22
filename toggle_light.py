#!/usr/bin/env python3
"""
Simple CLI to toggle a light via GPIO17.
"""
import RPi.GPIO as GPIO
import sys

# Use BCM pin numbering
GPIO.setmode(GPIO.BCM)
REMOTE_PIN = 17

# Set up the pin as an output, defaulting to LOW (off)
GPIO.setup(REMOTE_PIN, GPIO.OUT, initial=GPIO.LOW)

def turn_on():
    GPIO.output(REMOTE_PIN, GPIO.HIGH)
    print("Light ON")

def turn_off():
    GPIO.output(REMOTE_PIN, GPIO.LOW)
    print("Light OFF")

def usage():
    print("Usage: python3 toggle_light.py [on|off|toggle]")
    sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        usage()

    cmd = sys.argv[1].lower()
    try:
        if cmd == "on":
            turn_on()
        elif cmd == "off":
            turn_off()
        elif cmd == "toggle":
            # read current state and flip it
            state = GPIO.input(REMOTE_PIN)
            GPIO.output(REMOTE_PIN, not state)
            print(f"Light {'ON' if not state else 'OFF'} (toggled)")
        else:
            usage()
    finally:
        GPIO.cleanup()
