@echo off

IF NOT EXIST build mkdir build
pushd build


nasm ../source/bootloader.asm -i ../source/ -f bin -o bootloader.bin


nasm ../source/kernel.asm -i ../source/ -f bin -o kernel.bin

rem Building the floppy image
copy /b bootloader.bin+kernel.bin bootloader.img 

popd