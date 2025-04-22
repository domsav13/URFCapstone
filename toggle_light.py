#!/usr/bin/env python3
import RPi.GPIO as GPIO
import sys
import time

GPIO.setmode(GPIO.BCM)
REMOTE = 17

def turn_on():
    # actively pull REMOTE to GND
    GPIO.setup(REMOTE, GPIO.OUT)
    GPIO.output(REMOTE, GPIO.LOW)
    print("Light ON")

def turn_off():
    # release pin (float)
    GPIO.setup(REMOTE, GPIO.IN)
    print("Light OFF")

def usage():
    print("Usage: toggle_light.py [on|off|toggle]")
    sys.exit(1)

if __name__=="__main__":
    if len(sys.argv)!=2: usage()
    cmd = sys.argv[1].lower()

    # We’ll read the pin only if it’s already been set as an output
    current = None
    try:
        if cmd=="on":
            turn_on()
        elif cmd=="off":
            turn_off()
        elif cmd=="toggle":
            # if it’s currently configured as output+LOW, we’re on
            if GPIO.gpio_function(REMOTE)==GPIO.OUT and GPIO.input(REMOTE)==0:
                turn_off()
            else:
                turn_on()
        else:
            usage()
    finally:
        time.sleep(0.1)
        GPIO.cleanup()
