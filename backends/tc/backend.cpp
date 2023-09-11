/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#include "backend.h"

#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/target.h"

namespace TC {

const cstring Extern::dropPacket = "drop_packet";
const cstring Extern::sendToPort = "send_to_port";

cstring pnaMainParserInputMetaFields[TC::MAX_PNA_PARSER_META] = {"recirculated", "input_port"};

cstring pnaMainInputMetaFields[TC::MAX_PNA_INPUT_META] = {
    "recirculated", "timestamp", "parser_error", "class_of_service", "input_port"};

cstring pnaMainOutputMetaFields[TC::MAX_PNA_OUTPUT_META] = {"class_of_service"};

const cstring pnaParserMeta = "pna_main_parser_input_metadata_t";
const cstring pnaInputMeta = "pna_main_input_metadata_t";
const cstring pnaOutputMeta = "pna_main_output_metadata_t";

bool Backend::process() {
    CHECK_NULL(toplevel);
    if (toplevel->getMain() == nullptr) {
        ::error("main is missing in the package");
        return false;  //  no main
    }
    auto refMapEBPF = refMap;
    auto typeMapEBPF = typeMap;
    parseTCAnno = new ParseTCAnnotations();
    tcIR = new ConvertToBackendIR(toplevel, pipeline, refMap, typeMap, options);
    genIJ = new IntrospectionGenerator(pipeline, refMap, typeMap);
    addPasses({parseTCAnno, new P4::ResolveReferences(refMap),
               new P4::TypeInference(refMap, typeMap), tcIR, genIJ});
    toplevel->getProgram()->apply(*this);
    if (::errorCount() > 0) return false;
    if (!ebpfCodeGen(refMapEBPF, typeMapEBPF)) return false;
    return true;
}

bool Backend::ebpfCodeGen(P4::ReferenceMap *refMapEBPF, P4::TypeMap *typeMapEBPF) {
    target = new EBPF::KernelSamplesTarget(options.emitTraceMessages);
    ebpfOption.xdp2tcMode = options.xdp2tcMode;
    ebpfOption.exe_name = options.exe_name;
    ebpfOption.file = options.file;
    PnaProgramStructure structure(refMapEBPF, typeMapEBPF);
    auto parsePnaArch = new ParsePnaArchitecture(&structure);
    auto main = toplevel->getMain();
    if (!main) return false;

    if (main->type->name != "PNA_NIC") {
        ::warning(ErrorType::WARN_INVALID,
                  "%1%: the main package should be called PNA_NIC"
                  "; are you using the wrong architecture?",
                  main->type->name);
        return false;
    }

    main->apply(*parsePnaArch);
    auto evaluator = new P4::EvaluatorPass(refMapEBPF, typeMapEBPF);
    auto program = toplevel->getProgram();

    PassManager rewriteToEBPF = {
        evaluator,
        new VisitFunctor([this, evaluator, structure]() { top = evaluator->getToplevelBlock(); }),
    };

    auto hook = options.getDebugHook();
    rewriteToEBPF.addDebugHook(hook, true);
    program = program->apply(rewriteToEBPF);

    // map IR node to compile-time allocated resource blocks.
    top->apply(*new BMV2::BuildResourceMap(&structure.resourceMap));

    main = top->getMain();
    if (!main) return false;  // no main
    main->apply(*parsePnaArch);
    program = top->getProgram();

    EBPF::EBPFTypeFactory::createFactory(typeMapEBPF);
    auto convertToEbpf = new ConvertToEbpfPNA(ebpfOption, refMapEBPF, typeMapEBPF, tcIR);
    PassManager toEBPF = {
        new BMV2::DiscoverStructure(&structure),
        new InspectPnaProgram(refMapEBPF, typeMapEBPF, &structure),
        // convert to EBPF objects
        new VisitFunctor([evaluator, convertToEbpf]() {
            auto tlb = evaluator->getToplevelBlock();
            tlb->apply(*convertToEbpf);
        }),
    };

    toEBPF.addDebugHook(hook, true);
    program = program->apply(toEBPF);

    ebpf_program = convertToEbpf->getEBPFProgram();

    return true;
}

void Backend::serialize() const {
    if (!options.outputFile.isNullOrEmpty()) {
        auto outstream = openFile(options.outputFile, false);
        if (outstream != nullptr) {
            *outstream << pipeline->toString();
            outstream->flush();
        }
    }
    auto progName = options.file;
    auto filename = progName.findlast('/');
    if (filename) progName = filename;
    progName = progName.exceptLast(3);
    progName = progName.trim("/\t\n\r");
    cstring parserFile = progName + "_parser.c";
    cstring postParserFile = progName + "_post_parser.c";
    cstring headerFile = progName + "_parser.h";
    if (!options.cFile.isNullOrEmpty()) {
        if (options.cFile.get(options.cFile.size() - 1) != '/') {
            options.cFile = options.cFile + '/';
        }
        parserFile = options.cFile + parserFile;
        postParserFile = options.cFile + postParserFile;
        headerFile = options.cFile + headerFile;
    }
    auto cstream = openFile(postParserFile, false);
    auto pstream = openFile(parserFile, false);
    auto hstream = openFile(headerFile, false);
    if (cstream == nullptr) {
        ::error("Unable to open File %1%", postParserFile);
        return;
    }
    if (pstream == nullptr) {
        ::error("Unable to open File %1%", parserFile);
        return;
    }
    if (hstream == nullptr) {
        ::error("Unable to open File %1%", headerFile);
        return;
    }
    if (ebpf_program == nullptr) return;
    EBPF::CodeBuilder c(target), p(target), h(target);
    ebpf_program->emit(&c);
    ebpf_program->emitParser(&p);
    ebpf_program->emitHeader(&h);
    *cstream << c.toString();
    *pstream << p.toString();
    *hstream << h.toString();
    cstream->flush();
    pstream->flush();
    hstream->flush();
}

bool Backend::serializeIntrospectionJson(std::ostream &out) const {
    if (genIJ->serializeIntrospectionJson(out)) {
        out.flush();
        return true;
    }
    return false;
}

void ConvertToBackendIR::setPipelineName() {
    cstring path = options.file;
    if (path != nullptr) {
        pipelineName = path;
    } else {
        ::error("filename is not given in command line option");
        return;
    }
    auto fileName = path.findlast('/');
    if (fileName) {
        pipelineName = fileName;
        pipelineName = pipelineName.replace("/", "");
    }
    auto fileext = pipelineName.find(".");
    pipelineName = pipelineName.replace(fileext, "");
    pipelineName = pipelineName.trim();
}

bool ConvertToBackendIR::preorder(const IR::P4Program *p) {
    if (p != nullptr) {
        setPipelineName();
        return true;
    }
    return false;
}

cstring ConvertToBackendIR::externalName(const IR::IDeclaration *declaration) const {
    cstring name = declaration->externalName();
    if (name.startsWith(".")) name = name.substr(1);
    auto Name = name.replace('.', '/');
    return Name;
}

bool ConvertToBackendIR::isDuplicateOrNoAction(const IR::P4Action *action) {
    auto actionName = externalName(action);
    if (actions.find(actionName) != actions.end()) return true;
    if (actionName == P4::P4CoreLibrary::instance().noAction.name) return true;
    return false;
}

void ConvertToBackendIR::postorder(const IR::P4Action *action) {
    if (action != nullptr) {
        if (isDuplicateOrNoAction(action)) return;
        auto actionName = externalName(action);
        actions.emplace(actionName, action);
        actionCount++;
        unsigned int actionId = actionCount;
        IR::TCAction *tcAction = new IR::TCAction(actionName);
        tcAction->setPipelineName(pipelineName);
        tcAction->setActionId(actionId);
        actionIDList.emplace(actionId, actionName);
        auto paramList = action->getParameters();
        if (paramList != nullptr && !paramList->empty()) {
            for (auto param : paramList->parameters) {
                auto paramType = typeMap->getType(param);
                IR::TCActionParam *tcActionParam = new IR::TCActionParam();
                tcActionParam->setParamName(param->name.originalName);
                if (!paramType->is<IR::Type_Bits>()) {
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "%1% parameter with type other than bit is not supported", param);
                    return;
                } else {
                    auto paramTypeName = paramType->to<IR::Type_Bits>()->baseName();
                    if (paramTypeName != "bit") {
                        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "%1% parameter with type other than bit is not supported", param);
                        return;
                    }
                    tcActionParam->setDataType(TC::BIT_TYPE);
                    unsigned int width = paramType->to<IR::Type_Bits>()->width_bits();
                    tcActionParam->setBitSize(width);
                }
                auto annoList = param->getAnnotations()->annotations;
                for (auto anno : annoList) {
                    if (anno->name != ParseTCAnnotations::tcType) continue;
                    auto expr = anno->expr[0];
                    if (auto typeLiteral = expr->to<IR::StringLiteral>()) {
                        auto val = getTcType(typeLiteral);
                        if (val != TC::BIT_TYPE) {
                            tcActionParam->setDataType(val);
                        } else {
                            ::error(ErrorType::ERR_INVALID,
                                    "tc_type annotation cannot have '%1%' as value", expr);
                        }
                    } else {
                        ::error(ErrorType::ERR_INVALID,
                                "tc_type annotation cannot have '%1%' as value", expr);
                    }
                }
                tcAction->addActionParams(tcActionParam);
            }
        }
        tcPipeline->addActionDefinition(tcAction);
    }
}

void ConvertToBackendIR::updateDefaultMissAction(const IR::P4Table *t, IR::TCTable *tabledef) {
    auto defaultAction = t->getDefaultAction();
    if (defaultAction == nullptr || !defaultAction->is<IR::MethodCallExpression>()) return;
    auto methodexp = defaultAction->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(methodexp, refMap, typeMap);
    auto actionCall = mi->to<P4::ActionCall>();
    if (actionCall == nullptr) return;
    auto actionName = externalName(actionCall->action);
    if (actionName != P4::P4CoreLibrary::instance().noAction.name) {
        for (auto tcAction : tcPipeline->actionDefs) {
            if (actionName == tcAction->actionName) {
                tabledef->setDefaultMissAction(tcAction);
                auto defaultActionProperty =
                    t->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
                if (defaultActionProperty->isConstant) {
                    tabledef->setDefaultMissConst(true);
                }
            }
        }
    }
}

void ConvertToBackendIR::updateDefaultHitAction(const IR::P4Table *t, IR::TCTable *tabledef) {
    auto actionlist = t->getActionList();
    if (actionlist != nullptr) {
        unsigned int defaultHit = 0;
        unsigned int defaultHitConst = 0;
        cstring defaultActionName = nullptr;
        for (auto action : actionlist->actionList) {
            auto annoList = action->getAnnotations()->annotations;
            bool isTableOnly = false;
            bool isDefaultHit = false;
            bool isDefaultHitConst = false;
            for (auto anno : annoList) {
                if (anno->name == IR::Annotation::tableOnlyAnnotation) {
                    isTableOnly = true;
                }
                if (anno->name == ParseTCAnnotations::default_hit) {
                    isDefaultHit = true;
                    defaultHit++;
                    auto adecl = refMap->getDeclaration(action->getPath(), true);
                    defaultActionName = externalName(adecl);
                }
                if (anno->name == ParseTCAnnotations::default_hit_const) {
                    isDefaultHitConst = true;
                    defaultHitConst++;
                    auto adecl = refMap->getDeclaration(action->getPath(), true);
                    defaultActionName = externalName(adecl);
                }
            }
            if (isTableOnly && isDefaultHit && isDefaultHitConst) {
                ::error(ErrorType::ERR_INVALID,
                        "Table '%1%' has an action reference '%2%' which is "
                        "annotated with '@tableonly', '@default_hit' and '@default_hit_const'",
                        t->name.originalName, action->getName().originalName);
                break;
            } else if (isTableOnly && isDefaultHit) {
                ::error(ErrorType::ERR_INVALID,
                        "Table '%1%' has an action reference '%2%' which is "
                        "annotated with '@tableonly' and '@default_hit'",
                        t->name.originalName, action->getName().originalName);
                break;
            } else if (isTableOnly && isDefaultHitConst) {
                ::error(ErrorType::ERR_INVALID,
                        "Table '%1%' has an action reference '%2%' which is "
                        "annotated with '@tableonly' and '@default_hit_const'",
                        t->name.originalName, action->getName().originalName);
                break;
            } else if (isDefaultHit && isDefaultHitConst) {
                ::error(ErrorType::ERR_INVALID,
                        "Table '%1%' has an action reference '%2%' which is "
                        "annotated with '@default_hit' and '@default_hit_const'",
                        t->name.originalName, action->getName().originalName);
                break;
            }
        }
        if (::errorCount() > 0) {
            return;
        }
        if ((defaultHit > 0) && (defaultHitConst > 0)) {
            ::error(ErrorType::ERR_INVALID,
                    "Table '%1%' cannot have both '@default_hit' action "
                    "and '@default_hit_const' action",
                    t->name.originalName);
            return;
        } else if (defaultHit > 1) {
            ::error(ErrorType::ERR_INVALID, "Table '%1%' can have only one '@default_hit' action",
                    t->name.originalName);
            return;
        } else if (defaultHitConst > 1) {
            ::error(ErrorType::ERR_INVALID,
                    "Table '%1%' can have only one '@default_hit_const' action",
                    t->name.originalName);
            return;
        }
        if (defaultActionName != nullptr &&
            defaultActionName != P4::P4CoreLibrary::instance().noAction.name) {
            for (auto tcAction : tcPipeline->actionDefs) {
                if (defaultActionName == tcAction->actionName) {
                    tabledef->setDefaultHitAction(tcAction);
                    if (defaultHitConst == 1) {
                        tabledef->setDefaultHitConst(true);
                    }
                }
            }
        }
    }
}

void ConvertToBackendIR::postorder(const IR::P4Table *t) {
    if (t != nullptr) {
        tableCount++;
        unsigned int tId = tableCount;
        auto tName = t->name.originalName;
        tableIDList.emplace(tId, tName);
        auto ctrl = findContext<IR::P4Control>();
        auto cName = ctrl->name.originalName;
        IR::TCTable *tableDefinition = new IR::TCTable(tId, tName, cName, pipelineName);
        auto tEntriesCount = TC::DEFAULT_TABLE_ENTRIES;
        auto sizeProperty = t->getSizeProperty();
        if (sizeProperty) {
            if (sizeProperty->fitsUint64()) {
                tEntriesCount = sizeProperty->asUint64();
            } else {
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "table with size %1% cannot be supported", t->getSizeProperty());
                return;
            }
        }
        tableDefinition->setTableEntriesCount(tEntriesCount);
        unsigned int keySize = 0;
        unsigned int keyCount = 0;
        auto key = t->getKey();
        if (key != nullptr && key->keyElements.size()) {
            for (auto k : key->keyElements) {
                auto keyExp = k->expression;
                auto keyExpType = typeMap->getType(keyExp);
                auto widthBits = keyExpType->width_bits();
                keySize += widthBits;
                keyCount++;
            }
        }
        tableDefinition->setKeySize(keySize);
        tableKeysizeList.emplace(tId, keySize);
        auto annoList = t->getAnnotations()->annotations;
        for (auto anno : annoList) {
            if (anno->name != ParseTCAnnotations::numMask) continue;
            auto expr = anno->expr[0];
            if (auto val = expr->to<IR::Constant>()) {
                tableDefinition->setNumMask(val->asUint64());
            } else {
                ::error(ErrorType::ERR_INVALID,
                        "nummask annotation cannot have '%1%' as value. Only integer "
                        "constants are allowed",
                        expr);
            }
        }
        auto actionlist = t->getActionList();
        for (auto action : actionlist->actionList) {
            for (auto actionDef : tcPipeline->actionDefs) {
                auto adecl = refMap->getDeclaration(action->getPath(), true);
                auto actionName = externalName(adecl);
                if (actionName != actionDef->actionName) continue;
                auto annoList = action->getAnnotations()->annotations;
                unsigned int tableFlag = TC::TABLEDEFAULT;
                for (auto anno : annoList) {
                    if (anno->name == IR::Annotation::tableOnlyAnnotation) {
                        tableFlag = TC::TABLEONLY;
                    }
                    if (anno->name == IR::Annotation::defaultOnlyAnnotation) {
                        tableFlag = TC::DEFAULTONLY;
                    }
                }
                tableDefinition->addAction(actionDef, tableFlag);
            }
        }
        updateDefaultHitAction(t, tableDefinition);
        updateDefaultMissAction(t, tableDefinition);
        updateMatchType(t, tableDefinition);
        tcPipeline->addTableDefinition(tableDefinition);
    }
}

void ConvertToBackendIR::postorder(const IR::P4Program *p) {
    if (p != nullptr) {
        tcPipeline->setPipelineName(pipelineName);
        tcPipeline->setPipelineId(TC::DEFAULT_PIPELINE_ID);
        tcPipeline->setNumTables(tableCount);
    }
}

/**
 * This function is used for checking whether given member is PNA Parser metadata
 */
bool ConvertToBackendIR::isPnaParserMeta(const IR::Member *mem) {
    if (mem->expr != nullptr && mem->expr->type != nullptr) {
        if (auto str_type = mem->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == pnaParserMeta) return true;
        }
    }
    return false;
}

bool ConvertToBackendIR::isPnaMainInputMeta(const IR::Member *mem) {
    if (mem->expr != nullptr && mem->expr->type != nullptr) {
        if (auto str_type = mem->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == pnaInputMeta) return true;
        }
    }
    return false;
}

bool ConvertToBackendIR::isPnaMainOutputMeta(const IR::Member *mem) {
    if (mem->expr != nullptr && mem->expr->type != nullptr) {
        if (auto str_type = mem->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == pnaOutputMeta) return true;
        }
    }
    return false;
}

unsigned int ConvertToBackendIR::findMappedKernelMeta(const IR::Member *mem) {
    if (isPnaParserMeta(mem)) {
        for (auto i = 0; i < TC::MAX_PNA_PARSER_META; i++) {
            if (mem->member.name == pnaMainParserInputMetaFields[i]) {
                if (i == TC::PARSER_RECIRCULATED) {
                    return TC::SKBREDIR;
                } else if (i == TC::PARSER_INPUT_PORT) {
                    return TC::SKBIIF;
                }
            }
        }
    } else if (isPnaMainInputMeta(mem)) {
        for (auto i = 0; i < TC::MAX_PNA_INPUT_META; i++) {
            if (mem->member.name == pnaMainInputMetaFields[i]) {
                switch (i) {
                    case TC::INPUT_RECIRCULATED:
                        return TC::SKBREDIR;
                    case TC::INPUT_TIMESTAMP:
                        return TC::SKBTSTAMP;
                    case TC::INPUT_PARSER_ERROR:
                        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "%1% is not supported in this target", mem);
                        return TC::UNSUPPORTED;
                    case TC::INPUT_CLASS_OF_SERVICE:
                        return TC::SKBPRIO;
                    case TC::INPUT_INPUT_PORT:
                        return TC::SKBIIF;
                }
            }
        }
    } else if (isPnaMainOutputMeta(mem)) {
        if (mem->member.name == pnaMainOutputMetaFields[TC::OUTPUT_CLASS_OF_SERVICE]) {
            return TC::SKBPRIO;
        }
    }
    return TC::UNDEFINED;
}

const IR::Expression *ConvertToBackendIR::ExtractExpFromCast(const IR::Expression *exp) {
    const IR::Expression *castexp = exp;
    while (castexp->is<IR::Cast>()) {
        castexp = castexp->to<IR::Cast>()->expr;
    }
    return castexp;
}

unsigned ConvertToBackendIR::getTcType(const IR::StringLiteral *sl) {
    auto value = sl->value;
    auto typeVal = TC::BIT_TYPE;
    if (value == "dev") {
        typeVal = TC::DEV_TYPE;
    } else if (value == "macaddr") {
        typeVal = TC::MACADDR_TYPE;
    } else if (value == "ipv4") {
        typeVal = TC::IPV4_TYPE;
    } else if (value == "ipv6") {
        typeVal = TC::IPV6_TYPE;
    } else if (value == "be16") {
        typeVal = TC::BE16_TYPE;
    } else if (value == "be32") {
        typeVal = TC::BE32_TYPE;
    } else if (value == "be64") {
        typeVal = TC::BE64_TYPE;
    }
    return typeVal;
}

unsigned ConvertToBackendIR::getTableId(cstring tableName) const {
    for (auto t : tableIDList) {
        if (t.second == tableName) return t.first;
    }
    return 0;
}

unsigned ConvertToBackendIR::getActionId(cstring actionName) const {
    for (auto a : actionIDList) {
        if (a.second == actionName) return a.first;
    }
    return 0;
}

unsigned ConvertToBackendIR::getTableKeysize(unsigned tableId) const {
    auto itr = tableKeysizeList.find(tableId);
    if (itr != tableKeysizeList.end()) return itr->second;
    return 0;
}

void ConvertToBackendIR::updateMatchType(const IR::P4Table *t, IR::TCTable *tabledef) {
    auto key = t->getKey();
    auto tableMatchType = TC::EXACT_TYPE;
    if (key != nullptr && key->keyElements.size()) {
        if (key->keyElements.size() == 1) {
            auto matchTypeExp = key->keyElements[0]->matchType->path;
            auto mtdecl = refMap->getDeclaration(matchTypeExp, true);
            auto matchTypeInfo = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchTypeInfo->name.name == P4::P4CoreLibrary::instance().exactMatch.name) {
                tableMatchType = TC::EXACT_TYPE;
            } else if (matchTypeInfo->name.name == P4::P4CoreLibrary::instance().lpmMatch.name) {
                tableMatchType = TC::LPM_TYPE;
            } else if (matchTypeInfo->name.name ==
                       P4::P4CoreLibrary::instance().ternaryMatch.name) {
                tableMatchType = TC::TERNARY_TYPE;
            } else {
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "match type %1% is not supported in this target",
                        key->keyElements[0]->matchType);
                return;
            }
        } else {
            unsigned totalKey = key->keyElements.size();
            unsigned exactKey = 0;
            unsigned lpmKey = 0;
            unsigned ternaryKey = 0;
            unsigned keyCount = 0;
            unsigned lastkeyMatchType = TC::EXACT_TYPE;
            unsigned keyMatchType;
            for (auto k : key->keyElements) {
                auto matchTypeExp = k->matchType->path;
                auto mtdecl = refMap->getDeclaration(matchTypeExp, true);
                auto matchTypeInfo = mtdecl->getNode()->to<IR::Declaration_ID>();
                if (matchTypeInfo->name.name == P4::P4CoreLibrary::instance().exactMatch.name) {
                    keyMatchType = TC::EXACT_TYPE;
                    exactKey++;
                } else if (matchTypeInfo->name.name ==
                           P4::P4CoreLibrary::instance().lpmMatch.name) {
                    keyMatchType = TC::LPM_TYPE;
                    lpmKey++;
                } else if (matchTypeInfo->name.name ==
                           P4::P4CoreLibrary::instance().ternaryMatch.name) {
                    keyMatchType = TC::TERNARY_TYPE;
                    ternaryKey++;
                } else {
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "match type %1% is not supported in this target", k->matchType);
                    return;
                }
                keyCount++;
                if (keyCount == totalKey) {
                    lastkeyMatchType = keyMatchType;
                }
            }
            if (ternaryKey >= 1 || lpmKey > 1) {
                tableMatchType = TC::TERNARY_TYPE;
            } else if (exactKey == totalKey) {
                tableMatchType = TC::EXACT_TYPE;
            } else if (lpmKey == 1 && lastkeyMatchType == TC::LPM_TYPE) {
                tableMatchType = TC::LPM_TYPE;
            }
        }
    }
    tabledef->setMatchType(tableMatchType);
}

}  // namespace TC
