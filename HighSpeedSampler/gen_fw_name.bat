@echo off
setlocal enabledelayedexpansion

rem === ПРОВЕРКА АРГУМЕНТОВ VISUAL STUDIO ===
rem Скрипт ожидает, что ему передадут путь к собранному exe (макрос $(TargetPath))
if "%~1"=="" (
    echo Error: No target file specified. Pass $^(TargetPath^) as an argument.
    exit /b 1
)

rem Извлекаем полный путь, директорию и имя файла без расширения
set "TARGET_EXE=%~f1"
set "TARGET_DIR=%~dp1"
set "TARGET_NAME=%~n1"

rem === ПОЛУЧЕНИЕ ДАННЫХ ИЗ GIT ===
rem Переходим в директорию сборки для выполнения git команд
pushd "%TARGET_DIR%"

rem Получаем имя проекта из URL репозитория
for /f "tokens=3 delims=-." %%i in ('git config --local remote.origin.url 2^>NUL') do set git_proj=%%i

rem Получаем дату и время последнего коммита
for /f %%i in ('git log -1 "--pretty=%%cd" "--date=format:%%Y.%%m.%%d_%%H.%%M" 2^>NUL') do set git_date=%%i

popd

rem === FALLBACK (РЕЗЕРВНЫЕ ЗНАЧЕНИЯ) ===
rem Visual Studio запускает post-build из папки bin\Debug, где git может не сработать.
rem Если git не вернул данные, используем имя собранного файла и заглушку для даты.
if "!git_proj!"=="" set "git_proj=!TARGET_NAME!"
if "!git_date!"=="" set "git_date=no_date"

rem === ФОРМИРОВАНИЕ ИМЕНИ И КОПИРОВАНИЕ ===
set "f_name=!git_proj!_!git_date!"

if exist "%TARGET_EXE%" (
    rem Копируем exe в ту же директорию с новым именем
    copy /y "%TARGET_EXE%" "%TARGET_DIR%!f_name!.exe" 1>NUL
    echo Successfully created: !f_name!.exe
) else (
    echo Error: Source file not found at %TARGET_EXE%
    exit /b 1
)

endlocal