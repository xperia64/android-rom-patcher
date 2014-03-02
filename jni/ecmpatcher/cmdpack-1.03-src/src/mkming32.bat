@echo off

if "%1" == "" goto usage
if not exist %1.c goto notfound

echo (MinGW 32-bit Windows) %1

i686-w64-mingw32-gcc -DWINVER=0x0400 -O9 -Wall -Wextra -Werror -fomit-frame-pointer %1.c -s -o %1.exe

goto exit

:usage
echo Usage: %0 program_name
goto exit

:notfound
echo %1.c not found
goto exit

:exit
