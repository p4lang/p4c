#include "ir/configuration.h"
#include "lib/gmputil.h"
#include "constantParsing.h"

IR::Constant* cvtCst(Util::SourceInfo srcInfo, const char* s, unsigned skip, unsigned base) {
    char *sep;
    auto size = strtol(s, &sep, 10);
    if (sep == nullptr || !*sep)
       BUG("Expected to find separator %1%", s);
    if (size <= 0) {
        ::error("%1%: Non-positive size %2%", srcInfo, size);
        return nullptr; }
    if (size > P4CConfiguration::MaximumWidthSupported) {
        ::error("%1%: %2% size too large", srcInfo, size);
        return nullptr; }

    bool isSigned = *sep == 's';
    mpz_class value = Util::cvtInt(sep+skip+1, base);
    const IR::Type* type = IR::Type_Bits::get(srcInfo, size, isSigned);
    IR::Constant* result = new IR::Constant(srcInfo, type, value, base);
    return result;
}
