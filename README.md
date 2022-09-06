# Michaelsoft-Binbows
The new amazing operating system!
## Progress
This is very early in development, so far I have implemented a boot loader, FAT12 file system formatter, ps2 keyboard driver, 64-bit mode kernel loader and simple printf.
![printf in action](https://raw.githubusercontent.com/Aleman778/Michaelsoft-Binbows/main/kernel_printf.png)

## Setup Windows
Prerequsites:
1. Download Netwide Assembler https://nasm.us, make sure this is on your path
2. Download Bochs x86 emulator https://bochs.sourceforge.io/

### Compile the project
Run the `build.bat` file which will create a floppy image in the `build` directory
which can be booted from.

### Setting up Bochs
Open up the bochs application and you will be presented by a GUI and we want
to load the floppy image.
In `Edit Options` naviate to `Disk & Boot` and double click it, this opens up a new
window and is where you can specify the floppy image.
The type of floppy drive is `3.5" 1.44M` and then select the `main_floppy.img` file
from the `build` directory.
Select the type of floppy media to be `1.44M` and set status to `inserted`,
press `OK`.
Before running it save the settings for later, click `Save` and place the
configuration file in your `build` directory with the default name `bochsrc`.
Now you can `Run` the program and it can also be executed directly via that file.

## 4coder
Using 4coder there is a project file which allows you to load the files
and build the project. `F1` is configured to build the project and
`F2` runs the `build\\bochsrc.bxrc`.

![binbows](https://raw.githubusercontent.com/Aleman778/Michaelsoft-Binbows/main/banner.jpg)
The name is just a joke, don't sue me.
