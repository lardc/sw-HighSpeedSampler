@echo off

setlocal
set APP=HighSpeedSampler.exe
set APP_PATH=D:\HighSpeedSampler\

pushd %APP_PATH%

:loop

tasklist /fi "ImageName eq %APP%" /fo csv 2>NUL | find /I "%APP%">NUL
if "%ERRORLEVEL%"=="1" %APP_PATH%%APP%
ping 127.0.0.1 -n 5 > nul

goto loop
