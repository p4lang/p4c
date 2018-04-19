/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _IR_DBPRINT_H_
#define _IR_DBPRINT_H_

#include <assert.h>
#include "lib/log.h"  // to get operator<< templates that call dbprint methods
#include "lib/indent.h"

namespace DBPrint {

// Support for debug print, needed by the LOG* macros.
// TODO: These should be part of a def file
enum dbprint_flags {
    Precedence = 0xf,
    Prec_Postfix = 15,
    Prec_Prefix = 14,
    Prec_Mul = 13, Prec_Div = 13, Prec_Mod = 13,
    Prec_Add = 12, Prec_Sub = 12,
    Prec_AddSat = 12, Prec_SubSat = 12,
    Prec_Shl = 11, Prec_Shr = 11,
    Prec_BAnd = 10,
    Prec_BXor = 9,
    Prec_BOr = 8,
    Prec_Lss = 7, Prec_Leq = 7, Prec_Grt = 7, Prec_Geq = 7,
    Prec_Equ = 6, Prec_Neq = 6,
    Prec_LAnd = 5,
    Prec_LOr = 4,
    Prec_Cond = 3,
    Prec_Low = 1,

    Reset = 0,
    TableNoActions = 0x10,
    Brief = 0x20,
};

int dbgetflags(std::ostream &out);
int dbsetflags(std::ostream &out, int val, int mask = ~0U);

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
    explicit clrflag(int fl) : setflags_helper(0, fl) {}
};


}  // end namespace DBPrint

inline std::ostream &operator<<(std::ostream &out, const DBPrint::setflags_helper &p) {
    p.set(out); return out; }

inline std::ostream &operator<<(std::ostream &out, const DBPrint::dbprint_flags fl) {
    DBPrint::dbsetflags(out, fl, fl ? fl : ~0);
    return out; }

#endif /* _IR_DBPRINT_H_ */
