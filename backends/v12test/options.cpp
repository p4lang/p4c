#include "options.h"

namespace V12Test {

V12TestOptions::V12TestOptions() : top4(nullptr) {
    registerOption("--top4", "regex",
                   [this](const char* arg) { top4 = arg; return true; },
                   "[Compiler debugging] Dump the P4 representation after\n"
                   "passes matching regex");
}

}  // namespace V12Test
