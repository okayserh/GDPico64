# GDPico64, Hardware
Raspberry pico based graphics card to emulate an 80s EF9365 based graphics card to be used in conjunction with a modular 8 bit Z80 based computer system.

# Layout and functions
Since my background is more on the software side, this is the first PCB I've developed. While it works in my setup, it may lack some features professional PCB designers or electrical engineers would add.

It uses 74245 chips to translate the Z80 bus signals into 3.3v signals that are compatible to the Pico GPIO pins. In order to save GPIOs on the Pico, the address and data part of the bus is multiplixed by using two 74245 chips. For outputting data signals to the Z80 bus, the 74245 chips are used as well. However, for the controlling the RESET and the WAIT signal an 5V inverter is used. While the 74245 worked for those signals as well in my breadboard development setup, there seems to be a risk that the 74245 is used out of spec in this setup when pull-up resitors drive the signals to 5V.

Known shortcomings:
No protection against powering the Pico via the parallel bus and the USB port.
As of now, only tested with powering the whole system through the USB port of the Pico.
