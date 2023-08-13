#include "dpdkProgramStructure.h"

#include <string.h>

#include <list>
#include <ostream>

#include "frontends/common/parser_options.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/stringify.h"
#include "options.h"

bool ParseDpdkArchitecture::preorder(const IR::ToplevelBlock *block) {
    /// Blocks are not in IR tree, use a custom visitor to traverse
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) visit(it.second->getNode());
    }
    return false;
}

void ParseDpdkArchitecture::parse_pna_block(const IR::PackageBlock *block) {
    structure->p4arch = "pna";
    auto p = block->findParameterValue("main_parser");
    if (p == nullptr) {
        ::error(ErrorType::ERR_MODEL, "Package %1% has no parameter named 'main_parser'", block);
        return;
    }
    auto parser = p->to<IR::ParserBlock>();
    structure->parsers.emplace("MainParserT", parser->container);
    p = block->findParameterValue("pre_control");
    auto pre_control = p->to<IR::ControlBlock>();
    structure->pipelines.emplace("PreControlT", pre_control->container);
    p = block->findParameterValue("main_control");
    auto pipeline = p->to<IR::ControlBlock>();
    structure->pipelines.emplace("MainControlT", pipeline->container);
    structure->pipeline_controls.emplace(pipeline->container->name);
    p = block->findParameterValue("main_deparser");
    auto deparser = p->to<IR::ControlBlock>();
    structure->deparsers.emplace("MainDeparserT", deparser->container);
    structure->non_pipeline_controls.emplace(deparser->container->name);
}

void ParseDpdkArchitecture::parse_psa_block(const IR::PackageBlock *block) {
    structure->p4arch = "psa";
    auto pkg = block->findParameterValue("ingress");
    if (pkg == nullptr) {
        ::error(ErrorType::ERR_MODEL, "Package %1% has no parameter named 'ingress'", block);
        return;
    }
    if (auto ingress = pkg->to<IR::PackageBlock>()) {
        auto p = ingress->findParameterValue("ip");
        if (!p) {
            ::error(ErrorType::ERR_MODEL, "'ingress' package %1% has no parameter named 'ip'",
                    block);
            return;
        }
        auto parser = p->to<IR::ParserBlock>();
        structure->parsers.emplace("IngressParser", parser->container);
        p = ingress->findParameterValue("ig");
        if (!p) {
            ::error(ErrorType::ERR_MODEL, "'ingress' package %1% has no parameter named 'ig'",
                    block);
            return;
        }
        auto pipeline = p->to<IR::ControlBlock>();
        structure->pipelines.emplace("Ingress", pipeline->container);
        structure->pipeline_controls.emplace(pipeline->container->name);
        p = ingress->findParameterValue("id");
        if (!p) {
            ::error(ErrorType::ERR_MODEL, "'ingress' package %1% has no parameter named 'id'",
                    block);
            return;
        }
        auto deparser = p->to<IR::ControlBlock>();
        structure->deparsers.emplace("IngressDeparser", deparser->container);
        structure->non_pipeline_controls.emplace(deparser->container->name);
    }
    pkg = block->findParameterValue("egress");
    if (auto egress = pkg->to<IR::PackageBlock>()) {
        auto p = egress->findParameterValue("ep");
        if (!p) {
            ::error(ErrorType::ERR_MODEL, "'egress' package %1% has no parameter named 'ep'",
                    block);
            return;
        }
        auto parser = p->to<IR::ParserBlock>();
        structure->parsers.emplace("EgressParser", parser->container);
        p = egress->findParameterValue("eg");
        if (!p) {
            ::error(ErrorType::ERR_MODEL, "'egress' package %1% has no parameter named 'eg'",
                    block);
            return;
        }
        auto pipeline = p->to<IR::ControlBlock>();
        structure->pipelines.emplace("Egress", pipeline->container);
        structure->pipeline_controls.emplace(pipeline->container->name);
        p = egress->findParameterValue("ed");
        if (!p) {
            ::error(ErrorType::ERR_MODEL, "'egress' package %1% has no parameter named 'ed'",
                    block);
            return;
        }
        auto deparser = p->to<IR::ControlBlock>();
        structure->deparsers.emplace("EgressDeparser", deparser->container);
        structure->non_pipeline_controls.emplace(deparser->container->name);
    }
}

bool ParseDpdkArchitecture::preorder(const IR::PackageBlock *block) {
    auto &options = DPDK::DpdkContext::get().options();
    if (options.arch == "psa" ||
        block->instanceType->to<IR::Type_Package>()->name == "PSA_Switch") {
        parse_psa_block(block);
    } else if (options.arch == "pna" ||
               block->instanceType->to<IR::Type_Package>()->name == "PNA_NIC") {
        parse_pna_block(block);
    } else {
        ::error(ErrorType::ERR_MODEL, "Unknown architecture %1%", options.arch);
    }
    return false;
}

bool InspectDpdkProgram::isHeaders(const IR::Type_StructLike *st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

void InspectDpdkProgram::addHeaderType(const IR::Type_StructLike *st) {
    LOG5("In addHeaderType with struct " << st->toString());
    if (st->is<IR::Type_HeaderUnion>()) {
        LOG5("Struct is Type_HeaderUnion");
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            addHeaderType(ht);
        }
        structure->header_union_types.emplace(st->getName(), st->to<IR::Type_HeaderUnion>());
        return;
    } else if (st->is<IR::Type_Header>()) {
        LOG5("Struct is Type_Header");
        structure->header_types.emplace(st->getName(), st->to<IR::Type_Header>());
    } else if (st->is<IR::Type_Struct>()) {
        LOG5("Struct is Type_Struct");
        structure->metadata_types.emplace(st->getName(), st->to<IR::Type_Struct>());
    }
}

void InspectDpdkProgram::addHeaderInstance(const IR::Type_StructLike *st, cstring name) {
    auto inst = new IR::Declaration_Variable(name, st);
    if (st->is<IR::Type_Header>())
        structure->headers.emplace(name, inst);
    else if (st->is<IR::Type_Struct>())
        structure->metadata.emplace(name, inst);
    else if (st->is<IR::Type_HeaderUnion>())
        structure->header_unions.emplace(name, inst);
}

void InspectDpdkProgram::addTypesAndInstances(const IR::Type_StructLike *type, bool isHeader) {
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
                structure->header_union_types.emplace(type->getName(),
                                                      type->to<IR::Type_HeaderUnion>());
                addHeaderInstance(type, f->controlPlaneName());
            } else {
                LOG5("Adding struct with type " << type);
                structure->metadata_types.emplace(type->getName(), type->to<IR::Type_Struct>());
                addHeaderInstance(type, f->controlPlaneName());
            }
        } else if (ft->is<IR::Type_Stack>()) {
            LOG5("Field is Type_Stack " << ft->toString());
            auto stack = ft->to<IR::Type_Stack>();
            auto stack_size = stack->getSize();
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            auto stack_type = stack->elementType->to<IR::Type_Header>();
            std::vector<unsigned> ids;
            for (unsigned i = 0; i < stack_size; i++) {
                cstring hdrName = f->controlPlaneName() + "[" + Util::toString(i) + "]";
                addHeaderInstance(stack_type, hdrName);
            }
        } else {
            // Treat this field like a scalar local variable
            cstring newName = refMap->newName(type->getName() + "." + f->name);
            LOG5("newname for scalarMetadataFields is " << newName);
            if (ft->is<IR::Type_Bits>()) {
                LOG5("Field is a Type_Bits");
                auto tb = ft->to<IR::Type_Bits>();
                structure->scalars_width += tb->size;
                structure->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                LOG5("Field is a Type_Boolean");
                structure->scalars_width += 1;
                structure->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Error>()) {
                LOG5("Field is a Type_Error");
                structure->scalars_width += 32;
                structure->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

bool InspectDpdkProgram::isStandardMetadata(cstring ptName) {
    return (!strcmp(ptName, "psa_ingress_parser_input_metadata_t") ||
            !strcmp(ptName, "psa_egress_parser_input_metadata_t") ||
            !strcmp(ptName, "psa_ingress_input_metadata_t") ||
            !strcmp(ptName, "psa_ingress_output_metadata_t") ||
            !strcmp(ptName, "psa_egress_input_metadata_t") ||
            !strcmp(ptName, "psa_egress_deparser_input_metadata_t") ||
            !strcmp(ptName, "psa_egress_output_metadata_t"));
}

bool InspectDpdkProgram::preorder(const IR::Declaration_Variable *dv) {
    auto ft = typeMap->getType(dv->getNode(), true);
    cstring scalarsName = refMap->newName("scalars");

    if (ft->is<IR::Type_Bits>()) {
        LOG5("Adding " << dv << " into scalars map");
        structure->scalars.emplace(scalarsName, dv);
    } else if (ft->is<IR::Type_Boolean>()) {
        LOG5("Adding " << dv << " into scalars map");
        structure->scalars.emplace(scalarsName, dv);
    } else {
        BUG("Unhandled type %1%", dv);
    }

    return false;
}

// This visitor only visits the parameter in the statement from architecture.
bool InspectDpdkProgram::preorder(const IR::Parameter *param) {
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
    if (structure->hasVisited(st)) {
        LOG5("Parameter is visited, returning");
        return false;
    }
    if (isStandardMetadata(ptName)) {
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

bool InspectDpdkProgram::preorder(const IR::P4Action *action) {
    structure->actions.emplace(action->name, action);
    return false;
}
