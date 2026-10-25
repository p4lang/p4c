// Copyright 2022 VMware, Inc.
// SPDX-FileCopyrightText: 2022 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "ternaryBool.h"

#include "lib/cstring.h"

namespace P4 {

using namespace literals;

cstring toString(const TernaryBool &c) {
    switch (c) {
        case TernaryBool::Yes:
            return "Yes"_cs;
        case TernaryBool::No:
            return "No"_cs;
            ;
        case TernaryBool::Maybe:
            return "Maybe"_cs;
    }
    return cstring::empty;
}

}  // namespace P4
