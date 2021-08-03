@echo off

setlocal
set APP=HighSpeedSampler.exe
set APP_PATH=D:\HighSpeedSampler\
set CNT=1

pushd %APP_PATH%
echo Pre-launch 30s pause
ping 127.0.0.1 -n 30 > nul

:loop
echo %TIME%
echo Attempt %CNT%
tasklist /fi "ImageName eq %APP%" /fo csv 2>NUL | find /I "%APP%">NUL
if "%ERRORLEVEL%"=="1" %APP_PATH%%APP%
ping 127.0.0.1 -n 10 > nul
set /a CNT=%CNT%+1
echo --------

if "%CNT%"=="4" (
	goto loop_exit
) else (
	goto loop
)

:loop_exit
shutdown -r -f -t 0