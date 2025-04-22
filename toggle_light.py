#!/usr/bin/env python3
"""
toggle_light.py – drive THN15N Remote pin on BCM17 to turn light ON/OFF.
"""
import RPi.GPIO as GPIO
import sys

REMOTE_PIN = 17  # BCM17

GPIO.setmode(GPIO.BCM)
GPIO.setup(REMOTE_PIN, GPIO.OUT)

def usage():
    print("Usage: python3 toggle_light.py [on|off|toggle]")
    sys.exit(1)

if len(sys.argv) != 2:
    usage()

cmd = sys.argv[1].lower()
if cmd == "on":
    GPIO.output(REMOTE_PIN, GPIO.HIGH)
    print("Light ON")
elif cmd == "off":
    GPIO.output(REMOTE_PIN, GPIO.LOW)
    print("Light OFF")
elif cmd == "toggle":
    new_state = not GPIO.input(REMOTE_PIN)
    GPIO.output(REMOTE_PIN, new_state)
    print(f"Light {'ON' if new_state else 'OFF'} (toggled)")
else:
    usage()

# ➔ DO NOT call GPIO.cleanup() here,
# so that the pin remains HIGH or LOW after the script ends.
