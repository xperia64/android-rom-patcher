@echo off

if "%1" == "" goto usage
if not exist %1.c goto notfound

echo (Turbo C 16-bit DOS) %1

rem -
rem - Have to be careful with optimization options; in
rem - particular, -O has demonstrably generated wrong code
rem -

TCC -IY:\TURBOC\INCLUDE -LY:\TURBOC\LIB -ml %1.c Y:\TURBOC\LIB\WILDARGS.OBJ
if errorlevel 1 goto exit
if exist %1.obj del %1.obj
if exist %1.o   del %1.o
upx --best %1.exe > nul
goto exit

:usage
echo Usage: %0 program_name
goto exit

:notfound
echo %1.c not found
goto exit

:exit
