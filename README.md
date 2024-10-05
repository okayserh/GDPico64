# GDPico64
Raspberry pico based graphics card to emulate an 80s EF9365 based graphics card to be used in conjunction with a modular 8 bit Z80 based computer system.

# Background
In the mid/late 80s, german public television broadcasted a series of 26 episodes of a stepwise introduction to microcomputer technology. It started with the usual stuff like binary logic, memory and the purpose of a central processing unit. Parallel to the explanations the series worked on building a modular 8 bit microcomputer, which was based on an 8 bit bus system. While the basic system used the Z80 CPU, it was possible to extend the system to an 68008 CPU board and with some tricks even to an 68020 CPU.
A full set of information and the required software is available on the following website (most of it is in german language):
www.ndr-nkc.de
In its modular setup it is conceptually very similar to the RC2014* project.

# Project
One cornerstone of the modular computer system was the graphics card named GDP64, where the 64 stands for 64kB of dedicated graphics memory, which offered 4 separate pages of 512x256 monochrome pixels (rather hi-res for that time!). The GDP64 was designed around the EF9365 graphics chip, which seems to have been one of the first graphics chips to offer a hardware implementation of graphics primitives like lines or charatcers. Since the chip has a MAME (www.mamedev.org) implementation, it seems to have found some use outside of this modular computer project but overall is rather exotic. While the line drawing performance was quite impressive for its time with up to 1 Mio. Pixels/s, it lacked some features like scrolling or dedicated text modes.

# Trademarks
    "RC2014" is a registered trademark of RFC2795 Ltd.
    Other names and brands may be claimed as the property of others.
