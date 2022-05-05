#include "dpdkHelpers.h"
#include "ir/dbprint.h"
#include "printUtils.h"
#include "dpdkAsmOpt.h"
using namespace DBPrint;

static constexpr unsigned DEFAULT_LEARNER_TABLE_SIZE = 0x10000;
static constexpr unsigned DEFAULT_LEARNER_TABLE_TIMEOUT = 120;
ordered_map<cstring, cstring> DPDK::ShortenTokenLength::origNameMap = {};
auto& origNameMap =  DPDK::ShortenTokenLength::origNameMap;

void add_space(std::ostream &out, int size) {
    out << std::setfill(' ') << std::setw(size) << " ";
}

void add_comment(std::ostream& out, cstring str, cstring sep = "") {
    if (origNameMap.count(str)) {
        out<<sep<<";oldname:"<<origNameMap.at(str)<<"\n";
    }
}

std::ostream &IR::DpdkAsmProgram::toSpec(std::ostream &out) const {
    for (auto l : globals) {
        l->toSpec(out) << std::endl;
    }
    out << std::endl;
    for (auto h : headerType) {
        add_comment(out, h->name.toString());
        h->toSpec(out) << std::endl;
    }
    for (auto s : structType) {
        add_comment(out, s->name.toString());
        s->toSpec(out) << std::endl;
    }
    for (auto s : externDeclarations) {
        add_comment(out, s->name.toString());
        s->toSpec(out) << std::endl;
    }
    for (auto a : actions) {
        add_comment(out, a->name.toString());
        a->toSpec(out) << std::endl << std::endl;
    }
    for (auto t : tables) {
        add_comment(out, t->name);
        t->toSpec(out) << std::endl << std::endl;
    }
    for (auto s : selectors) {
        add_comment(out, s->name);
        s->toSpec(out) << std::endl;
    }
    for (auto s : learners) {
        add_comment(out, s->name);
        s->toSpec(out) << std::endl;
    }
    for (auto s : statements) {
        s->toSpec(out) << std::endl;
    }
    return out;
}

std::ostream &IR::DpdkAsmStatement::toSpec(std::ostream &out) const {
    BUG("asm statement %1% not implemented", this);
    return out;
}

std::ostream &IR::DpdkDeclaration::toSpec(std::ostream &out) const {
    // TBD
    return out;
}

std::ostream &IR::DpdkExternDeclaration::toSpec(std::ostream &out) const {
    if (DPDK::toStr(this->getType()) == "Register") {
        auto args = this->arguments;
        if (args->size() == 0) {
            ::error(ErrorType::ERR_INVALID,
                    "Register extern declaration %1% must contain a size parameter\n",
                this->Name());
        } else {
            auto size = args->at(0)->expression;
            auto init_val = args->size() == 2? args->at(1)->expression: nullptr;
            auto regDecl = new IR::DpdkRegisterDeclStatement(this->Name(), size, init_val);
            regDecl->toSpec(out) << std::endl;
        }
    } else if (DPDK::toStr(this->getType()) == "Counter") {
        auto args = this->arguments;
        unsigned value = 0;
        if (args->size() < 2) {
            ::error(ErrorType::ERR_INVALID,
                    "Counter extern declaration %1% must contain 2 parameters\n", this->Name());
        } else {
            auto n_counters = args->at(0)->expression;
            auto counter_type = args->at(1)->expression;
            if (counter_type->is<IR::Constant>())
                value = counter_type->to<IR::Constant>()->asUnsigned();
            if (value == 2) {
                /* For PACKETS_AND_BYTES counter type, two regarray declarations are emitted and
                   the counter name is suffixed with _packets and _bytes */
                auto regDecl = new IR::DpdkRegisterDeclStatement(this->Name() + "_packets",
                                   n_counters, new IR::Constant(0));
                regDecl->toSpec(out) << std::endl << std::endl;
                regDecl = new IR::DpdkRegisterDeclStatement(this->Name() + "_bytes", n_counters,
                                                            new IR::Constant(0));
                regDecl->toSpec(out) << std::endl;
            } else {
                auto regDecl = new IR::DpdkRegisterDeclStatement(this->Name(), n_counters,
                                                                 new IR::Constant(0));
                regDecl->toSpec(out) << std::endl;
            }
        }
    } else if (DPDK::toStr(this->getType()) == "Meter") {
        auto args = this->arguments;
        if (args->size() < 2) {
            ::error(ErrorType::ERR_INVALID,
                    "Meter extern declaration %1% must contain a size parameter"
                    " and meter type parameter", this->Name());
        } else {
            auto n_meters = args->at(0)->expression;
            auto metDecl = new IR::DpdkMeterDeclStatement(this->Name(), n_meters);
            metDecl->toSpec(out) << std::endl;
        }
    }
    return out;
}

std::ostream &IR::DpdkHeaderType::toSpec(std::ostream &out) const {
    out << "struct " << name << " {" << std::endl;
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        add_comment(out, (*it)->name.toString(), "\t");
        if (auto t = (*it)->type->to<IR::Type_Bits>())
            out << "\tbit<" << t->width_bits() << ">";
        else if (auto t = (*it)->type->to<IR::Type_Name>())
            out << "\t" << t->path->name;
        else if ((*it)->type->to<IR::Type_Boolean>())
            out << "\tbool";
        else if (auto t = (*it)->type->to<IR::Type_Varbits>())
            out << "\tvarbit<" << t->size << ">";
        else {
            BUG("Unsupported type: %1% ", *it);
        }
        out << " " << (*it)->externalName();
        out << std::endl;
    }
    out << "}" << std::endl;
    return out;
}

std::ostream &IR::DpdkStructType::toSpec(std::ostream &out) const {
    if (getAnnotations()->getSingle("__packet_data__")) {
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            add_comment(out, (*it)->name.toString());
            if (auto t = (*it)->type->to<IR::Type_Name>()) {
                out << "header " << (*it)->name << " instanceof "
                    << t->path->name;
            } else if (auto t = (*it)->type->to<IR::Type_Stack>()) {
                if (!t->elementType->is<IR::Type_Name>())
                    BUG("%1% Unsupported type", t->elementType);
                cstring type_name = t->elementType->to<IR::Type_Name>()->path->name;
                if (!t->size->is<IR::Constant>()) {
                    BUG("Header stack index in %1% must be compile-time constant", t);
                }
                for (auto i = 0; i < t->size->to<IR::Constant>()->value; i++) {
                    out << "header " << (*it)->name << "_" << i << " instanceof "
                        << type_name << std::endl;
                }
            } else {
                BUG("Unsupported type %1%", *it);
            }
            out << std::endl;
        }
    } else {
        out << "struct " << name << " {" << std::endl;
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            add_comment(out, (*it)->name.toString(), "\t");
            if (auto t = (*it)->type->to<IR::Type_Bits>())
                out << "\tbit<" << t->width_bits() << ">";
            else if (auto t = (*it)->type->to<IR::Type_Name>()) {
                if (t->path->name == "error") {
                    out << "\tbit<8>";
                } else {
                    out << "\t" << t->path;
                }
            } else if ((*it)->type->to<IR::Type_Error>()) {
                // DPDK implements error type as bit<8>
                out << "\tbit<8>";
            } else if ((*it)->type->to<IR::Type_Boolean>()) {
                // DPDK implements bool as bit<8>
                out << "\tbit<8>";
            } else {
                std::cout << (*it)->type->node_type_name() << std::endl;
                BUG("Unsupported type");
            }
            out << " " << (*it)->externalName();
            out << std::endl;
        }
        out << "}" << std::endl;
        if (getAnnotations()->getSingle("__metadata__")) {
            out << "metadata instanceof " << name << std::endl;
        }
    }
    return out;
}

std::ostream &IR::DpdkListStatement::toSpec(std::ostream &out) const {
    out << "apply {" << std::endl;
    for (auto s : statements) {
        out << "\t";
        s->toSpec(out);
        if (!s->to<IR::DpdkLabelStatement>())
            out << std::endl;
    }
    out << "}" << std::endl;
    return out;
}

std::ostream &IR::DpdkApplyStatement::toSpec(std::ostream &out) const {
    out << "table " << table;
    return out;
}

std::ostream &IR::DpdkMirrorStatement::toSpec(std::ostream &out) const {
    out << "mirror " << DPDK::toStr(slotId) << " " << DPDK::toStr(sessionId);
    return out;
}

std::ostream &IR::DpdkLearnStatement::toSpec(std::ostream &out) const {
    out << "learn " << action << " ";
    if (argument)
        out << DPDK::toStr(argument) << " ";
    out << DPDK::toStr(timeout);
    return out;
}

std::ostream &IR::DpdkEmitStatement::toSpec(std::ostream &out) const {
    out << "emit " << DPDK::toStr(header);
    return out;
}

std::ostream &IR::DpdkExtractStatement::toSpec(std::ostream &out) const {
    out << "extract " << DPDK::toStr(header);
    if (length)
        out << " " << DPDK::toStr(length);
    return out;
}

std::ostream &IR::DpdkLookaheadStatement::toSpec(std::ostream &out) const {
    out << "lookahead " << DPDK::toStr(header);
    return out;
}

std::ostream &IR::DpdkJmpStatement::toSpec(std::ostream &out) const {
    out << instruction << " " << label;
    return out;
}

std::ostream& IR::DpdkJmpHeaderStatement::toSpec(std::ostream& out) const {
    out << instruction << " " << label << " " << DPDK::toStr(header);
    return out;
}

std::ostream& IR::DpdkJmpActionStatement::toSpec(std::ostream& out) const {
    out << instruction << " " << label << " " << action;
    return out;
}

std::ostream& IR::DpdkJmpCondStatement::toSpec(std::ostream& out) const {
    out << instruction << " " << label << " " << DPDK::toStr(src1)
        << " " << DPDK::toStr(src2);
    return out;
}

std::ostream& IR::DpdkBinaryStatement::toSpec(std::ostream& out) const {
    BUG_CHECK(dst->equiv(*src1), "The first source field %1% in a binary operation"
            "must be the same as the destination field %2% to be supported by DPDK",
            src1, dst);
    out << instruction << " " << DPDK::toStr(dst)
        << " " << DPDK::toStr(src2);
    return out;
}

std::ostream& IR::DpdkUnaryStatement::toSpec(std::ostream& out) const {
    out << instruction << " " << DPDK::toStr(dst) << " " << DPDK::toStr(src);
    return out;
}

std::ostream &IR::DpdkRxStatement::toSpec(std::ostream &out) const {
    out << "rx " << DPDK::toStr(port);
    return out;
}

std::ostream &IR::DpdkTxStatement::toSpec(std::ostream &out) const {
    out << "tx " << DPDK::toStr(port);
    return out;
}

std::ostream &IR::DpdkExternObjStatement::toSpec(std::ostream &out) const {
    out << "extern_obj ";
    return out;
}

std::ostream &IR::DpdkExternFuncStatement::toSpec(std::ostream &out) const {
    out << "extern_func ";
    return out;
}

std::ostream &IR::DpdkReturnStatement::toSpec(std::ostream &out) const {
    out << "return ";
    return out;
}

std::ostream &IR::DpdkRearmStatement::toSpec(std::ostream &out) const {
    out << "rearm";
    if (timeout)
        out << " " << DPDK::toStr(timeout);
    return out;
}

std::ostream &IR::DpdkRecirculateStatement::toSpec(std::ostream &out) const {
    out << "recirculate";
    return out;
}

std::ostream &IR::DpdkRecircidStatement::toSpec(std::ostream &out) const {
    out << "recircid " << DPDK::toStr(pass);
    return out;
}

std::ostream &IR::DpdkLabelStatement::toSpec(std::ostream &out) const {
    out << label << " :";
    return out;
}

std::ostream &IR::DpdkTable::toSpec(std::ostream &out) const {
    out << "table " << name << " {" << std::endl;
    if (match_keys) {
        out << "\tkey {" << std::endl;
        for (auto key : match_keys->keyElements) {
            out << "\t\t" << DPDK::toStr(key->expression) << " ";
            if ((key->matchType)->toString() == "ternary") {
                out << "wildcard" << std::endl;
            } else {
                out << DPDK::toStr(key->matchType) << std::endl;
            }
        }
        out << "\t}" << std::endl;
    }
    out << "\tactions {" << std::endl;
    for (auto action : actions->actionList) {
        if (action->expression->toString() == "NoAction") {
            out << "\t\tNoAction";
        } else {
            out << "\t\t" << DPDK::toStr(action->expression);
        }
        if (action->annotations->getAnnotation("tableonly"))
            out << " @tableonly";
        if (action->annotations->getAnnotation("defaultonly"))
            out << " @defaultonly";
        out << std::endl;
    }
    out << "\t}" << std::endl;

    if (default_action->toString() == "NoAction")
        out << "\tdefault_action NoAction";
    else
        out << "\tdefault_action " << DPDK::toStr(default_action);
    if (default_action->to<IR::MethodCallExpression>()->arguments->size() ==
        0) {
        out << " args none ";
    } else {
        out << " args ";
        auto mce = default_action->to<IR::MethodCallExpression>();
        auto earg = mce->arguments->at(0)->expression;
        if (earg->is<IR::ListExpression>()) {
            auto paramCount = earg->to<IR::ListExpression>()->components.size();
            std::cout<<"Earg : "<<earg<<std::endl;
            for (unsigned i = 0; i < paramCount; i++) {
                if (earg->to<IR::ListExpression>()->components.at(i)->is<IR::Constant>()) {
                    auto val = earg->to<IR::ListExpression>()->
                               components.at(i)->to<IR::Constant>()->asUnsigned();
                    out << default_action_paraList.parameters.at(i)->toString() << " ";
                    out << "0x" << std::hex << val << " ";
                } else if (earg->to<IR::ListExpression>()->components.at(i)->
                           is<IR::BoolLiteral>()) {
                    earg->dbprint(std::cout);
                    auto val = earg->to<IR::ListExpression>()->
                               components.at(i)->to<IR::BoolLiteral>()->value;
                    out << default_action_paraList.parameters.at(i)->toString() << " ";
                    out << "0x" << std::hex << val << " ";
                } else {
                    BUG("Unsupported parameter type in default action in DPDK Target");
                }
            }
        }
    }
    auto def = properties->getProperty("default_action");
    if (def->isConstant)
        out <<"const";
    out << std::endl;
    if (auto psa_implementation =
            properties->getProperty("psa_implementation")) {
        out << "\taction_selector " << DPDK::toStr(psa_implementation->value)
            << std::endl;
    }
    if (auto size = properties->getProperty("size")) {
        out << "\tsize " << DPDK::toStr(size->value) << "" << std::endl;
    } else {
        out << "\tsize 0x10000" << std::endl;
    }
    out << "}" << std::endl;
    return out;
}

std::ostream &IR::DpdkSelector::toSpec(std::ostream &out) const {
    out << "selector " << name << " {" << std::endl;
    out << "\tgroup_id " << DPDK::toStr(group_id) << std::endl;
    if (selectors) {
        out << "\tselector {" << std::endl;
        for (auto key : selectors->keyElements) {
            out << "\t\t" << DPDK::toStr(key->expression) << std::endl;
        }
        out << "\t}" << std::endl;
    }
    out << "\tmember_id " << DPDK::toStr(member_id) << std::endl;
    out << "\tn_groups_max " << n_groups_max << std::endl;
    out << "\tn_members_per_group_max " << n_members_per_group_max << std::endl;
    out << "}" << std::endl;
    return out;
}

std::ostream& IR::DpdkLearner::toSpec(std::ostream& out) const {
    out << "learner " << name << " {" << std::endl;
    if (match_keys) {
        out << "\tkey {" << std::endl;
        for (auto key : match_keys->keyElements) {
            out << "\t\t" << DPDK::toStr(key->expression) << std::endl;
        }
    }
    out << "\t}" << std::endl;
    out << "\tactions {" << std::endl;
    for (auto action : actions->actionList) {
        out << "\t\t" << DPDK::toStr(action->expression);
        if (action->getAnnotation("tableonly"))
            out << " @tableonly";
        if (action->getAnnotation("defaultonly"))
            out << " @defaultonly";
        out << std::endl;
    }
    out << "\t}" << std::endl;

    out << "\tdefault_action " << DPDK::toStr(default_action);
    if (default_action->to<IR::MethodCallExpression>()->arguments->size() ==
        0) {
        out << " args none ";
    } else {
        BUG("non-zero default action arguments not supported yet");
    }
    out << std::endl;
    if (auto size = properties->getProperty("size")) {
        out << "\tsize " << DPDK::toStr(size->value) << "" << std::endl;
    } else {
        out << "\tsize " << DEFAULT_LEARNER_TABLE_SIZE << std::endl;
    }

    // The initial timeout values
    out << "\ttimeout {" << std::endl;
    out << "\t\t" << 60 << std::endl;
    out << "\t\t" << 120 << std::endl;
    out << "\t\t" << 180 << "\n\t\t}";
    out << "\n}" << std::endl;
    return out;
}

std::ostream &IR::DpdkAction::toSpec(std::ostream &out) const {
    out << "action " << name.toString() << " args ";

    if (para.parameters.size() == 0)
        out << "none ";

    for (auto p : para.parameters) {
        out << "instanceof " << p->type << " ";
        if (p != para.parameters.back())
            out << " ";
    }
    out << "{" << std::endl;
    for (auto i : statements) {
        out << "\t";
        i->toSpec(out);
        if (!i->to<IR::DpdkLabelStatement>())
            out << std::endl;
    }
    out << "\treturn" << std::endl;
    out << "}";

    return out;
}

std::ostream &IR::DpdkChecksumAddStatement::toSpec(std::ostream &out) const {
    out << "ckadd "
        << "h.cksum_state." << intermediate_value << " " << DPDK::toStr(field);
    return out;
}

std::ostream &IR::DpdkChecksumSubStatement::toSpec(std::ostream &out) const {
    out << "cksub "
        << "h.cksum_state." << intermediate_value << " " << DPDK::toStr(field);
    return out;
}

std::ostream &IR::DpdkChecksumClearStatement::toSpec(std::ostream &out) const {
    out << "mov "
        << "h.cksum_state." << intermediate_value << " " << "0x0";
    return out;
}

std::ostream &IR::DpdkGetHashStatement::toSpec(std::ostream &out) const {
    out << "hash " << hash << " " << DPDK::toStr(dst) << " ";
    if (auto l = fields->to<IR::ListExpression>()) {
        if (l->components.size() == 1) {
            out << " " << DPDK::toStr(l->components.at(0));
            out << " " << DPDK::toStr(l->components.at(0));
        } else {
            out << " " << DPDK::toStr(l->components.at(0));
            out << " " << DPDK::toStr(l->components.at(l->components.size() - 1));
        }
    } else {
        ::error(ErrorType::ERR_INVALID,
                "%1%: get_hash's arg is not a ListExpression.", this);
    }
    out << "";
    return out;
}

std::ostream &IR::DpdkGetChecksumStatement::toSpec(std::ostream &out) const {
    out << "mov " << DPDK::toStr(dst) << " "
        << "h.cksum_state." << intermediate_value;
    return out;
}

std::ostream &IR::DpdkCastStatement::toSpec(std::ostream &out) const {
    out << "mov " << DPDK::toStr(dst) << " " << DPDK::toStr(src);
    return out;
}

std::ostream &IR::DpdkVerifyStatement::toSpec(std::ostream &out) const {
    out << "verify " << DPDK::toStr(condition) << " " << DPDK::toStr(error);
    return out;
}

std::ostream &IR::DpdkMeterDeclStatement::toSpec(std::ostream &out) const {
    add_comment(out, meter);
    out << "metarray " << meter << " size " << DPDK::toStr(size);
    return out;
}

std::ostream &IR::DpdkMeterExecuteStatement::toSpec(std::ostream &out) const {
    out << "meter " << meter << " " << DPDK::toStr(index) << " " << DPDK::toStr(length);
    out << " " << DPDK::toStr(color_in) << " " << DPDK::toStr(color_out);
    return out;
}

/* DPDK target uses Registers for implementing using Counters, atomic register add instruction
   is used for incrementing the counter. Packet counters are incremented by packet length
   specified as parameter and byte counters are incremente by 1 */
std::ostream &IR::DpdkCounterCountStatement::toSpec(std::ostream &out) const {
    add_comment(out, counter);
    out << "regadd " << counter << " " << DPDK::toStr(index) << " ";
    if (incr)
        out << DPDK::toStr(incr);
    else
        out << "1";
    return out;
}

std::ostream &IR::DpdkRegisterDeclStatement::toSpec(std::ostream &out) const {
    add_comment(out, reg);
    out << "regarray " << reg << " size " << DPDK::toStr(size) << " initval ";
    if (init_val)
        out << DPDK::toStr(init_val);
    else
        out << "0";
    return out;
}

std::ostream &IR::DpdkRegisterReadStatement::toSpec(std::ostream &out) const {
    out << "regrd " << DPDK::toStr(dst) << " " << reg << " "
        << DPDK::toStr(index);
    return out;
}

std::ostream &IR::DpdkRegisterWriteStatement::toSpec(std::ostream &out) const {
    out << "regwr " << reg << " " << DPDK::toStr(index) << " "
        << DPDK::toStr(src);
    return out;
}

std::ostream& IR::DpdkValidateStatement::toSpec(std::ostream& out) const {
    out << "validate " << DPDK::toStr(header);
    return out;
}

std::ostream& IR::DpdkInvalidateStatement::toSpec(std::ostream& out) const {
    out << "invalidate " << DPDK::toStr(header);
    return out;
}

std::ostream& IR::DpdkDropStatement::toSpec(std::ostream& out) const {
    out << "drop";
    return out;
}
