#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define USE_CLANG_INSTEAD

#define SRC_DIR "src/"
#ifdef _WIN32
#define PROG_NAME "pdfutils.exe"
#else
#define PROG_NAME "pdfutils"
#endif

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};
#ifdef _WIN32
#ifndef USE_CLANG_INSTEAD
#error "`cefer.h` does not support for msvc"
#else
    cmd_append(&cmd, "clang", "-std=c11");
    cmd_append(&cmd, "-Wall", "-Wextra", "-Wpedantic", "-Wno-unused-parameter");
    cmd_append(&cmd, SRC_DIR"main.c");
    cmd_append(&cmd, "-o", PROG_NAME);
    cmd_append(&cmd, "-I", "C:/Users/almag/.local/mupdf/include");
    cmd_append(&cmd, "-L", "C:/Users/almag/.local/mupdf/platform/win32/x64/Release");
    cmd_append(&cmd, "-llibmupdf", "-llibthirdparty", "-lmsvcrt");
#endif
#endif
    if (!cmd_run(&cmd)) return 1;

    return 0;
}
