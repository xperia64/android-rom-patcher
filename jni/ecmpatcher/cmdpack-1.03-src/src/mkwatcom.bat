@echo off

if "%1" == "" goto usage
if not exist %1.c goto notfound

echo (OpenWatcom 16-bit DOS) %1

PATH Y:\WATCOM\BINW;%PATH%
SET INCLUDE=Y:\WATCOM\H
SET WATCOM=Y:\WATCOM
SET EDPATH=Y:\WATCOM\EDDAT
SET WIPFC=Y:\WATCOM\WIPFC

if exist %1.err del %1.err

wcc -ml -j -0 -d0 -obmilers -we -wx -q %WATCOM%\src\startup\wildargv.c
if errorlevel 1 goto doerr

wcc -ml -j -0 -d0 -obmilers -we -wx -q %1.c
if exist %1.err goto doerr
if errorlevel 1 goto doerr

wlink system dos name %1.exe file %1.obj file wildargv.obj
if exist %1.err goto doerr
if errorlevel 1 goto doerr

if exist %1.obj del %1.obj

upx --best %1.exe > nul

goto exit

:doerr
if exist %1 type %1.err
if exist %1 del %1.err
goto exit

:usage
echo Usage: %0 program_name
goto exit

:notfound
echo %1.c not found
goto exit

:exit
if exist wildargv.obj del wildargv.obj
