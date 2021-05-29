# Michaelsoft-Binbows
The new amazing operating system!
![binbows](https://raw.githubusercontent.com/Aleman778/Michaelsoft-Binbows/main/banner.jpg)

## Setup Windows
Prerequsites:
1. Download Netwide Assembler (nasm.us), make sure this is on your path
2. Download Bochs x86 emulator (https://bochs.sourceforge.io/)

### Compile the project
Run the `build.bat` file which will create a floppy image in the `build` directory
which can be booted from.

### Setting up Bochs
Open up the bochs application and you will be presented by a GUI and we want
to load the floppy image.
In `Edit Options` naviate to `Disk & Boot` and double click it, this opens up a new
window and is where you can specify the floppy image.
The type of floppy drive is `5.25" 1.2M` and then select the bootloader floppy image
from the `build` directory (if the file is not visisble so change to `All files`).
Select the type of floppy media to be `1.2M` and set status to `inserted`,
press `OK`.
Before running it save the settings for later, click `Save` and place the
configuration file in your `build` directory with the name `bootloader`.
Now you can `Run` the program and it can also be executed directly via that file.

## 4coder
Using 4coder there is a project file which allows you to load the files
and build the project. `F1` is configured to build the project and
`F2` runs the `build\\bochsrc.bxrc`.
