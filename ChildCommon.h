// ChildCommon.h
#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

#define PIPE_NAME L"\\\\.\\pipe\\OilConsolePipe"

std::wstring GetExeDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring pathStr(path);
    size_t pos = pathStr.find_last_of(L"\\/");
    return (pos != std::wstring::npos) ? pathStr.substr(0, pos + 1) : L"";
}

bool IsOilRunning() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Oil.exe") == 0) {
                CloseHandle(hSnap);
                return true;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return false;
}

void RequestOilToSpawn(const std::string& exeName) {
    std::wstring exeDir = GetExeDirectory();
    std::wstring oilPath = exeDir + L"Oil.exe";

    if (!IsOilRunning()) {
        STARTUPINFOW si = { sizeof(si) };
        si.cb = sizeof(si);
        // FORCE WINDOW VISIBILITY
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;

        PROCESS_INFORMATION pi;
        std::wstring cmd = L"\"" + oilPath + L"\" " + std::wstring(exeName.begin(), exeName.end());

        BOOL created = CreateProcessW(
            oilPath.c_str(), 
            &cmd[0], 
            NULL, NULL, FALSE, 
            CREATE_NEW_CONSOLE, // Requests an independent console window
            NULL, NULL, &si, &pi
        );

        if (created) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return;
        } else {
            wchar_t err[256];
            swprintf_s(err, 256, L"Failed to start Oil.exe at:\n%s\nError code: %d", oilPath.c_str(), GetLastError());
            MessageBoxW(NULL, err, L"Launch Error", MB_ICONERROR);
            return;
        }
    }

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 10; ++i) {
        hPipe = CreateFileW(
            PIPE_NAME, GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, NULL
        );
        if (hPipe != INVALID_HANDLE_VALUE) break;
        Sleep(100);
    }

    if (hPipe != INVALID_HANDLE_VALUE) {
        std::string msg = "SPAWN:" + exeName;
        DWORD bytesWritten;
        WriteFile(hPipe, msg.c_str(), (DWORD)msg.length(), &bytesWritten, NULL);
        CloseHandle(hPipe);
    }
}

void ExecuteChildLogic(int argc, char* argv[], void (*workingCode)(), const std::string& exeName) {
    bool isSandboxedChild = (argc > 1 && std::string(argv[1]) == "--child");

    if (!isSandboxedChild) {
        RequestOilToSpawn(exeName);
        ExitProcess(0);
    } else {
        std::setvbuf(stdout, NULL, _IONBF, 0);
        std::cout << std::unitbuf;
        
        workingCode();
        
        std::cout << std::flush;
    }
}