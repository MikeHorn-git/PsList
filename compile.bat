@echo off
echo Compiling ...
cl /EHsc pslist.c

if %errorlevel% neq 0 (
    echo Compile failed.
    exit /b %errorlevel%
)
