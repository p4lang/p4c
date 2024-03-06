#include "backends/p4tools/modules/smith/smith.h"
#include "lib/crash.h"
#include "lib/exceptions.h"

int main(int argc, char **argv) {
    setup_signals();

    std::vector<const char *> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        P4Tools::P4Smith::Smith().main(args);
    } catch (const Util::CompilerBug &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        std::cerr << "Please submit a bug report with your code.\n";
        return 1;
    } catch (const Util::CompilerUnimplemented &e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const Util::CompilationError &e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        std::cerr << "Please submit a bug report with your code.\n";
        return 1;
    } catch (...) {
        std::cerr << "Internal error. Please submit a bug report with your code.\n";
        return 1;
    }
}
