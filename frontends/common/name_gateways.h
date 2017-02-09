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

#ifndef FRONTENDS_COMMON_NAME_GATEWAYS_H_
#define FRONTENDS_COMMON_NAME_GATEWAYS_H_

#include "ir/ir.h"

class NameGateways : public Transform {
    const IR::Node *preorder(IR::If *n) override {
        return new IR::NamedCond(*n);
    }
    const IR::Node *preorder(IR::NamedCond *n) override {
        return n;
    }
};

#endif  /* FRONTENDS_COMMON_NAME_GATEWAYS_H_ */
