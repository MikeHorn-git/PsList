#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <psapi.h>
#include <winbase.h>

// Function declarations
BOOL GetProcessList(const char* processName, DWORD processId, BOOL showThreads);
void PrintHeader();
void PrintProcessInfo(PROCESSENTRY32* pe32, char* processName, char* CrTime, DWORD priorityClass, char* commandLine, char* RealCpuTime, BOOL showThreads);
SIZE_T GetVirtualMemoryUsage(HANDLE hProcess);
BOOL GetCreationTime(HANDLE hProcess, SYSTEMTIME* systemTime);
BOOL GetCommand(HANDLE hProcess, TCHAR processCmdLine[MAX_PATH]);
void ListProcessThreads(DWORD dwOwnerPID);

SIZE_T GetVirtualMemoryUsage(HANDLE hProcess) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        return pmc.PagefileUsage / 1024 / 1024; // Return size in MB
    }
    else {
        printf("Error getting memory info.\n");
        return 0;
    }
}

// Get the process creation time
BOOL GetCreationTime(HANDLE hProcess, SYSTEMTIME* systemTime) {
    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
        FileTimeToSystemTime(&creationTime, systemTime);
        return TRUE;
    }
    return FALSE;
}

// Get the process launch command
BOOL GetCommand(HANDLE hProcess, TCHAR processCmdLine[MAX_PATH]) {
    GetProcessImageFileName(hProcess, processCmdLine, MAX_PATH);
    return TRUE;
}

// Function to obtain the list of processes
BOOL GetProcessList(const char* processName, DWORD processId, BOOL showThreads) {

    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    DWORD handleCnt;
    PROCESS_MEMORY_COUNTERS pmc;
    SYSTEMTIME creationTime;
    TCHAR processCmdLine[MAX_PATH];
    char CrTime[100];
    char RealCpuTime[100]; // Modification: CrTime should be a string

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("CreateToolhelp32Snapshot FAIL");
        return FALSE;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        printf("Error Process32First");
        CloseHandle(hProcessSnap);
        return FALSE;
    }

    PrintHeader();

    do {
        // If a process is specified and does not match, skip to the next one
        if (processId != 0 && pe32.th32ProcessID != processId) {
            continue;
        }

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

        // ---------- Identify priority class ----------
        DWORD dwPriorityClass = GetPriorityClass(hProcess);

        GetCommand(hProcess, processCmdLine); // Get process launch command

        // ---------- CPU Time ----------
        FILETIME createTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
            // Convert FILETIME to relative time (in 100 nanoseconds)
            ULONGLONG cpuTime = ((ULONGLONG)kernelTime.dwHighDateTime << 32 | kernelTime.dwLowDateTime) +
                                    ((ULONGLONG)userTime.dwHighDateTime << 32 | userTime.dwLowDateTime);

            // Convert time to milliseconds
            cpuTime /= 10000;
            // Calculate hours, minutes, seconds, and milliseconds
            int hours = cpuTime / 3600000;
            int minutes = (cpuTime % 3600000) / 60000;
            int seconds = (cpuTime % 60000) / 1000;
            int milliseconds = cpuTime % 1000;
            snprintf(RealCpuTime, sizeof(RealCpuTime), "%02I64u:%02I64u:%02I64u.%03I64u", (ULONGLONG)hours, (ULONGLONG)minutes, (ULONGLONG)seconds, (ULONGLONG)milliseconds);
        }

        // ---------- Creation Time ----------
        if (GetCreationTime(hProcess, &creationTime)) {
            snprintf(CrTime, sizeof(CrTime), "%02d/%02d/%04d %02d:%02d:%02d\n",
                    creationTime.wDay, creationTime.wMonth, creationTime.wYear,
                    creationTime.wHour, creationTime.wMinute, creationTime.wSecond); // Modification: Use snprintf for CrTime
                    // Remove newline characters from CrTime
                    size_t len = strlen(CrTime);
                    for (size_t i = 0; i < len; i++) {
                        if (CrTime[i] == '\n' || CrTime[i] == '\r') {
                            CrTime[i] = ' ';
                        }
                    }

        }

        PrintProcessInfo(&pe32, processName, CrTime, dwPriorityClass, processCmdLine, RealCpuTime, showThreads);

        CloseHandle(hProcess);
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return TRUE;
}

// Function to print the header of the process list
void PrintHeader() {
    printf("Process information for this machine:\n\n");
    printf("%-40s%-8s%-5s%-5s%-8s%-8s%-21s%-15s%-21s\n", "Name", "PID", "Pri", "Thd", "Hnd", "Priv", "Creation Time", "Memory Usage", "CPU Time");
}

// Function to print specific information of a process
void PrintProcessInfo(PROCESSENTRY32* pe32, char* processName, char* CrTime, DWORD priorityClass, char* commandLine, char* RealCpuTime, BOOL showThreads) {
    char nameWithoutExe[260];
    char* dotExe = strstr(pe32->szExeFile, ".exe");

    if (dotExe != NULL) {
        strncpy(nameWithoutExe, pe32->szExeFile, dotExe - pe32->szExeFile);
        nameWithoutExe[dotExe - pe32->szExeFile] = '\0';
    } else {
        strcpy(nameWithoutExe, pe32->szExeFile);
    }

    // Check if the process name matches the searched process (or if no name is specified)
    if (processName == NULL || strcmp(processName, nameWithoutExe) == 0) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32->th32ProcessID);
        if (hProcess != NULL) {
            DWORD handleCount;
            if (GetProcessHandleCount(hProcess, &handleCount)) {
                SIZE_T virtualMemUsage = GetVirtualMemoryUsage(hProcess);
                printf("%-40s%-8lu%-5lu%-5lu%-8lu%-8lu%-21s%-9IuMo    %-21s\n", nameWithoutExe, pe32->th32ProcessID, pe32->pcPriClassBase, pe32->cntThreads, handleCount, priorityClass, CrTime, virtualMemUsage, RealCpuTime);
                if (showThreads) {
                    ListProcessThreads(pe32->th32ProcessID); // Show threads
                }
            }
            CloseHandle(hProcess);
        }
        else {
            printf("%-40s%-8lu%-5lu%-5lu%-8s%-8lu%-21s%-9s      %-21s\n", nameWithoutExe, pe32->th32ProcessID, pe32->pcPriClassBase, pe32->cntThreads, "N/A", priorityClass, "N/A", "N/A", "N/A");
        }
    }
}

// Function to list the threads of a specified process
void ListProcessThreads(DWORD dwOwnerPID) {
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    // Obtain a snapshot of the system's threads
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return;

    // Fill the THREADENTRY32 structure for iteration
    te32.dwSize = sizeof(THREADENTRY32);

    // Retrieve information about the first thread
    if (!Thread32First(hThreadSnap, &te32)) {
        CloseHandle(hThreadSnap);
        return;
    }

    // Iterate through the threads
    do {
        // If the -t option is specified, display the threads of the specified process
        if (te32.th32OwnerProcessID == dwOwnerPID) {
            // Display thread information
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID);
            if (hThread != NULL) {
                CONTEXT context;
                context.ContextFlags = CONTEXT_FULL;
                if (GetThreadContext(hThread, &context)) {
                    char state[20];
#ifdef _M_IX86
                    if (context.EFlags & 0x100) {
                        strcpy(state, "Suspended");
                    }
                    else {
                        DWORD exitCode;
                        if (GetExitCodeThread(hThread, &exitCode)) {
                            if (exitCode == STILL_ACTIVE) {
                                strcpy(state, "Active");
                            }
                            else {
                                strcpy(state, "Terminated");
                            }
                        }
                        else {
                            strcpy(state, "Unknown");
                        }
                    }
                    printf("Thread ID: %lu, State: %s, Entry point: %p\n", te32.th32ThreadID, state, (void*)context.Eip);
#elif defined(_M_AMD64)
                    if (context.ContextFlags & CONTEXT_AMD64) {
                        DWORD exitCode;
                        if (GetExitCodeThread(hThread, &exitCode)) {
                            if (exitCode == STILL_ACTIVE) {
                                strcpy(state, "Active");
                            }
                            else {
                                strcpy(state, "Terminated");
                            }
                        }
                        else {
                            strcpy(state, "Unknown");
                        }
                        printf("Thread ID: %lu, State: %s, Entry point: %p\n", te32.th32ThreadID, state, (void*)context.Rip);
                    }
                    else {
                        strcpy(state, "Suspended");
                        printf("Thread ID: %lu, State: %s, Entry point: %p\n", te32.th32ThreadID, state, (void*)context.Rip);
                    }
#endif
                }
                CloseHandle(hThread);
            }
        }
    } while (Thread32Next(hThreadSnap, &te32));

    // Don't forget to close the handle to release resources
    CloseHandle(hThreadSnap);
}

// Main function
int main(int argc, char* argv[]) {
    printf("\nPsList - Yet another Process viewer. Inspired by sysinternals. \nMade By ChaosLayer and MikeHorn-git\n\n");

    const char* processName = NULL;
    DWORD processId = 0;
    BOOL showThreads = FALSE;

    // Iterate through the command-line arguments
    for (int i = 1; i < argc; i++) {
        // Check the -t option to display threads
        if (strcmp(argv[i], "-t") == 0) {
            showThreads = TRUE;
        }
        // Check the -d option with the process ID
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            processId = atoi(argv[i + 1]);
            i++; // Skip to the next argument
        }
        // Check the help options -h and --help
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            // Display help
            printf("Usage:\n");
            printf("  %s -d <processId> : View threads of the specified process ID.\n", argv[0]);
            printf("  %s <processName> : View processes with the specified name.\n", argv[0]);
            printf("  %s -t : Display threads for listed processes.\n", argv[0]);
            printf("  %s --help | -h : Display this help.\n", argv[0]);
            printf("(WARNING: some system process information may not display)\n");
            return 0;
        }
        // If none of the above cases, the argument is considered the process name
        else {
            processName = argv[i];
        }
    }

    // Call GetProcessList with the parsed options
    GetProcessList(processName, processId, showThreads);

    return 0;
}

