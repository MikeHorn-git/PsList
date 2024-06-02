# Requirements
* [Msbuild](https://visualstudio.microsoft.com/downloads/)


# Compilation
The source file is a standalone C file.  

## Automatic build
```batch
.\compile.bat
```
## Manual build
```batch
cl.exe /EHsc pslist.c
```

## Flags
* EH : Exception Handling
* sc : Use the standard C++ EH model

# Usage
* .\pslist.exe -d <processId> : View threads of the specified process ID..
* .\pslist.exe <processName> : View processes with the specified name.
* .\pslist.exe -t : Display threads for listed processes.
* .\pslist.exe --help | -h : Display this help.

# Authors
* ChaosLayer
* [MikeHorn-git](https://github.com/MikeHorn-git)
