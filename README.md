# cloudy
The files shared here work with the Mutable Dev Environment which is documented here - https://github.com/pichenettes/mutable-dev-environment.

# license
Code: MIT License. Hardware: CC-BY-SA-4.0 

# To build on MacOS without using the Vagrant setup

1. Clone cloudy somewhere. We’ll call it `cloudy/` for these instructions.
2. Install arm-none-eabi toolchain (official toolchain, preferably). 
    1. On Mac this installs into /Applications/ARM
3. Clone the mutable eurorack repository. You will need the folders stmlib and stm_audio_bootloader directories.
4. In your cloudy git dir copy the stmlib and stm_audio_bootloader directories from the eurorack repo into cloudy/, so that `cloudy/clouds`, `cloudy/stmlib`, and `cloudy/stm_audio_bootloader` directories are all children of `cloudy/` - NOTE: do NOT use symlinks. Things don't work right with them.
5. Change directories to cloudy/clouds
6. Execute `PYTHONPATH=. TOOLCHAIN_PATH=/Applications/ARM/ make -C .. -e -f clouds/makefile -j8 wav` (adjust the toolchain path if you installed the ARM binutils somewhere else, and if your system has only 2 or 4 processors choose -j2 or -j3)
7. You should end up with cloudy/build/clouds/clouds.bin, .hex, and .wav 

The ARM toolchain can be downloaded from https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads -> choose the MacOS PKG (not the tarball), and install it after download.
