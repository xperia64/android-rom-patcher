@echo off

if "%1" == "" goto usage
if not exist %1.c goto notfound

echo (DJGPP 32-bit DOS) %1

rem - You need djdev203, binutils/gcc, and pmode13b.
rem - This assumes DJGPP is in Y:\DJGPP\BIN.
rem - This also assumes UPX is somewhere in your path.

gcc -O9 -Wall -Wextra -Werror -fomit-frame-pointer %1.c -s -o %1.exe
IF ERRORLEVEL 1 goto exit

if exist %1.obj del %1.obj
if exist %1.o   del %1.o

exe2coff %1.exe
IF ERRORLEVEL 1 goto exit

copy /b Y:\DJGPP\BIN\PMODSTUB.EXE+%1 %1.exe > nul
IF ERRORLEVEL 1 goto exit

del %1
IF ERRORLEVEL 1 goto exit

upx --best %1.exe > nul
IF ERRORLEVEL 1 goto exit

goto exit

:usage
echo Usage: %0 program_name
goto exit

:notfound
echo %1.c not found
goto exit

:exit
