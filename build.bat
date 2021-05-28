@echo off

IF NOT EXIST build mkdir build
pushd build

nasm ../source/bootloader.asm -f bin -o bootloader.flp

popd