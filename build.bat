@echo off

IF NOT EXIST build mkdir build
pushd build

rem Load the visual studio paths, for MSVC compiler
call vcvarsall.bat x64

rem Assemble the bootloader and kernel
rem nasm ../code/bootloader.asm -i ../code/ -f bin -o bootloader.bin
rem nasm ../code/kernel.asm -i ../code/ -f bin -o kernel.bin

rem Build fat12 program
cl -nologo -Zi ../code/fat12.c -link -incremental:no -opt:ref 

rem Building the floppy image
rem copy /b bootloader.bin+kernel.bin main_floppy.img 

popd