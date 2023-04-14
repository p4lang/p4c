#include "lib/crash.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/testgen.h"

int main(int argc, char **argv) {
    setup_signals();

    std::vector<const char *> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        P4Tools::P4Testgen::Testgen().main(args);
    } catch (const Util::CompilerBug &e) {
        std::cerr << "Internal error: " << e.what() << std::endl;
        std::cerr << "Please submit a bug report with your code." << std::endl;
        return 1;
    } catch (const Util::CompilerUnimplemented &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const Util::CompilationError &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << std::endl;
        std::cerr << "Please submit a bug report with your code." << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Internal error. Please submit a bug report with your code." << std::endl;
        return 1;
    }
}
