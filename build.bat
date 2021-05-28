@echo off

IF NOT EXIST build mkdir build
pushd build

nasm ../source/bootloader.asm -i ../source/ -f bin -o bootloader.bin
nasm ../source/external_program.asm -i ../source/ -f bin -o external_program.bin

copy /b bootloader.bin+external_program.bin bootloader.flp 

popd