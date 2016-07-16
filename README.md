# 7seg
7-segment display driver for eco-trailer

This program reads the watts generated and total kwh and drives a custom build display, consisting of two rows of 7" 7-segment displays.

The display consists of shift registers, each bit driving a segment.  The program converts the incoming data to digits, converts the digits to their 7-segment equivalent, and outputs 8 bytes to position the data correctly. 
