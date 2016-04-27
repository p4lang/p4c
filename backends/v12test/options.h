#ifndef _BACKENDS_V12TEST_OPTIONS_H_
#define _BACKENDS_V12TEST_OPTIONS_H_

#include "frontends/common/options.h"

namespace V12Test {

class V12TestOptions : public CompilerOptions {
 public:
    V12TestOptions();
    const char* top4;  // regular expression matching pass names
};

}  // namespace V12Test

#endif /* _BACKENDS_V12TEST_OPTIONS_H_ */
