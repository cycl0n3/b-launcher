#include "ChildCommon.h"

void WorkingCodeA() {
    std::cout << "[A.exe] Sandboxed logic executing!" << std::endl;
    for (int i = 1; i <= 3; ++i) {
        std::cout << "[A.exe] Processing step " << i << "..." << std::endl;
        Sleep(500);
    }
}

int main(int argc, char* argv[]) {
    ExecuteChildLogic(argc, argv, WorkingCodeA, "A.exe");
    return 0;
}