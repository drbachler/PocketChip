# PocketChip

Next Thing Co. seems to be going out of business so this repo is to consolidate
all the resources required to keep it alive, in case all resources become
unavailable in the future.

NOTE: this repo is a snapshot of all the tools and code needed for the
PocketChip to work as I wnat to, in case at some point in the future they become
unavailable. The original repos will be mentioned when necessary.

## ChipSDK

All the tools needed to flash new images into your CHIP, and specifically for
PocketChip. Some tools are built from source and others are installed through
OS' package manager, currently is defined for Debian based OSs. To install the
thing, run:

```bash
./setup.sh
```

*Orignial repositories*: <https://github.com/NextThingCo/CHIP-SDK>,
<https://github.com/NextThingCo/CHIP-tools>,
<http://github.com/nextthingco/chip-mtd-utils>,
<http://github.com/NextThingCo/CHIP-buildroot>,
<http://github.com/linux-sunxi/sunxi-tools>

### Flashing PocketChip

First of all, to be able to flash any image into your PocketChip, with the
device turend off, you must connect FEL and GND pins in your CHIP. Once you have
done that you have to turn on the device.

There are two ways of flashing:

1. Using *CHIP-tools*. To flash a new image into your PocketChip run the
    following:

    ```bash
    FEL='sudo sunxi-fel'
    FASTBOOT='sudo fastboot'
    SNIB=false

    ./CHIP-tools/chip-update-firmware.sh -L CHIP-images/stable-pocketchip-b126
    ```
2. Using *Vagrant*. The Vagrant VM contains all the flash images available in
   "flash-collection". To flash a new image into your PocketChip:

   * Make sure you've got the latest VirtualBox and the Extension Pack
     installed.
   * Import the OVA to VirtualBox
   * Extract CHIP-SDK (it contains the SSH-Key to connect to the machine)
   * Change to CHIP-SDK directory.
   * Run "vagrant up"
   * Run "vagrant ssh" to connect to the machine.
   * Now follow the process outlined in README.md

## Pocket-home Marshmallow edition

User [o-marshmallow](https://github.com/o-marshmallow) has created an alternative "desktop"
environment for the PocketChip, that can be found in his
[repo](https://github.com/o-marshmallow/PocketCHIP-pocket-home). Specific
instructions on how to compile and install this thing can be found inside the
specific folder, although I recomend install directly from the deb package
(pocket-home\_0.0.8.9-1\_armhf.deb)

Some notes extracted from [Next Thing Co. Forum]
(https://bbs.nextthing.co/t/apt-pocket-home-marshmallow-edition/6579/1):

1. How to add an icon or change background color/image?
 From the *Personalize* screen in *Settings*

2. How to make an icon execute a script?
let’s say you script is located in `/home/chip/myscript.sh`, in the *Command*
field of the icon, specify:
`vala-terminal -fs 8 -g 20 20 -e "/home/chip/myscript.sh"`

## Retro Arch

It is possible to install Retro Arch emulator ro run ROMs from different
platforms (GB, GBc, NES, ...). To install it, it must be built from source. To
perform such task the following commands must be executed:

```bash
sudo apt-get update

sudo apt-get install -y libsdl1.2-dev libsdl2-dev libboost-system-dev
libboost-filesystem-dev libboost-date-time-dev libfreeimage-dev libfreetype6-dev
libeigen3-dev libcurl4-openssl-dev libasound2-dev libgl1-mesa-dev cmake
build-essential git pkg-config 

git clone git://github.com/libretro/RetroArch.git

cd RetroArch/

./configure --enable-opengles --disable-oss --disable-sdl --disable-ffmpeg
--disable-vg --disable-cg --enable-neon --enable-floathard

make

sudo make install
```

Once installed There will be no images in the interface, to solve that you have
to navigate to *Main Menu*, then *Online Updater* and, finally, *Update Assets*.
Next thing to do is to enable advanced settings. To do so navigate to
*Settings*, then *User Interface* and enable *Show Advanced Settings*. To
finalize with the setting process we are going to proide an URL to download
retro arch cores (emulators). Navigate to *Settings*, then *Network*, after that
*Updater* and finally *Buildbat Cores URL* and you input:
http://buildbot/libretro.com/nightly/linux/armhf/

