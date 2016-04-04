#ifndef _IR_DBPRINT_H_
#define _IR_DBPRINT_H_

#include <assert.h>
#include "lib/log.h"  // to get operator<< templates that call dbprint methods
#include "lib/indent.h"

namespace DBPrint {

enum dbprint_flags {
    Precedence = 0xf,
    Prec_Postfix = 15,
    Prec_Prefix = 14,
    Prec_Mul = 13, Prec_Div = 13, Prec_Mod = 13,
    Prec_Add = 12, Prec_Sub = 12,
    Prec_Shl = 11, Prec_Shr = 11,
    Prec_Lss = 10, Prec_Leq = 10, Prec_Grt = 10, Prec_Geq = 10,
    Prec_Equ = 9, Prec_Neq = 9,
    Prec_BAnd = 8,
    Prec_BXor = 7,
    Prec_BOr = 6,
    Prec_LAnd = 5,
    Prec_LOr = 4,
    Prec_Cond = 3,
    Prec_Low = 1,

    TableNoActions = 0x10,
};

int dbgetflags(std::ostream &out);
int dbsetflags(std::ostream &out, int val, int mask);

inline int getprec(std::ostream &out) { return dbgetflags(out) & DBPrint::Precedence; }
class setflags_helper {
    int val, mask;
    setflags_helper() = delete;
 protected:
    setflags_helper(int v, int m) : val(v), mask(m) { assert((val & ~mask) == 0); }
 public:
    void set(std::ostream &out) const { dbsetflags(out, val, mask); }
};
struct setprec : public setflags_helper {
    explicit setprec(int prec) : setflags_helper(prec, DBPrint::Precedence) {}
};
struct setflag : public setflags_helper {
    explicit setflag(int fl) : setflags_helper(fl, fl) {}
};
struct clrflag : public setflags_helper {
    explicit clrflag(int fl) : setflags_helper(0, ~fl) {}
};


}  // end namespace DBPrint

inline std::ostream &operator<<(std::ostream &out, const DBPrint::setflags_helper &p) {
    p.set(out); return out; }

#endif /* _IR_DBPRINT_H_ */
