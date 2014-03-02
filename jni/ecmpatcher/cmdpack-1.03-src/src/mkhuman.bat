echo off

rem I'm using gcc 2.6.3 and the "Mr. DON" libcd4.tgz, and g2cpp.x is replaced with cpp.x from
rem the gcc 2.95.2 beta

if "%1" == "" goto usage
if not exist %1.c goto notfound

echo (gcc2 Human68k) %1

gcc2 -O9 -Wall -Werror -fomit-frame-pointer %1.c -s -o %1.x

del tmp.o > nul

goto exit

:usage
echo Usage: %0 program_name
goto exit

:notfound
echo %1.c not found
goto exit

:exit
