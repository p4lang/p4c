/*
Copyright 2013-present Barefoot Networks, Inc.
Copyright 2022 VMware Inc.

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

#include "psaProgramStructure.h"

#include <algorithm>
#include <list>
#include <map>
#include <ostream>
#include <string>

#include "backends/bmv2/common/backend.h"
#include "ir/indexed_vector.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/stringify.h"

namespace BMV2 {

void InspectPsaProgram::postorder(const IR::Declaration_Instance *di) {
    if (!pinfo->resourceMap.count(di)) return;
    auto blk = pinfo->resourceMap.at(di);
    if (blk->is<IR::ExternBlock>()) {
        auto eb = blk->to<IR::ExternBlock>();
        LOG3("populate " << eb);
        pinfo->extern_instances.emplace(di->name, di);
    }
}

bool InspectPsaProgram::isHeaders(const IR::Type_StructLike *st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

void InspectPsaProgram::addHeaderType(const IR::Type_StructLike *st) {
    LOG5("In addHeaderType with struct " << st->toString());
    if (st->is<IR::Type_HeaderUnion>()) {
        LOG5("Struct is Type_HeaderUnion");
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            addHeaderType(ht);
        }
        pinfo->header_union_types.emplace(st->getName(), st->to<IR::Type_HeaderUnion>());
        return;
    } else if (st->is<IR::Type_Header>()) {
        LOG5("Struct is Type_Header");
        pinfo->header_types.emplace(st->getName(), st->to<IR::Type_Header>());
    } else if (st->is<IR::Type_Struct>()) {
        LOG5("Struct is Type_Struct");
        pinfo->metadata_types.emplace(st->getName(), st->to<IR::Type_Struct>());
    }
}

void InspectPsaProgram::addHeaderInstance(const IR::Type_StructLike *st, cstring name) {
    auto inst = new IR::Declaration_Variable(name, st);
    if (st->is<IR::Type_Header>())
        pinfo->headers.emplace(name, inst);
    else if (st->is<IR::Type_Struct>())
        pinfo->metadata.emplace(name, inst);
    else if (st->is<IR::Type_HeaderUnion>())
        pinfo->header_unions.emplace(name, inst);
}

void InspectPsaProgram::addTypesAndInstances(const IR::Type_StructLike *type, bool isHeader) {
    LOG5("Adding type " << type->toString() << " and isHeader " << isHeader);
    for (auto f : type->fields) {
        LOG5("Iterating through field " << f->toString());
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (isHeader && ft->is<IR::Type_Struct>()) {
                ::error(ErrorType::ERR_INVALID,
                        "Type %1% should only contain headers, header stacks, or header unions",
                        type);
                return;
            }
            auto st = ft->to<IR::Type_StructLike>();
            addHeaderType(st);
        }
    }

    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            if (auto hft = ft->to<IR::Type_Header>()) {
                LOG5("Field is Type_Header");
                addHeaderInstance(hft, f->controlPlaneName());
            } else if (ft->is<IR::Type_HeaderUnion>()) {
                LOG5("Field is Type_HeaderUnion");
                for (auto uf : ft->to<IR::Type_HeaderUnion>()->fields) {
                    auto uft = typeMap->getType(uf, true);
                    if (auto h_type = uft->to<IR::Type_Header>()) {
                        addHeaderInstance(h_type, uf->controlPlaneName());
                    } else {
                        ::error(ErrorType::ERR_INVALID, "Type %1% cannot contain type %2%", ft,
                                uft);
                        return;
                    }
                }
                pinfo->header_union_types.emplace(type->getName(),
                                                  type->to<IR::Type_HeaderUnion>());
                addHeaderInstance(type, f->controlPlaneName());
            } else {
                LOG5("Adding struct with type " << type);
                pinfo->metadata_types.emplace(type->getName(), type->to<IR::Type_Struct>());
                addHeaderInstance(type, f->controlPlaneName());
            }
        } else if (ft->is<IR::Type_Stack>()) {
            LOG5("Field is Type_Stack " << ft->toString());
            auto stack = ft->to<IR::Type_Stack>();
            // auto stack_name = f->controlPlaneName();
            auto stack_size = stack->getSize();
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            auto stack_type = stack->elementType->to<IR::Type_Header>();
            std::vector<unsigned> ids;
            for (unsigned i = 0; i < stack_size; i++) {
                cstring hdrName = f->controlPlaneName() + "[" + Util::toString(i) + "]";
                /* TODO */
                // auto id = json->add_header(stack_type, hdrName);
                addHeaderInstance(stack_type, hdrName);
                // ids.push_back(id);
            }
            // addHeaderStackInstance();
        } else {
            // Treat this field like a scalar local variable
            cstring newName = refMap->newName(type->getName() + "." + f->name);
            LOG5("newname for scalarMetadataFields is " << newName);
            if (ft->is<IR::Type_Bits>()) {
                LOG5("Field is a Type_Bits");
                auto tb = ft->to<IR::Type_Bits>();
                pinfo->scalars_width += tb->size;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                LOG5("Field is a Type_Boolean");
                pinfo->scalars_width += 1;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Error>()) {
                LOG5("Field is a Type_Error");
                pinfo->scalars_width += 32;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

bool InspectPsaProgram::preorder(const IR::Declaration_Variable *dv) {
    auto ft = typeMap->getType(dv->getNode(), true);
    cstring scalarsName = refMap->newName("scalars");

    if (ft->is<IR::Type_Bits>()) {
        LOG5("Adding " << dv << " into scalars map");
        pinfo->scalars.emplace(scalarsName, dv);
    } else if (ft->is<IR::Type_Boolean>()) {
        LOG5("Adding " << dv << " into scalars map");
        pinfo->scalars.emplace(scalarsName, dv);
    }

    return false;
}

// This visitor only visits the parameter in the statement from architecture.
bool InspectPsaProgram::preorder(const IR::Parameter *param) {
    auto ft = typeMap->getType(param->getNode(), true);
    LOG3("add param " << ft);
    // only convert parameters that are IR::Type_StructLike
    if (!ft->is<IR::Type_StructLike>()) {
        return false;
    }
    auto st = ft->to<IR::Type_StructLike>();
    // check if it is psa specific standard metadata
    cstring ptName = param->type->toString();
    // parameter must be a type that we have not seen before
    if (pinfo->hasVisited(st)) {
        LOG5("Parameter is visited, returning");
        return false;
    }
    if (PsaProgramStructure::isStandardMetadata(ptName)) {
        LOG5("Parameter is psa standard metadata");
        addHeaderType(st);
        // remove _t from type name
        cstring headerName = ptName.exceptLast(2);
        addHeaderInstance(st, headerName);
    }
    auto isHeader = isHeaders(st);
    addTypesAndInstances(st, isHeader);
    return false;
}

void InspectPsaProgram::postorder(const IR::P4Parser *p) {
    if (pinfo->block_type.count(p)) {
        auto info = pinfo->block_type.at(p);
        if (info.first == INGRESS && info.second == PARSER)
            pinfo->parsers.emplace("ingress", p);
        else if (info.first == EGRESS && info.second == PARSER)
            pinfo->parsers.emplace("egress", p);
    }
}

void InspectPsaProgram::postorder(const IR::P4Control *c) {
    if (pinfo->block_type.count(c)) {
        auto info = pinfo->block_type.at(c);
        if (info.first == INGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("egress", c);
        else if (info.first == INGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("egress", c);
    }
}

bool ParsePsaArchitecture::preorder(const IR::ToplevelBlock *block) {
    /// Blocks are not in IR tree, use a custom visitor to traverse
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) visit(it.second->getNode());
    }
    return false;
}

bool ParsePsaArchitecture::preorder(const IR::ExternBlock *block) {
    if (block->node->is<IR::Declaration>()) structure->globals.push_back(block);
    return false;
}

bool ParsePsaArchitecture::preorder(const IR::PackageBlock *block) {
    auto pkg = block->findParameterValue("ingress");
    if (pkg == nullptr) {
        modelError("Package %1% has no parameter named 'ingress'", block);
        return false;
    }
    if (auto ingress = pkg->to<IR::PackageBlock>()) {
        auto p = ingress->findParameterValue("ip");
        if (p == nullptr) {
            modelError("'ingress' package %1% has no parameter named 'ip'", block);
            return false;
        }
        auto parser = p->to<IR::ParserBlock>();
        if (parser == nullptr) {
            modelError("%1%: 'ip' argument of 'ingress' should be bound to a parser", block);
            return false;
        }
        p = ingress->findParameterValue("ig");
        if (p == nullptr) {
            modelError("'ingress' package %1% has no parameter named 'ig'", block);
            return false;
        }
        auto pipeline = p->to<IR::ControlBlock>();
        if (pipeline == nullptr) {
            modelError("%1%: 'ig' argument of 'ingress' should be bound to a control", block);
            return false;
        }
        p = ingress->findParameterValue("id");
        if (p == nullptr) {
            modelError("'ingress' package %1% has no parameter named 'id'", block);
            return false;
        }
        auto deparser = p->to<IR::ControlBlock>();
        if (deparser == nullptr) {
            modelError("'%1%: id' argument of 'ingress' should be bound to a control", block);
            return false;
        }
        structure->block_type.emplace(parser->container, std::make_pair(INGRESS, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(INGRESS, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(INGRESS, DEPARSER));
        structure->pipeline_controls.emplace(pipeline->container->name);
        structure->non_pipeline_controls.emplace(deparser->container->name);
    } else {
        modelError("'ingress' %1% is not bound to a package", pkg);
        return false;
    }
    pkg = block->findParameterValue("egress");
    if (pkg == nullptr) {
        modelError("Package %1% has no parameter named 'egress'", block);
        return false;
    }
    if (auto egress = pkg->to<IR::PackageBlock>()) {
        auto p = egress->findParameterValue("ep");
        if (p == nullptr) {
            modelError("'egress' package %1% has no parameter named 'ep'", block);
            return false;
        }
        auto parser = p->to<IR::ParserBlock>();
        if (parser == nullptr) {
            modelError("%1%: 'ep' argument of 'egress' should be bound to a parser", block);
            return false;
        }
        p = egress->findParameterValue("eg");
        if (p == nullptr) {
            modelError("'egress' package %1% has no parameter named 'eg'", block);
            return false;
        }
        auto pipeline = p->to<IR::ControlBlock>();
        if (pipeline == nullptr) {
            modelError("%1%: 'ig' argument of 'egress' should be bound to a control", block);
            return false;
        }
        p = egress->findParameterValue("ed");
        if (p == nullptr) {
            modelError("'egress' package %1% has no parameter named 'ed'", block);
            return false;
        }
        auto deparser = p->to<IR::ControlBlock>();
        if (deparser == nullptr) {
            modelError("%1%: 'ed' argument of 'egress' should be bound to a control", block);
            return false;
        }
        structure->block_type.emplace(parser->container, std::make_pair(EGRESS, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(EGRESS, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(EGRESS, DEPARSER));
        structure->pipeline_controls.emplace(pipeline->container->name);
        structure->non_pipeline_controls.emplace(deparser->container->name);
    } else {
        modelError("'egress' is not bound to a package", pkg);
        return false;
    }

    return false;
}

}  // namespace BMV2
