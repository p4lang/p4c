#ifndef _p4_parse_h_
#define _p4_parse_h_

#include <memory>
#include "lib/source_file.h"
#include "frontends/common/options.h"

namespace IR { class V1Program; }

const IR::V1Program *parse_p4v1_file(const CompilerOptions &, FILE *in);

#endif /* _p4_parse_h_ */
