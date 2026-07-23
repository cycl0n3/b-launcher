#include "ChildCommon.h"

void WorkingCodeC() {
    std::cout << "[C.exe] Sandboxed logic executing!" << std::endl;
    for (int i = 1; i <= 3; ++i) {
        std::cout << "[C.exe] Computing cycle " << i << "..." << std::endl;
        Sleep(400);
    }
}

int main(int argc, char* argv[]) {
    ExecuteChildLogic(argc, argv, WorkingCodeC, "C.exe");
    return 0;
}