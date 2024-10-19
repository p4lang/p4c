/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/**
 *  Collect pipelines form program.
 */
#include "collect_pipelines.h"

#include "bf-p4c/device.h"

namespace BFN {

template <typename T>
static bool ptrNameEq(T *a, T *b) {
    return a == b || (a && b && a->getName() == b->getName());
}

bool CollectPipelines::FullGress::operator==(const FullGress &other) const {
    return ptrNameEq(parser, other.parser) && ptrNameEq(control, other.control) &&
           ptrNameEq(deparser, other.deparser);
}

template <typename Type>
static void _setPipe(CollectPipelines::Pipe *self, const IR::IDeclaration *dec,
                     CollectPipelines::FullGress CollectPipelines::Pipe::*gress,
                     Type *CollectPipelines::FullGress::*gressField, cstring type) {
    auto val = dec->to<Type>();
    BUG_CHECK(val, "invalid type of %1%, expecting %2%", dec, type);
    (self->*gress).*gressField = val;
}

void CollectPipelines::Pipe::set(unsigned count, unsigned idx, const IR::IDeclaration *dec) {
    using Gress = CollectPipelines::FullGress;

    BUG_CHECK(idx < count, "Pipe argument %1% out of range", idx);

    BUG_CHECK(count == 6u || count == 7u, "Cannot process pipelines with %1% arguments", count);
    if (idx == 6) {
        ghost = dec->to<IR::BFN::TnaControl>();
        BUG_CHECK(ghost, "invalid type of %1%, expecting control", dec);
    } else {
        auto gress = idx < 3 ? &Pipe::ingress : &Pipe::egress;
        idx %= 3;
        switch (idx) {
            case 0:
                _setPipe(this, dec, gress, &Gress::parser, "parser"_cs);
                break;
            case 1:
                _setPipe(this, dec, gress, &Gress::control, "control"_cs);
                break;
            case 2:
                _setPipe(this, dec, gress, &Gress::deparser, "deparser"_cs);
                break;
        }
    }
}

bool CollectPipelines::Pipe::operator==(const Pipe &other) const {
    return ptrNameEq(dec, other.dec);
}

bool CollectPipelines::Pipe::operator<(const Pipe &other) const {
    return (dec && other.dec && std::strcmp(dec->getName().name, other.dec->getName().name) < 0) ||
           dec < other.dec;
}

// Checks the "main"
bool CollectPipelines::preorder(const IR::Declaration_Instance *di) {
    // Check if this is specialized type with baseType referring to "Switch" or
    // "Pipeline"
    auto type = di->type->to<IR::Type_Specialized>();
    if (!type || !type->baseType->is<IR::Type_Name>()) return false;

    auto name = type->baseType->to<IR::Type_Name>()->path->name;
    if (name == "Switch") {
        for (auto argument : *di->arguments) {
            auto pathExp = argument->expression->to<IR::PathExpression>();
            BUG_CHECK(pathExp && pathExp->type->is<IR::Type_Package>(),
                      "pipe expression %1% has unexpected type", argument->expression);

            auto pipe = pipesByDec.at(refMap->getDeclaration(pathExp->path));
            pipelines->pipes.push_back(pipe);
            const IR::Type_Declaration *decs[]{
                pipe.ingress.parser, pipe.ingress.control, pipe.ingress.deparser,
                pipe.egress.parser,  pipe.egress.control,  pipe.egress.deparser,
                pipe.ghost};
            for (auto dec : decs) {
                if (dec) pipelines->declarationToPipe[dec].insert(pipelines->pipes.size() - 1);
            }
        }
        LOG6("[CollectPipelines] Found switch with " << di->arguments->size() << " pipes");
    }

    if (name == "Pipeline") {
        unsigned i = 0;
        Pipe pipe;
        for (auto *argument : *di->arguments) {
            auto *call = argument->expression->to<IR::ConstructorCallExpression>();
            BUG_CHECK(call, "Cannot handle pipelines with non-call expression %1% (in %2%)",
                      argument->expression, di);
            BUG_CHECK(call->constructedType, "Cannot get the callee in expression %1% (in %2%)",
                      call, di);
            auto *type = call->constructedType->to<IR::Type_Name>();
            BUG_CHECK(type, "Cannot extract type from type name in expression %1% (in %2%)", call,
                      di);
            auto *dec = refMap->getDeclaration(type->path);
            pipe.set(di->arguments->size(), i, dec);
            ++i;
        }
        pipe.dec = di;
        pipesByDec[di] = pipe;
        LOG6("[CollectPipelines] Found pipeline " << di->getName() << " "
                                                  << (pipe.ghost ? "with ghost" : "without ghost"));
    }
    return false;
}

}  // namespace BFN
