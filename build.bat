@echo off

IF NOT EXIST build mkdir build
pushd build

rem Load the visual studio paths, for MSVC compiler
call vcvarsall.bat x64

echo Assemble the bootloader and kernel
nasm ../code/bootloader.asm -i ../code/ -f bin -o bootloader.bin
nasm ../code/kernel.asm -i ../code/ -f bin -o kernel.bin

echo Build fat12 program
cl -nologo -Zi -FC ../tools/fat12.c -link -incremental:no -opt:ref 

echo Formatting the floppy image
fat12 format

rem echo Building the floppy image
rem copy /b bootloader.bin+kernel.bin main_floppy.img 

popd