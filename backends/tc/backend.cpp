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

#include <filesystem>

#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/target.h"

namespace P4::TC {

using namespace ::P4::literals;

const cstring Extern::dropPacket = "drop_packet"_cs;
const cstring Extern::sendToPort = "send_to_port"_cs;

cstring pnaMainParserInputMetaFields[TC::MAX_PNA_PARSER_META] = {"recirculated"_cs,
                                                                 "input_port"_cs};

cstring pnaMainInputMetaFields[TC::MAX_PNA_INPUT_META] = {
    "recirculated"_cs, "timestamp"_cs, "parser_error"_cs, "class_of_service"_cs, "input_port"_cs};

cstring pnaMainOutputMetaFields[TC::MAX_PNA_OUTPUT_META] = {"class_of_service"_cs};

const cstring pnaParserMeta = "pna_main_parser_input_metadata_t"_cs;
const cstring pnaInputMeta = "pna_main_input_metadata_t"_cs;
const cstring pnaOutputMeta = "pna_main_output_metadata_t"_cs;

bool Backend::process() {
    CHECK_NULL(toplevel);
    if (toplevel->getMain() == nullptr) {
        ::P4::error("main is missing in the package");
        return false;  //  no main
    }
    auto refMapEBPF = refMap;
    auto typeMapEBPF = typeMap;
    auto hook = options.getDebugHook();
    parseTCAnno = new ParseTCAnnotations();
    tcIR = new ConvertToBackendIR(toplevel, pipeline, refMap, typeMap, options);
    genIJ = new IntrospectionGenerator(pipeline, refMap, typeMap);
    PassManager backEnd = {};
    backEnd.addPasses({parseTCAnno, new P4::ClearTypeMap(typeMap),
                       new P4::TypeChecking(refMap, typeMap, true), tcIR, genIJ});
    backEnd.addDebugHook(hook, true);
    toplevel->getProgram()->apply(backEnd);
    if (::P4::errorCount() > 0) return false;
    if (!ebpfCodeGen(refMapEBPF, typeMapEBPF)) return false;
    return true;
}

bool Backend::ebpfCodeGen(P4::ReferenceMap *refMapEBPF, P4::TypeMap *typeMapEBPF) {
    target = new EBPF::P4TCTarget(options.emitTraceMessages);
    ebpfOption.xdp2tcMode = options.xdp2tcMode;
    ebpfOption.exe_name = options.exe_name;
    ebpfOption.file = options.file;
    PnaProgramStructure structure(refMapEBPF, typeMapEBPF);
    auto parsePnaArch = new ParsePnaArchitecture(&structure);
    auto main = toplevel->getMain();
    if (!main) return false;

    if (main->type->name != "PNA_NIC") {
        ::P4::warning(ErrorType::WARN_INVALID,
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
    top->apply(*new P4::BuildResourceMap(&structure.resourceMap));

    main = top->getMain();
    if (!main) return false;  // no main
    main->apply(*parsePnaArch);
    program = top->getProgram();

    EBPF::EBPFTypeFactory::createFactory(typeMapEBPF);
    auto convertToEbpf = new ConvertToEbpfPNA(ebpfOption, refMapEBPF, typeMapEBPF, tcIR);
    PassManager toEBPF = {
        new P4::DiscoverStructure(&structure),
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
    std::string progName = tcIR->getPipelineName().string();
    if (ebpf_program == nullptr) return;
    EBPF::CodeBuilder c(target), p(target), h(target);
    ebpf_program->emit(&c);
    ebpf_program->emitParser(&p);
    ebpf_program->emitHeader(&h);
    if (::P4::errorCount() > 0) {
        return;
    }
    std::filesystem::path outputFile = options.outputFolder / (progName + ".template");

    auto outstream = openFile(outputFile, false);
    if (outstream != nullptr) {
        *outstream << pipeline->toString();
        outstream->flush();
        std::filesystem::permissions(outputFile.c_str(),
                                     std::filesystem::perms::owner_all |
                                         std::filesystem::perms::group_all |
                                         std::filesystem::perms::others_all,
                                     std::filesystem::perm_options::add);
    }
    std::filesystem::path parserFile = options.outputFolder / (progName + "_parser.c");
    std::filesystem::path postParserFile = options.outputFolder / (progName + "_control_blocks.c");
    std::filesystem::path headerFile = options.outputFolder / (progName + "_parser.h");

    auto cstream = openFile(postParserFile, false);
    auto pstream = openFile(parserFile, false);
    auto hstream = openFile(headerFile, false);
    if (cstream == nullptr) {
        ::P4::error("Unable to open File %1%", postParserFile);
        return;
    }
    if (pstream == nullptr) {
        ::P4::error("Unable to open File %1%", parserFile);
        return;
    }
    if (hstream == nullptr) {
        ::P4::error("Unable to open File %1%", headerFile);
        return;
    }
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
    if (options.file.empty()) {
        ::P4::error("filename is not given in command line option");
        return;
    }

    pipelineName = cstring(options.file.stem());
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

bool ConvertToBackendIR::isDuplicateAction(const IR::P4Action *action) {
    auto actionName = externalName(action);
    if (actions.find(actionName) != actions.end()) return true;
    return false;
}

void ConvertToBackendIR::postorder(const IR::P4Action *action) {
    if (action != nullptr) {
        if (isDuplicateAction(action)) return;
        auto actionName = externalName(action);
        if (actionName == P4::P4CoreLibrary::instance().noAction.name) {
            tcPipeline->addNoActionDefinition(new IR::TCAction("NoAction"_cs));
            actions.emplace("NoAction"_cs, action);
            return;
        }
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
                    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "%1% parameter with type other than bit is not supported", param);
                    return;
                } else {
                    auto paramTypeName = paramType->to<IR::Type_Bits>()->baseName();
                    if (paramTypeName != "bit") {
                        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                    "%1% parameter with type other than bit is not supported",
                                    param);
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
                            ::P4::error(ErrorType::ERR_INVALID,
                                        "tc_type annotation cannot have '%1%' as value", expr);
                        }
                    } else {
                        ::P4::error(ErrorType::ERR_INVALID,
                                    "tc_type annotation cannot have '%1%' as value", expr);
                    }
                }
                auto direction = param->direction;
                if (direction == IR::Direction::InOut) {
                    tcActionParam->setDirection(TC::INOUT);
                } else if (direction == IR::Direction::In) {
                    tcActionParam->setDirection(TC::IN);
                } else if (direction == IR::Direction::Out) {
                    tcActionParam->setDirection(TC::OUT);
                } else {
                    tcActionParam->setDirection(TC::NONE);
                }
                tcAction->addActionParams(tcActionParam);
            }
        }
        tcPipeline->addActionDefinition(tcAction);
    }
}

void ConvertToBackendIR::updateTimerProfiles(IR::TCTable *tabledef) {
    if (options.timerProfiles > DEFAULT_TIMER_PROFILES) {
        tabledef->addTimerProfiles(options.timerProfiles);
    }
}
void ConvertToBackendIR::updateConstEntries(const IR::P4Table *t, IR::TCTable *tabledef) {
    // Check if there are const entries.
    auto entriesList = t->getEntries();
    if (entriesList == nullptr) return;
    auto keys = t->getKey();
    if (keys == nullptr) {
        return;
    }
    for (auto e : entriesList->entries) {
        auto keyset = e->getKeys();
        if (keyset->components.size() != keys->keyElements.size()) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "No of keys in const_entries should be same as no of keys in the table.");
            return;
        }
        ordered_map<cstring, cstring> keyList;
        for (size_t itr = 0; itr < keyset->components.size(); itr++) {
            auto keyElement = keys->keyElements.at(itr);
            auto keyString = keyElement->expression->toString();
            auto annotations = keyElement->getAnnotations();
            if (annotations) {
                if (auto anno = annotations->getSingle("name"_cs)) {
                    keyString = anno->expr.at(0)->to<IR::StringLiteral>()->value;
                }
            }
            auto keySetElement = keyset->components.at(itr);
            auto key = keySetElement->toString();
            if (keySetElement->is<IR::DefaultExpression>()) {
                key = "default"_cs;
            } else if (keySetElement->is<IR::Constant>()) {
                big_int kValue = keySetElement->to<IR::Constant>()->value;
                int kBase = keySetElement->to<IR::Constant>()->base;
                std::stringstream value;
                switch (kBase) {
                    case 2:
                        value << "0b";
                        break;
                    case 8:
                        value << "0o";
                        break;
                    case 16:
                        value << "0x";
                        break;
                    case 10:
                        break;
                    default:
                        BUG("Unexpected base %1%", kBase);
                }
                std::deque<char> buf;
                do {
                    const int digit = static_cast<int>(static_cast<big_int>(kValue % kBase));
                    kValue = kValue / kBase;
                    buf.push_front(Util::DigitToChar(digit));
                } while (kValue > 0);
                for (auto ch : buf) value << ch;
                key = value.str();
            } else if (keySetElement->is<IR::Range>()) {
                auto left = keySetElement->to<IR::Range>()->left;
                auto right = keySetElement->to<IR::Range>()->right;
                auto operand = keySetElement->to<IR::Range>()->getStringOp();
                key = left->toString() + operand + right->toString();
            } else if (keySetElement->is<IR::Mask>()) {
                auto left = keySetElement->to<IR::Mask>()->left;
                auto right = keySetElement->to<IR::Mask>()->right;
                auto operand = keySetElement->to<IR::Mask>()->getStringOp();
                key = left->toString() + operand + right->toString();
            }
            keyList.emplace(keyString, key);
        }
        cstring actionName;
        if (const auto *path = e->action->to<IR::PathExpression>())
            actionName = path->toString();
        else if (const auto *mce = e->action->to<IR::MethodCallExpression>())
            actionName = mce->method->toString();
        else
            BUG("Unexpected entry action type.");
        IR::TCEntry *constEntry = new IR::TCEntry(actionName, keyList);
        tabledef->addConstEntries(constEntry);
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
                bool isTCMayOverrideMiss = false;
                const IR::Annotation *overrideAnno =
                    defaultActionProperty->getAnnotations()->getSingle(
                        ParseTCAnnotations::tcMayOverride);
                if (overrideAnno) {
                    isTCMayOverrideMiss = true;
                }
                bool directionParamPresent = false;
                auto paramList = actionCall->action->getParameters();
                for (auto param : paramList->parameters) {
                    if (param->direction != IR::Direction::None) directionParamPresent = true;
                }
                if (!directionParamPresent) {
                    auto i = 0;
                    if (isTCMayOverrideMiss) {
                        if (paramList->parameters.empty())
                            ::P4::warning(
                                ErrorType::WARN_INVALID,
                                "%1% annotation cannot be used with default_action without "
                                "parameters",
                                overrideAnno);
                        else
                            tabledef->setTcMayOverrideMiss();
                    }
                    for (auto param : paramList->parameters) {
                        auto defaultParam = new IR::TCDefaultActionParam();
                        for (auto actionParam : tcAction->actionParams) {
                            if (actionParam->paramName == param->name.originalName) {
                                defaultParam->setParamDetail(actionParam);
                            }
                        }
                        auto defaultArg = methodexp->arguments->at(i++);
                        if (auto constVal = defaultArg->expression->to<IR::Constant>()) {
                            if (!isTCMayOverrideMiss)
                                defaultParam->setDefaultValue(
                                    Util::toString(constVal->value, 0, true, constVal->base));
                            tabledef->defaultMissActionParams.push_back(defaultParam);
                        }
                    }
                } else {
                    if (isTCMayOverrideMiss)
                        ::P4::warning(ErrorType::WARN_INVALID,
                                      "%1% annotation cannot be used with default_action with "
                                      "directional parameters",
                                      overrideAnno);
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
        bool isTcMayOverrideHitAction = false;
        for (auto action : actionlist->actionList) {
            auto annoList = action->getAnnotations()->annotations;
            bool isTableOnly = false;
            bool isDefaultHit = false;
            bool isDefaultHitConst = false;
            bool isTcMayOverrideHit = false;
            for (auto anno : annoList) {
                if (anno->name == IR::Annotation::tableOnlyAnnotation) {
                    isTableOnly = true;
                }
                if (anno->name == ParseTCAnnotations::defaultHit) {
                    isDefaultHit = true;
                    defaultHit++;
                    auto adecl = refMap->getDeclaration(action->getPath(), true);
                    defaultActionName = externalName(adecl);
                }
                if (anno->name == ParseTCAnnotations::defaultHitConst) {
                    isDefaultHitConst = true;
                    defaultHitConst++;
                    auto adecl = refMap->getDeclaration(action->getPath(), true);
                    defaultActionName = externalName(adecl);
                }
                if (anno->name == ParseTCAnnotations::tcMayOverride) {
                    isTcMayOverrideHit = true;
                }
            }
            if (isTableOnly && isDefaultHit && isDefaultHitConst) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@tableonly', '@default_hit' and '@default_hit_const'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isTableOnly && isDefaultHit) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@tableonly' and '@default_hit'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isTableOnly && isDefaultHitConst) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@tableonly' and '@default_hit_const'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isDefaultHit && isDefaultHitConst) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@default_hit' and '@default_hit_const'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isTcMayOverrideHit) {
                auto adecl = refMap->getDeclaration(action->getPath(), true);
                auto p4Action = adecl->getNode()->checkedTo<IR::P4Action>();
                if (!isDefaultHit && !isDefaultHitConst) {
                    ::P4::warning(ErrorType::WARN_INVALID,
                                  "Table '%1%' has an action reference '%2%' which is "
                                  "annotated with '@tc_may_override' without '@default_hit' or "
                                  "'@default_hit_const'",
                                  t->name.originalName, action->getName().originalName);
                    isTcMayOverrideHit = false;
                    break;
                } else if (p4Action->getParameters()->parameters.empty()) {
                    ::P4::warning(ErrorType::WARN_INVALID,
                                  " '@tc_may_override' cannot be used for %1%  action "
                                  " without parameters",
                                  action->getName().originalName);
                    isTcMayOverrideHit = false;
                    break;
                }
                isTcMayOverrideHitAction = true;
            }
        }
        if (::P4::errorCount() > 0) {
            return;
        }
        if ((defaultHit > 0) && (defaultHitConst > 0)) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "Table '%1%' cannot have both '@default_hit' action "
                        "and '@default_hit_const' action",
                        t->name.originalName);
            return;
        } else if (defaultHit > 1) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "Table '%1%' can have only one '@default_hit' action",
                        t->name.originalName);
            return;
        } else if (defaultHitConst > 1) {
            ::P4::error(ErrorType::ERR_INVALID,
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
                    if (isTcMayOverrideHitAction) {
                        if (!checkParameterDirection(tcAction)) {
                            tabledef->setTcMayOverrideHit();
                            for (auto param : tcAction->actionParams) {
                                auto defaultParam = new IR::TCDefaultActionParam();
                                defaultParam->setParamDetail(param);
                                tabledef->defaultHitActionParams.push_back(defaultParam);
                            }
                        }
                    }
                }
            }
        }
    }
}

void ConvertToBackendIR::updatePnaDirectCounter(const IR::P4Table *t, IR::TCTable *tabledef,
                                                unsigned tentries) {
    cstring propertyName = "pna_direct_counter"_cs;
    auto property = t->properties->getProperty(propertyName);
    if (property == nullptr) return;
    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    auto ctrl = findContext<IR::P4Control>();
    auto cName = ctrl->name.originalName;
    auto externInstanceName = cName + "." + expr->toString();
    tabledef->setDirectCounter(externInstanceName);

    auto externInstance = P4::ExternInstance::resolve(expr, refMap, typeMap);
    if (!externInstance) {
        ::P4::error(ErrorType::ERR_INVALID,
                    "Expected %1% property value for table %2% to resolve to an "
                    "extern instance: %3%",
                    propertyName, t->name.originalName, property);
        return;
    }
    for (auto ext : externsInfo) {
        if (ext.first == externInstance->type->toString()) {
            auto externDefinition = tcPipeline->getExternDefinition(ext.first);
            if (externDefinition) {
                auto extInstDef = ((IR::TCExternInstance *)externDefinition->getExternInstance(
                    externInstanceName));
                extInstDef->setExternTableBindable(true);
                extInstDef->setNumElements(tentries);
                break;
            }
        }
    }
}

void ConvertToBackendIR::updateAddOnMissTable(const IR::P4Table *t) {
    auto tblname = t->name.originalName;
    for (auto table : tcPipeline->tableDefs) {
        if (table->tableName == tblname) {
            add_on_miss_tables.push_back(t);
            auto tableDefinition = ((IR::TCTable *)table);
            tableDefinition->setTableAddOnMiss();
            tableDefinition->setTablePermission(HandleTableAccessPermission(t));
        }
    }
}

unsigned ConvertToBackendIR::GetAccessNumericValue(std::string_view access) {
    unsigned value = 0;
    for (auto s : access) {
        unsigned mask = 0;
        switch (s) {
            case 'C':
                mask = 1 << 6;
                break;
            case 'R':
                mask = 1 << 5;
                break;
            case 'U':
                mask = 1 << 4;
                break;
            case 'D':
                mask = 1 << 3;
                break;
            case 'X':
                mask = 1 << 2;
                break;
            case 'P':
                mask = 1 << 1;
                break;
            case 'S':
                mask = 1;
                break;
            default:
                ::P4::error(ErrorType::ERR_INVALID,
                            "tc_acl annotation cannot have '%1%' in access permisson", s);
        }
        value |= mask;
    }
    return value;
}

cstring ConvertToBackendIR::HandleTableAccessPermission(const IR::P4Table *t) {
    bool IsTableAddOnMiss = false;
    cstring control_path, data_path;
    for (auto table : add_on_miss_tables) {
        if (table->name.originalName == t->name.originalName) {
            IsTableAddOnMiss = true;
        }
    }
    auto find = tablePermissions.find(t->name.originalName);
    if (find != tablePermissions.end()) {
        auto paths = tablePermissions[t->name.originalName];
        control_path = paths->first;
        data_path = paths->second;
    }
    // Default access value of Control_path and Data_Path
    if (control_path.isNullOrEmpty()) {
        control_path = cstring(IsTableAddOnMiss ? DEFAULT_ADD_ON_MISS_TABLE_CONTROL_PATH_ACCESS
                                                : DEFAULT_TABLE_CONTROL_PATH_ACCESS);
    }
    if (data_path.isNullOrEmpty()) {
        data_path = cstring(IsTableAddOnMiss ? DEFAULT_ADD_ON_MISS_TABLE_DATA_PATH_ACCESS
                                             : DEFAULT_TABLE_DATA_PATH_ACCESS);
    }

    if (IsTableAddOnMiss) {
        auto access = data_path.find('C');
        if (!access) {
            ::P4::warning(
                ErrorType::WARN_INVALID,
                "Add on miss table '%1%' should have 'create' access permissons for data path.",
                t->name.originalName);
        }
    }
    // FIXME: refactor not to require cstring
    auto access_cp = GetAccessNumericValue(control_path.string_view());
    auto access_dp = GetAccessNumericValue(data_path.string_view());
    auto access_permisson = (access_cp << 7) | access_dp;
    std::stringstream value;
    value << "0x" << std::hex << access_permisson;
    return value.str();
}

std::pair<cstring, cstring> *ConvertToBackendIR::GetAnnotatedAccessPath(
    const IR::Annotation *anno) {
    cstring control_path, data_path;
    if (anno) {
        auto expr = anno->expr[0];
        if (auto typeLiteral = expr->to<IR::StringLiteral>()) {
            auto permisson_str = typeLiteral->value;
            auto char_pos = permisson_str.find(":");
            control_path = permisson_str.before(char_pos);
            data_path = permisson_str.substr(char_pos - permisson_str.begin() + 1);
        }
    }
    auto paths = new std::pair<cstring, cstring>(control_path, data_path);
    return paths;
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
                ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
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
            if (anno->name == ParseTCAnnotations::tc_acl) {
                tablePermissions.emplace(t->name.originalName, GetAnnotatedAccessPath(anno));
            } else if (anno->name == ParseTCAnnotations::numMask) {
                auto expr = anno->expr[0];
                if (auto val = expr->to<IR::Constant>()) {
                    tableDefinition->setNumMask(val->asUint64());
                } else {
                    ::P4::error(ErrorType::ERR_INVALID,
                                "nummask annotation cannot have '%1%' as value. Only integer "
                                "constants are allowed",
                                expr);
                }
            }
        }
        tableDefinition->setTablePermission(HandleTableAccessPermission(t));
        auto actionlist = t->getActionList();
        if (actionlist->size() == 0) {
            tableDefinition->addAction(tcPipeline->NoAction, TC::TABLEDEFAULT);
        } else {
            for (auto action : actionlist->actionList) {
                const IR::TCAction *tcAction = nullptr;
                auto adecl = refMap->getDeclaration(action->getPath(), true);
                auto actionName = externalName(adecl);
                for (auto actionDef : tcPipeline->actionDefs) {
                    if (actionName != actionDef->actionName) continue;
                    tcAction = actionDef;
                }
                if (actionName == P4::P4CoreLibrary::instance().noAction.name) {
                    tcAction = tcPipeline->NoAction;
                }
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
                if (tcAction) {
                    tableDefinition->addAction(tcAction, tableFlag);
                }
            }
        }
        updatePnaDirectCounter(t, tableDefinition, tEntriesCount);
        updateDefaultHitAction(t, tableDefinition);
        updateDefaultMissAction(t, tableDefinition);
        updateMatchType(t, tableDefinition);
        updateConstEntries(t, tableDefinition);
        updateTimerProfiles(tableDefinition);
        tcPipeline->addTableDefinition(tableDefinition);
    }
}

cstring ConvertToBackendIR::processExternPermission(const IR::Type_Extern *ext) {
    cstring control_path, data_path;
    // Check if access permissions is defined with annotation @tc_acl
    auto annoList = ext->getAnnotations()->annotations;
    for (auto anno : annoList) {
        if (anno->name == ParseTCAnnotations::tc_acl) {
            auto path = GetAnnotatedAccessPath(anno);
            control_path = path->first;
            data_path = path->second;
        }
    }
    // Default access value of Control_path and Data_Path
    if (control_path.isNullOrEmpty()) {
        control_path = cstring(DEFAULT_EXTERN_CONTROL_PATH_ACCESS);
    }
    if (data_path.isNullOrEmpty()) {
        data_path = cstring(DEFAULT_EXTERN_DATA_PATH_ACCESS);
    }
    auto access_cp = GetAccessNumericValue(control_path.string_view());
    auto access_dp = GetAccessNumericValue(data_path.string_view());
    auto access_permisson = (access_cp << 7) | access_dp;
    std::stringstream value;
    value << "0x" << std::hex << access_permisson;
    return value.str();
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::processExternConstructor(
    const IR::Type_Extern *extn, const IR::Declaration_Instance *decl,
    struct ExternInstance *instance) {
    safe_vector<const IR::TCKey *> keys;
    for (auto gd : *extn->getDeclarations()) {
        if (!gd->getNode()->is<IR::Method>()) {
            continue;
        }
        auto method = gd->getNode()->to<IR::Method>();
        auto params = method->getParameters();
        // Check if method is an constructor
        if (method->name != extn->name) {
            continue;
        }
        if (decl->arguments->size() != params->size()) {
            continue;
        }
        // Process all constructor arguments
        for (unsigned itr = 0; itr < params->size(); itr++) {
            auto parameter = params->getParameter(itr);
            auto exp = decl->arguments->at(itr)->expression;
            if (parameter->getAnnotations()->getSingle(ParseTCAnnotations::tc_numel)) {
                if (exp->is<IR::Constant>()) {
                    instance->is_num_elements = true;
                    instance->num_elements = exp->to<IR::Constant>()->asInt();
                }
            } else if (parameter->getAnnotations()->getSingle(ParseTCAnnotations::tc_init_val)) {
                // TODO: Process tc_init_val.
            } else {
                /* If a parameter is not annoated by tc_init or tc_numel then it is emitted as
                constructor parameters.*/
                IR::TCKey *key = new IR::TCKey(0, parameter->type->width_bits(),
                                               parameter->toString(), "param"_cs, false);
                keys.push_back(key);
                if (exp->is<IR::Constant>()) {
                    key->setValue(exp->to<IR::Constant>()->asInt64());
                }
            }
        }
    }
    return keys;
}

cstring ConvertToBackendIR::getControlPathKeyAnnotation(const IR::StructField *field) {
    cstring annoName;
    auto annotation = field->getAnnotations()->annotations.at(0);
    if (annotation->name == ParseTCAnnotations::tc_key ||
        annotation->name == ParseTCAnnotations::tc_data_scalar) {
        annoName = annotation->name;
    } else if (annotation->name == ParseTCAnnotations::tc_data) {
        annoName = "param"_cs;
    }
    return annoName;
}

ConvertToBackendIR::CounterType ConvertToBackendIR::toCounterType(const int type) {
    if (type == 0)
        return CounterType::PACKETS;
    else if (type == 1)
        return CounterType::BYTES;
    else if (type == 2)
        return CounterType::PACKETS_AND_BYTES;

    BUG("Unknown counter type %1%", type);
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::processCounterControlPathKeys(
    const IR::Type_Struct *extern_control_path, const IR::Type_Extern *extn,
    const IR::Declaration_Instance *decl) {
    safe_vector<const IR::TCKey *> keys;
    auto typeArg = decl->arguments->at(decl->arguments->size() - 1)->expression->to<IR::Constant>();
    CounterType type = toCounterType(typeArg->asInt());
    int kId = 1;
    for (auto field : extern_control_path->fields) {
        /* If there is no annotation to control path key, ignore the key.*/
        if (field->getAnnotations()->annotations.size() != 1) {
            continue;
        }
        cstring annoName = getControlPathKeyAnnotation(field);

        if (field->toString() == "pkts") {
            if (type == CounterType::PACKETS || type == CounterType::PACKETS_AND_BYTES) {
                auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
                keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
            }
            continue;
        }
        if (field->toString() == "bytes") {
            if (type == CounterType::BYTES || type == CounterType::PACKETS_AND_BYTES) {
                auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
                keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
            }
            continue;
        }

        /* If the field is of Type_Name example 'T'*/
        if (field->type->is<IR::Type_Name>()) {
            auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
            keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
        } else {
            IR::TCKey *key =
                new IR::TCKey(kId++, field->type->width_bits(), field->toString(), annoName, true);
            keys.push_back(key);
        }
    }
    return keys;
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::processExternControlPath(
    const IR::Type_Extern *extn, const IR::Declaration_Instance *decl, cstring eName) {
    safe_vector<const IR::TCKey *> keys;
    auto find = ControlStructPerExtern.find(eName);
    if (find != ControlStructPerExtern.end()) {
        auto extern_control_path = ControlStructPerExtern[eName];
        if (eName == "DirectCounter" || eName == "Counter") {
            keys = processCounterControlPathKeys(extern_control_path, extn, decl);
            return keys;
        }

        int kId = 1;
        for (auto field : extern_control_path->fields) {
            /* If there is no annotation to control path key, ignore the key.*/
            if (field->getAnnotations()->annotations.size() != 1) {
                continue;
            }
            cstring annoName = getControlPathKeyAnnotation(field);

            /* If the field is of Type_Name example 'T'*/
            if (field->type->is<IR::Type_Name>()) {
                auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
                keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
            } else {
                IR::TCKey *key = new IR::TCKey(kId++, field->type->width_bits(), field->toString(),
                                               annoName, true);
                keys.push_back(key);
            }
        }
    }
    return keys;
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::HandleTypeNameStructField(
    const IR::StructField *field, const IR::Type_Extern *extn, const IR::Declaration_Instance *decl,
    int &kId, cstring annoName) {
    safe_vector<const IR::TCKey *> keys;
    auto type_extern_params = extn->getTypeParameters()->parameters;
    for (unsigned itr = 0; itr < type_extern_params.size(); itr++) {
        if (type_extern_params.at(itr)->toString() == field->type->toString()) {
            auto decl_type = typeMap->getType(decl, true);
            auto ts = decl_type->to<IR::Type_SpecializedCanonical>();
            auto param_val = ts->arguments->at(itr);

            /* If 'T' is of Type_Struct, extract all fields of structure*/
            if (auto param_struct = param_val->to<IR::Type_Struct>()) {
                for (auto f : param_struct->fields) {
                    IR::TCKey *key =
                        new IR::TCKey(kId++, f->type->width_bits(), f->toString(), annoName, true);
                    keys.push_back(key);
                }
            } else {
                IR::TCKey *key = new IR::TCKey(kId++, param_val->width_bits(), field->toString(),
                                               annoName, true);
                keys.push_back(key);
            }
            break;
        }
    }
    return keys;
}

bool ConvertToBackendIR::hasExecuteMethod(const IR::Type_Extern *extn) {
    for (auto gd : *extn->getDeclarations()) {
        if (!gd->getNode()->is<IR::Method>()) {
            continue;
        }
        auto method = gd->getNode()->to<IR::Method>();
        const IR::Annotation *execAnnotation =
            method->getAnnotations()->getSingle(ParseTCAnnotations::tc_md_exec);
        if (execAnnotation) {
            return true;
        }
    }
    return false;
}

/* Process each declaration instance of externs*/
void ConvertToBackendIR::postorder(const IR::Declaration_Instance *decl) {
    auto decl_type = typeMap->getType(decl, true);
    if (auto ts = decl_type->to<IR::Type_SpecializedCanonical>()) {
        if (auto extn = ts->baseType->to<IR::Type_Extern>()) {
            auto eName = ts->baseType->toString();
            auto find = ControlStructPerExtern.find(eName);
            if (find == ControlStructPerExtern.end()) {
                return;
            }
            IR::TCExtern *externDefinition;
            auto instance = new struct ExternInstance();
            instance->instance_name = decl->toString();

            auto constructorKeys = processExternConstructor(extn, decl, instance);

            // Get Control Path information if specified for extern.
            auto controlKeys = processExternControlPath(extn, decl, eName);

            bool has_exec_method = hasExecuteMethod(extn);

            /* If the extern info is already present, add new instance
               Or else create new extern info.*/
            auto iterator = externsInfo.find(eName);
            if (iterator == externsInfo.end()) {
                struct ExternBlock *eb = new struct ExternBlock();
                if (eName == "DirectCounter") {
                    eb->externId = "0x1A000000"_cs;
                } else if (eName == "Counter") {
                    eb->externId = "0x19000000"_cs;
                } else {
                    externCount += 1;
                    std::stringstream value;
                    value << "0x" << std::hex << externCount;
                    eb->externId = value.str();
                }
                eb->permissions = processExternPermission(extn);
                eb->no_of_instances += 1;
                externsInfo.emplace(eName, eb);

                instance->instance_id = eb->no_of_instances;
                eb->eInstance.push_back(instance);

                externDefinition =
                    new IR::TCExtern(eb->externId, eName, pipelineName, eb->no_of_instances,
                                     eb->permissions, has_exec_method);
                tcPipeline->addExternDefinition(externDefinition);
            } else {
                auto eb = externsInfo[eName];
                externDefinition = ((IR::TCExtern *)tcPipeline->getExternDefinition(eName));
                externDefinition->numinstances = ++eb->no_of_instances;
                instance->instance_id = eb->no_of_instances;
                eb->eInstance.push_back(instance);
            }
            IR::TCExternInstance *tcExternInstance =
                new IR::TCExternInstance(instance->instance_id, instance->instance_name,
                                         instance->is_num_elements, instance->num_elements);
            if (controlKeys.size() != 0) {
                tcExternInstance->addControlPathKeys(controlKeys);
            }
            if (constructorKeys.size() != 0) {
                tcExternInstance->addConstructorKeys(constructorKeys);
            }
            externDefinition->addExternInstance(tcExternInstance);
        }
    }
}

void ConvertToBackendIR::postorder(const IR::Type_Struct *ts) {
    auto struct_name = ts->externalName();
    auto cp = "tc_ControlPath_";
    if (struct_name.startsWith(cp)) {
        auto type_extern_name = struct_name.substr(strlen(cp));
        ControlStructPerExtern.emplace(type_extern_name, ts);
    }
}

void ConvertToBackendIR::postorder(const IR::P4Program *p) {
    if (p != nullptr) {
        tcPipeline->setPipelineName(pipelineName);
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
                        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
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

cstring ConvertToBackendIR::getExternId(cstring externName) const {
    for (auto e : externsInfo) {
        if (e.first == externName) return e.second->externId;
    }
    return ""_cs;
}

unsigned ConvertToBackendIR::getExternInstanceId(cstring externName, cstring instanceName) const {
    for (auto e : externsInfo) {
        if (e.first == externName) {
            for (auto eI : e.second->eInstance) {
                if (eI->instance_name == instanceName) {
                    return eI->instance_id;
                }
            }
        }
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
            } else if (matchTypeInfo->name.name == "range" ||
                       matchTypeInfo->name.name == "rangelist" ||
                       matchTypeInfo->name.name == "optional") {
                tableMatchType = TC::TERNARY_TYPE;
            } else {
                ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
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
                } else if (matchTypeInfo->name.name == "range" ||
                           matchTypeInfo->name.name == "rangelist" ||
                           matchTypeInfo->name.name == "optional") {
                    keyMatchType = TC::TERNARY_TYPE;
                    ternaryKey++;
                } else {
                    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
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

bool ConvertToBackendIR::checkParameterDirection(const IR::TCAction *tcAction) {
    bool dirParam = false;
    for (auto actionParam : tcAction->actionParams) {
        if (actionParam->getDirection() != TC::NONE) {
            dirParam = true;
            break;
        }
    }
    return dirParam;
}

}  // namespace P4::TC
