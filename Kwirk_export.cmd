@echo off
setlocal enabledelayedexpansion

cd ..

set "configFile=config.h"
set "tempFile=%temp%\config.tmp.h"

for /L %%i in (0, 1, 29) do (
    call :ReplaceLevel "%%i"
    msbuild search.sln /p:Configuration=Release /p:Platform=x64
    pushd Kwirk\solutions
    ..\..\x64\Release\search.exe
    popd
)

exit /b

:ReplaceLevel
(for /f "delims=" %%a in ('findstr /n "^" "%configFile%"') do (
    set "line=%%a"
    set "line=!line:*:=!"
    if "!line:~0,14!"=="#define LEVEL " (
        echo(#define LEVEL %%i
    ) else (
        echo(!line!
    )
)) > "%tempFile%"
move /y "%tempFile%" "%configFile%"
exit /b