WindEmu is an attempt to emulate the Psion Series 5mx (or as internally called, "Windermere"). I don't think anyone's done this before... or if they have, I can't find evidence online!

- Platform-independent core emulation library written in C/C++
- Qt5 front-end (currently quite barebones...)
- Very experimental
- Boots from the Psion 5mx Pro's Sys$rom.bin

Hardware features:

- ✅ LCD: partially implemented
- ✅ Keyboard: implemented
- ❌ Touch panel: not implemented
- ❌ Audio: not implemented
- ❌ Serial/UART support: stubbed out
- ❌ ETNA (CompactFlash): not implemented
- ✅ RTC: implemented
- ❌ RTC alarm: not implemented
- ❌ Standby mode: not implemented

Known issues:

- ROM path is hardcoded into WindQt/main.cpp right now
- Memory protection is not enforced
- Memory errors do not result in an Abort exception but instead make the emulator freak out
- Some keys do not work properly
- State is not saved (just like a real Psion :p)
- LCD controller is almost entirely unimplemented aside from the very basics to display the framebuffer
- EPOC misbehaves massively with memory banks larger than 0x800000 (may be an OS design flaw? need to confirm)
- 4bpp display mode does not decode correctly

Copyright
---------

The Psion-specific code is copyright (c) 2019 Ash Wolf.

The ARM emulation core is a modified version of the one used in [mGBA](https://github.com/mgba-emu/mgba) by endrift. 

WindEmu is available under the Mozilla Public License 2.0.

Resources
---------

Special thanks to [PsiLinux/OpenPsion](http://linux-7110.sourceforge.net/index.shtml) for providing an avenue to learn about the hardware definitions (registers, etc).

The EPOC C++ SDK is available here: https://web.archive.org/web/20071010101808/http://www.psionteklogix.com/teknet/pdk/netpad-pdk/epoc_downloads.htm

The ARM variant used in the 5mx is documented here: http://infocenter.arm.com/help/topic/com.arm.doc.ddi0033d/DDI0033D_710a_prelim_ds.pdf

The datasheet for the CL-PS7110 SoC used in the Series 5 (_not_ the 5mx) is available here: https://www.igorkov.org/revo/datasheets/CL-PS7110.pdf - while not identical to Windermere, some components operate in similar fashion.




