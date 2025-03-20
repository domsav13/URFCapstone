# SmartSpot Capstone Project 2024-25
## Mechanical Engineering @ GWU

LOG & DESCRIPTIONS:
3/19/2025: need to combine Pi --> Arduino angle movement files with encoder files such that the motor can initialize absolute angular positions and move between angle commands according to encoder position. Encoder position is a value between 0 and 1024 that corresponds to an angle 0-360 degrees.

Working Programs:
- pi_motor/pi_motor.ino: arduino script that receives ON or OFF command from Pi, triggering motor
- pi_arduino.c: accompanying C script for user input
-- 
- shared_arduino/angle/angle.ino: arduino script that receives 0-360 degree input from Pi, moving motor that amount of degrees
- shared_arduino/angle_mover.c: accompanying C script for user input
--
- shared_arduino/motor_spinning/motor_spinning.ino: arduino script that HIGH/LOW's motor and collects encoder data
- shared_arduino/shared_memory_writer.c: writes encoder data from arduino serial monitor to shared memory
- shared_arduino/shared_data.h: shared memory variable structure
--
- user_interface/*: Python programs that deploy web user interfaces to (1) turn on/off motor with press of button and (2) turn motor number of degrees. These files do not interact with the Arduino however and need to be updated in that regard.
