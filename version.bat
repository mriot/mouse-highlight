@echo off
setlocal

:: Try WMIC first (for backwards compatibility)
WMIC.EXE Alias >NUL 2>&1
if %errorlevel%==0 (
	goto s_wmic
) else (
	goto s_powershell
)

:s_wmic
FOR /F "skip=1 tokens=1-6" %%G IN ('WMIC Path win32_utctime Get Day^,Hour^,Minute^,Month^,Second^,Year /Format:table 2^>NUL') DO (
	IF "%%~L"=="" GOTO s_done
	set "_yyyy=%%L"
	set "_mm=%%J"
	set "_dd=%%G"
	set "_hour=%%H"
	set "_minute=%%I"
)
goto s_done

:s_powershell
set "_tmp=%temp%\_getdate_%random%.txt"

:: Use PowerShell to output comma-separated integers (no leading zeros), write to temp file
powershell -NoProfile -Command "$d=Get-Date; '{0},{1},{2},{3},{4}' -f $d.Year,$d.Month,$d.Day,$d.Hour,$d.Minute" > "%_tmp%"
if not exist "%_tmp%" (
	echo ERROR: PowerShell did not produce output. 1>&2
	del "%_tmp%" 2>nul
	exit /b 1
)

:: Read the comma-separated values from the temp file
for /f "usebackq tokens=1-5 delims=," %%a in ("%_tmp%") do (
	set "_yyyy=%%a"
	set "_mm=%%b"
	set "_dd=%%c"
	set "_hour=%%d"
	set "_minute=%%e"
)

del "%_tmp%" 2>nul
goto s_done

:s_done
:: Set safe defaults if something went wrong
if not defined _yyyy set "_yyyy=0"
if not defined _mm    set "_mm=0"
if not defined _dd    set "_dd=0"
if not defined _hour  set "_hour=0"
if not defined _minute set "_minute=0"

:: Write Version.h
> "src\Version.h" echo #pragma once
>> "src\Version.h" echo #define V_MAJOR %_yyyy%
>> "src\Version.h" echo #define V_MINOR %_mm%
>> "src\Version.h" echo #define V_BUILD %_dd%

:: compute revision = minutes since midnight
set /a var_res=_hour * 60 + _minute
>> "src\Version.h" echo #define V_REVISION %var_res%

echo Created src\Version.h: %_yyyy%-%_mm%-%_dd% %_hour%:%_minute% (revision=%var_res%)
exit /b 0
