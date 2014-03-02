@echo off

rem - Build using various Microsoft C++ variants

if "%1" == "" goto usage
if not exist %1.c goto notfound

rem - Platform selection

if "%CPU%" == "ALPHA" goto axp
goto msc

rem - Build using the Microsoft/Digital AXP C++ compiler
rem - (part of the NT 3.5 SDK, I think)
:axp
echo (Digital AXP C++) %1
cl /W3 /WX /O /Gs /Gy %1.c /link /subsystem:console /opt:ref /fixed /release /nodefaultlib:oldnames.lib kernel32.lib user32.lib setargv.obj
IF ERRORLEVEL 1 goto exit
goto cleanup

rem - Build using just regular Microsoft C++?
:msc
echo (Microsoft C++) %1
cl /W3 /WX /Ox /Gy %1.c /link /fixed /opt:ref /release user32.lib setargv.obj
IF ERRORLEVEL 1 goto exit
goto cleanup

:cleanup
if exist %1.obj del %1.obj
if exist %1.o   del %1.o
goto exit

:usage
echo Usage: %0 program_name
goto exit

:notfound
echo %1.c not found
goto exit

:exit
