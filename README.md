# Requirements
* [Msbuild](https://visualstudio.microsoft.com/downloads/)


# Compilation
The source file is a standalone C file. Compile with cl.exe
Automatic build can be used with the compile.bat script if executed in the Developer Command Prompt.  
Or you can compile it by doing:
```bash
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
