# Requirements
* [Msbuild](https://visualstudio.microsoft.com/downloads/)

# Compilation
```batch
cl /EHsc pslist.c
```

# Usage
* .\pslist.exe -d <processId> : View threads of the specified process ID..
* .\pslist.exe <processName> : View processes with the specified name.
* .\pslist.exe -t : Display threads for listed processes.
* .\pslist.exe --help | -h : Display this help.

# Authors
* [Phobetor](https://github.com/Phobetore)
* [MikeHorn-git](https://github.com/MikeHorn-git)
