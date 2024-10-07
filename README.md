# GDPico64
Raspberry pico based graphics card to emulate an 80s EF9365 based graphics card to be used in conjunction with a modular 8 bit Z80 based computer system.

# Background
In the mid/late 80s, german public television broadcasted a series of 26 episodes of a stepwise introduction to microcomputer technology. It started with the usual stuff like binary logic, memory and the purpose of a central processing unit. Parallel to the explanations the series worked on building a modular 8 bit microcomputer, which was based on an 8 bit bus system. While the basic system used the Z80 CPU, it was possible to extend the system to an 68008 CPU board and with some tricks even to an 68020 CPU.
A full set of information and the required software is available on the following website (most of it is in german language):
www.ndr-nkc.de
In its modular setup it is conceptually very similar to the RC2014* project.

# Project
One cornerstone of the modular computer system was the graphics card named GDP64, where the 64 stands for 64kB of dedicated graphics memory, which offered 4 separate pages of 512x256 monochrome pixels (rather hi-res for that time!). The GDP64 was designed around the EF9365 graphics chip, which seems to have been one of the first graphics chips to offer a hardware implementation of graphics primitives like lines or characters. Since the chip has a MAME (www.mamedev.org) implementation, it seems to have found some use outside of this modular computer project but overall it is rather exotic. While the line drawing performance was quite impressive for its time with up to 1 Mio. Pixels/s, it lacked some features like scrolling or dedicated text modes.

This project uses the capabilities of the RP2040 to create a graphics output to support halfway modern displays (i.e. it produces an analog VGA signal where the current preferred resolution is 1920x1080@60Hz). Furthermore, the RP2040 provides a parallel interface to enable use of the GDPico64 in the same environment as the original.

As some GPIOs were free after having 8 Bit Color VGA output and the parallel bus, the board offers a PS/2 interface to connect a keyboard (this emulates the old KEY group). As an additional means of interfacing with the Z80 system, the Pico's stdio functionaliy (i.e. UART via USB) can also be used as keyboard input.

# Installation
The project assumes that the standard Pico SDK has been setup as documented in the official docs.

First the repository needs to be cloned into a separate directory:
git clone https://github.com/okayserh/GDPico64

Submodules are used to provide the FreeRTOS functions. These need to be cloned by:
git init
git ???

For compiling the code a build directory needs to be setup with subsequent use of the CMake system
```
mkdir build
cd build
cmake ..
```

Building is than just
```
make
```

Installation on the Pico board follows the standard procedure. I.e. the .uf2 file created by the
build process is copied onto the drive presented by the Pico board.

# Usage
For the beginning, it is suggested to have a serial interface via USB. Once the initialization is completed, the monitor switches to "terminal" mode. In "terminal mode", the input received via the stdio serial interface is forwarded as key press to the Z80 system. Thus, the host keyboard can be used as input for the Z80 system. Pressing the ESC key of the host system, the software switches to "monitor mode". In monitor mode the following functions are available:

* X XModem receive
* S XModem send
* D Dump XModem buffer
* T Terminal mode
* I Dump IO buffer
* C Reset CAS bufptr
* R Reset Z80

The three functions XModem receive XModem send and Reset CAS bufptr are linked to the emulation of the original cassette interface (CAS). This interface uses a 6850 UART to convert data streams into recordable audio. The emulation uses a 4k buffer in RAM as a substitute for the cassette. The buffer can be filled from the host computer or the Z80. When the buffer has been filled via XModem, the pointer can be reset (via C) and the Z80 can read the data via the emulated 6850 interface. This allows the transmission of programs into the Z80 environment via XModem. For the opposite direction, the pointer into the buffer should be reset and the transmission from the Z80 be started. When the buffer has been filled, the XModem buffer can be send to the host computer via XModem.

With the "Dump XModel buffer" function, the current content of the cassette buffer can be displayed through the serial interface via USB.

As a debugging function, "Dump IO buffer" outputs all 256 IO registers that are used internally to emulate the different IO devices.

Finally, the "Reset Z80" function sends a reset signal via the Z80 bus and reinitializes the PIO handling the parallel bus.

# Remarks and TODOs
* As this is a weekend project, the code is not yet very nice from a software engineering or code quality perspective. Time allowing, the code quality will be improved and maybe also new features will be added.
* For many of the functions that are implemented in the code, I've been looking for sources to get some inspiration (DMA based LUT mapping, parallel port implementation). While there are some codes, I still think that the code may provide some insights into how those tasks could be done if these things should become part of another project.
* FreeRTOS was added in a later stage of the project as multiple things need to be monitored simultaneously and the initial simple scheduler became overly complex. Usage of FreeRTOS may not always be as it should be. But for the time being the code seems to work.
* Currently, the parallel interface answers all IO requests. This needs to be fixed when other cards are to be used. Otherwise, there will be a bus conflict when the CPU does an IO read to an address that should be answered by another IO card.
* The monitor is rather simple and only accessible from the USB serial interface. A possible extension would be to use the graphics output to provide a stand-alone monitor program that can be used to confgure the system
* Another extension would be to use the 8 bit color output to provide a color capable graphics interface. A simple concept would use the 4 pages as 4 bits encoding the color through the lookup table.
* Generally it is also conceivable to complete change the graphics interface into something more like an 80s homecomputer graphics.
* Addition of the floppy controller to enable the use of CP/M.

# Trademarks
    "RC2014" is a registered trademark of RFC2795 Ltd.
    Other names and brands may be claimed as the property of others.
