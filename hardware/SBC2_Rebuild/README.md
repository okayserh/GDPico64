# SBC2 "Rebuild"
The SBC2 was a card that included the Z80 CPU, 8k of EPROM and 4k of RAM on a single PCB. The EPROM was mapped to 0x0000-0x2000, while the RAM was located from 0x8000 to 0x9000. In order to have a complete system on the PCB, an oscillator circuit and the reset logic were also implemented on the board.

It is important to note, that this board was merely enough to use the basic program ("grundprogramm"), which could be considered a simple monitor with extensions to enter simple machine code programs and to write those programs to the cassette interface. For user interested in getting more from the modular concept, separate cards with distinct CPUs and Memories were offered. By using these cards, the system could be extended to 1MB of memory with some bank logic. In those configurations it was also possible to use the system together with CP/M.

Since my original SBC2 board was messed up with wrong solders, I decided to redraw the SBC2 with KiCad and order replacements from a PCB shop. The result can be found in this folder.

# Remarks:
The required memory types 2732 and 6116 are hard to come by today. With hindsight bias, it was not such a good idea to spend time to replicate this design. In my current working setup, the card is only populated with a single 6116, while the sockets are filled with wires to forward the address lines that are not provided to the bus in the original design. Also the reset logic is left unpopulated as the reset must be provided by the GDPico64 in order to have the parallel bus set up properly. Henc,e, in my working setup it is mostly used ad clock generator and holder for the Z80 CPU.
Another point which seems not quite up to date is the dimensioning of the pull-up resistors. Those are 1k resistors, which to me seem rather strong by current CMOS standards.
The logic for the clock generation works, but the signal is not very good. Using an off the shelve oscillator might have been a better option for the redesign.

# Licensing
The copyright of the original PCB design is by Rolf Dieter Klein, who was the mastermind behind the whole computer system
Copyright of PCB layouts seems to be a bit difficult. My understanding is that schematics cannot be copyrighted. However, individual layouts can be subject of copyright. In this work I've recreated the original layout in KiCad with some minimal modifications in routing. I don't know if this suffices to grant me the copyright on this design.
In case anyone feels that this design violates his/her copyright, please drop me a note and I'll take this part of the project offline.
