#ifndef BACKENDS_P4FMT_OPTIONS_H_
#define BACKENDS_P4FMT_OPTIONS_H_

#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"

namespace p4c::P4Fmt {

class P4fmtOptions : public CompilerOptions {
 public:
    P4fmtOptions();
    virtual ~P4fmtOptions() = default;
    P4fmtOptions(const P4fmtOptions &) = default;
    P4fmtOptions(P4fmtOptions &&) = delete;
    P4fmtOptions &operator=(const P4fmtOptions &) = default;
    P4fmtOptions &operator=(P4fmtOptions &&) = delete;

    const std::filesystem::path &outputFile() const;

 private:
    /// File to output to.
    std::filesystem::path outFile;
};

using P4FmtContext = P4CContextWithOptions<P4fmtOptions>;

}  // namespace p4c::P4Fmt

#endif /* BACKENDS_P4FMT_OPTIONS_H_ */
