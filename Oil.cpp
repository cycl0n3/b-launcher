// Oil.cpp
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <set>
#include <mutex>

#define PIPE_NAME L"\\\\.\\pipe\\OilConsolePipe"
#define GUARD_PIPE_NAME L"\\\\.\\pipe\\OilSingleInstanceGuard"

std::set<std::wstring> activeChildren;
std::mutex stateMutex;

std::wstring GetExeDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring pathStr(path);
    size_t pos = pathStr.find_last_of(L"\\/");
    return (pos != std::wstring::npos) ? pathStr.substr(0, pos + 1) : L"";
}

// Named Pipe Single-Instance Check for Oil.exe
HANDLE EnsureSingleOilInstance() {
    // Try creating the guard named pipe
    HANDLE hGuardPipe = CreateNamedPipeW(
        GUARD_PIPE_NAME,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE, // Ensures this is the FIRST/ONLY instance
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, 512, 512, 0, NULL
    );

    // If pipe creation failed or instance already exists, another Oil.exe is running
    if (hGuardPipe == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    return hGuardPipe;
}

void EnsureConsoleVisible() {
    if (AllocConsole()) {
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONERR$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        
        std::ios::sync_with_stdio();
        std::setvbuf(stdout, NULL, _IONBF, 0);
    }
    
    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
    }
}

HANDLE CreateSandboxJob() {
    HANDLE hJob = CreateJobObjectW(NULL, NULL);
    if (!hJob) return NULL;

    JOBOBJECT_BASIC_UI_RESTRICTIONS uiLimits = {0};
    uiLimits.UIRestrictionsClass = JOB_OBJECT_UILIMIT_HANDLES |
                                 JOB_OBJECT_UILIMIT_READCLIPBOARD |
                                 JOB_OBJECT_UILIMIT_WRITECLIPBOARD |
                                 JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS;

    SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &uiLimits, sizeof(uiLimits));
    return hJob;
}

void ReadChildOutput(HANDLE hPipe) {
    char buffer[512];
    DWORD bytesRead;

    while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        if (bytesRead == 0) break;
        buffer[bytesRead] = '\0';
        std::cout << buffer;
        std::cout.flush();
    }
    CloseHandle(hPipe);
}

void MonitorChildLifetime(HANDLE hProcess, std::wstring exeName) {
    WaitForSingleObject(hProcess, INFINITE);
    
    std::lock_guard<std::mutex> lock(stateMutex);
    activeChildren.erase(exeName);
    std::cout << "[Oil.exe] Child process " << std::string(exeName.begin(), exeName.end()) 
              << " exited. Removed from active tracking." << std::endl;

    CloseHandle(hProcess);
}

void SpawnSandboxedChild(const std::wstring& exeName, HANDLE hJob) {
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (activeChildren.find(exeName) != activeChildren.end()) {
            std::cout << "[Oil.exe] REJECTED: " << std::string(exeName.begin(), exeName.end()) 
                      << " is already running under Oil.exe." << std::endl;
            return;
        }
        activeChildren.insert(exeName);
    }

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        std::lock_guard<std::mutex> lock(stateMutex);
        activeChildren.erase(exeName);
        return;
    }

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(si) };
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError  = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    
    std::wstring fullPath = GetExeDirectory() + exeName;
    std::wstring cmd = L"\"" + fullPath + L"\" --child";

    BOOL success = CreateProcessW(
        fullPath.c_str(), &cmd[0], NULL, NULL, TRUE,
        CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, NULL, &si, &pi
    );

    CloseHandle(hWritePipe);

    if (success) {
        AssignProcessToJobObject(hJob, pi.hProcess);
        ResumeThread(pi.hThread);

        CloseHandle(pi.hThread);

        std::cout << "[Oil.exe] Successfully spawned sandboxed child: " 
                  << std::string(exeName.begin(), exeName.end()) << std::endl;

        std::thread(ReadChildOutput, hReadPipe).detach();
        std::thread(MonitorChildLifetime, pi.hProcess, exeName).detach();
    } else {
        std::cout << "[Oil.exe] Failed to launch child: " << std::string(exeName.begin(), exeName.end()) 
                  << " (Error Code: " << GetLastError() << ")" << std::endl;
        CloseHandle(hReadPipe);

        std::lock_guard<std::mutex> lock(stateMutex);
        activeChildren.erase(exeName);
    }
}

int main(int argc, char* argv[]) {
    // 1. Check named pipe to ensure no other Oil.exe instance exists
    HANDLE hGuardPipe = EnsureSingleOilInstance();
    if (!hGuardPipe) {
        // Exit silently — another Oil.exe is already active
        return 0;
    }

    EnsureConsoleVisible();

    SetConsoleTitleW(L"Oil.exe Orchestrator Console");

    std::cout << "========================================" << std::endl;
    std::cout << "   [Oil.exe] Master Console Started     " << std::endl;
    std::cout << "========================================" << std::endl;

    HANDLE hSandboxJob = CreateSandboxJob();

    if (argc > 1) {
        wchar_t wExe[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, argv[1], -1, wExe, MAX_PATH);
        SpawnSandboxedChild(wExe, hSandboxJob);
    }

    while (true) {
        HANDLE hPipe = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            512, 512, 0, NULL
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                char buffer[128] = {0};
                DWORD bytesRead;
                if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
                    std::string req(buffer);
                    if (req.rfind("SPAWN:", 0) == 0) {
                        std::string exeStr = req.substr(6);
                        std::wstring wExe(exeStr.begin(), exeStr.end());
                        
                        SpawnSandboxedChild(wExe, hSandboxJob);
                    }
                }
                CloseHandle(hPipe);
            }
        }
    }

    CloseHandle(hSandboxJob);
    CloseHandle(hGuardPipe);
    return 0;
}