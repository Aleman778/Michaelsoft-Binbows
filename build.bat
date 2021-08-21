@echo off

rem Load the visual studio paths, for MSVC compiler
call vcvarsall.bat x64

IF NOT EXIST build mkdir build
pushd build

echo Assemble the bootloader and kernel loader
nasm ../code/boot.asm -i ../code/ -f bin -o boot.bin
nasm ../code/kernel_loader.asm -i ../code/ -f elf64 -o kernel_loader.o

rem TODO(alexander): more compiler flags
echo Building fat12 tools
cl -nologo -Zi -FC ../tools/fat12.c -link -incremental:no -opt:ref 

echo Building the kernel

wsl $WSLENV/x86_64-elf-gcc ../code/kernel.c kernel_loader.o -o kernel.bin -ffreestanding -mno-red-zone -fno-exceptions -nostdlib -Wall -Wextra -Werror -T ../code/kernel_linker.ld
%rem clang -target x86_64-pc-none-elf ../code/kernel.c kernel_loader.o -o kernel.bin -ffreestanding -mno-red-zone -fno-exceptions -nostdlib -Wall -Wextra -Werror -T ../code/kernel_linker.ld

echo Formatting the floppy image
fat12 format

popd
