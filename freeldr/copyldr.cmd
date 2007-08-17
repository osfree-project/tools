@echo off
rem
rem Automatically build and copy os2ldr to ext2 disk image
rem

wmake all >wmake.log 2>&1

if errorlevel 1 goto end

copy os2ldr \sys\vm\vpc\bochs >\dev\nul 2>&1
cd \sys\vm\vpc\bochs
mcopy -D overwrite os2ldr d:/
cd img
qemu -hda hd10megdlx.img -hdb disk.img -hdc os3-ext2-lilo.img >\dev\nul 2>&1
cd \var\src\os2\boot\freeldr

:end