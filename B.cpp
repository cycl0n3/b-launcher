#include "ChildCommon.h"

void WorkingCodeB() {
    std::cout << "[B.exe] Sandboxed logic executing!" << std::endl;
    for (int i = 1; i <= 3; ++i) {
        std::cout << "[B.exe] Working on task " << i << "..." << std::endl;
        Sleep(600);
    }
}

int main(int argc, char* argv[]) {
    ExecuteChildLogic(argc, argv, WorkingCodeB, "B.exe");
    return 0;
}