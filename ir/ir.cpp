#include "ir/ir.h"
#include "ir/parameterSubstitution.h"

namespace IR {

const ID ParserState::accept = ID("accept");
const ID ParserState::reject = ID("reject");
const ID ParserState::start = ID("start");
const ID Declaration_Errors::EID = ID("error");
const ID Declaration_MatchKind::MKID = ID("match_kind");
const cstring TableProperties::actionsPropertyName = "actions";
const cstring TableProperties::keyPropertyName = "key";
const cstring TableProperties::defaultActionPropertyName = "default_action";
const cstring IApply::applyMethodName = "apply";
const cstring SwitchStatement::default_label = "default";
const cstring P4Program::main = "main";

int IR::Declaration::nextId = 0;

Util::Enumerator<const IR::IDeclaration*>* P4Control::getDeclarations() const
{ return stateful.valueEnumerator()->as<const IDeclaration*>(); }

const Type_Method* P4Control::getConstructorMethodType() const {
    return new Type_Method(Util::SourceInfo(), getTypeParameters(), type, constructorParams);
}

const Type_Method* P4Parser::getConstructorMethodType() const {
    return new Type_Method(Util::SourceInfo(), getTypeParameters(), type, constructorParams);
}

const Type_Method* Type_Package::getConstructorMethodType() const {
    return new Type_Method(Util::SourceInfo(), getTypeParameters(), this, constructorParams);
}

void P4Program::checkDeclarations() const {
    std::unordered_map<cstring, ID> seen;
    for (auto decl : *declarations) {
        CHECK_NULL(decl);
        if (!decl->is<IDeclaration>())
            BUG("Program element is not a declaration %1%", decl);
        IR::ID name = decl->to<IR::IDeclaration>()->getName();

        // allow duplicate declarations for 'error' and 'match_kind'
        if (name == IR::Declaration_Errors::EID ||
            name == IR::Declaration_MatchKind::MKID)
            continue;
        auto f = seen.find(name.name);
        if (f != seen.end()) {
            ::error("Duplicate declaration of %1%: previous declaration at %2%",
                    name, f->second.srcInfo);
        }
        seen.emplace(name.name, name);
    }
}

// TODO: this is a linear scan, which could be too slow.
Util::Enumerator<const IR::IDeclaration*>* IGeneralNamespace::getDeclsByName(cstring name) const {
    std::function<bool(const IDeclaration*)> filter = [name](const IDeclaration* d)
            { return name == d->getName().name; };
    return getDeclarations()->where(filter);
}

void IGeneralNamespace::checkDuplicateDeclarations() const {
    std::unordered_map<cstring, ID> seen;
    auto decls = getDeclarations();
    for (auto decl : *decls) {
        IR::ID name = decl->getName();
        auto f = seen.find(name.name);
        if (f != seen.end()) {
            ::error("Duplicate declaration of %1%: previous declaration at %2%",
                    name, f->second.srcInfo);
        }
        seen.emplace(name.name, name);
    }
}

bool Type_Stack::sizeKnown() const { return size->is<Constant>(); }

int Type_Stack::getSize() const {
    if (!sizeKnown())
        BUG("%1%: Size not yet known", size);
    auto cst = size->to<IR::Constant>();
    if (!cst->fitsInt()) {
        ::error("Index too large: %1%", cst);
        return 0;
    }
    return cst->asInt();
}

const Method* Type_Extern::lookupMethod(cstring name, int paramCount) const {
    const Method* result = nullptr;
    size_t upc = paramCount;

    bool reported = false;
    for (auto m : *methods) {
        if (m->name == name && m->getParameterCount() == upc) {
            if (result == nullptr) {
                result = m;
            } else {
                ::error("Ambiguous method %1% with %2% parameters", name, paramCount);
                if (!reported) {
                    ::error("Candidate is %1%", result);
                    reported = true;
                }
                ::error("Candidate is %1%", m);
                return nullptr;
            }
        }
    }
    return result;
}

const Type_Method*
Type_Parser::getApplyMethodType() const {
    return new Type_Method(Util::SourceInfo(), typeParams, nullptr, applyParams);
}

const Type_Method*
Type_Control::getApplyMethodType() const {
    return new Type_Method(Util::SourceInfo(), typeParams, nullptr, applyParams);
}

const Type_Method*
Type_Table::getApplyMethodType() const {
    if (applyMethod == nullptr) {
        // Let's synthesize a new type for the return
        auto actions = container->properties->getProperty(IR::TableProperties::actionsPropertyName);
        if (actions == nullptr) {
            ::error("Table %1% does not contain a list of actions", container);
            return nullptr;
        }
        if (!actions->value->is<IR::ActionList>())
            BUG("Action property is not an IR::ActionList, but %1%",
                                    actions);
        auto alv = actions->value->to<IR::ActionList>();
        auto fields = new IR::NameMap<IR::StructField, ordered_map>();
        auto hit = new IR::StructField(Util::SourceInfo(), IR::Type_Table::hit,
                                       IR::Annotations::empty, IR::Type_Boolean::get());
        fields->addUnique(hit->name, hit);
        auto label = new IR::StructField(Util::SourceInfo(), IR::Type_Table::action_run,
                                         IR::Annotations::empty,
                                         new IR::Type_ActionEnum(Util::SourceInfo(), alv));
        fields->addUnique(label->name, label);
        auto rettype = new IR::Type_Struct(Util::SourceInfo(), ID(container->name),
                                           IR::Annotations::empty, std::move(*fields));
        applyMethod = new IR::Type_Method(Util::SourceInfo(), new TypeParameters(),
                                          rettype, container->parameters);
    }
    return applyMethod;
}

void ActionList::checkDuplicates() const {
    std::set<cstring> found;
    for (auto ale : *actionList) {
        if (found.count(ale->getName().name) > 0)
            ::error("Duplicate action name in table: %1%", ale);
        found.emplace(ale->getName().name);
    }
}

class ActionArgSetup : public Transform {
    /* FIXME -- use ParameterSubstitution for this somehow? */
    std::map<cstring, const IR::Expression *>    args;
    const IR::Node *preorder(IR::PathExpression *pe) override {
        if (args.count(pe->path->name))
            return args.at(pe->path->name);
        return pe; }

 public:
    void add_arg(const IR::ActionArg *a) { args[a->name] = a; }
    void add_arg(cstring name, const IR::Expression *e) { args[name] = e; }
};

ActionFunction::ActionFunction(const P4Action *ac, const Vector<Expression> *args) {
    srcInfo = ac->srcInfo;
    name = ac->name;
    ActionArgSetup setup;
    size_t arg_idx = 0;
    for (auto param : *ac->parameters->getEnumerator()) {
        if (param->direction == Direction::None) {
            auto arg = new IR::ActionArg(param->srcInfo, param->type, param->name);
            setup.add_arg(arg);
            this->args.push_back(arg);
        } else {
            if (!args || arg_idx >= args->size())
                error("%s: Not enough args for %s", args->srcInfo, ac);
            else
                setup.add_arg(param->name, args->at(arg_idx++)); } }
    if (arg_idx != (args ? args->size(): 0))
        error("%s: Too many args for %s", args->srcInfo, ac);
    for (auto stmt : *ac->body)
        if (auto assign = stmt->to<IR::AssignmentStatement>()) {
            auto prim = new IR::Primitive("modify_field", assign->left, assign->right);
            action.push_back(prim->apply(setup));
        } else if (auto mc = stmt->to<IR::MethodCallStatement>()) {
            BUG("extern method call %1% not yet implemented", mc);
        } else {
            BUG("un-handled statment %1% in action", stmt); }
}

V1Table::V1Table(const P4Table *tc) {
    srcInfo = tc->srcInfo;
    name = tc->name;
    for (auto prop : *tc->properties->getEnumerator()) {
        if (auto key = prop->value->to<Key>()) {
            auto reads = new IR::Vector<IR::Expression>();
            for (auto el : *key->keyElements) {
                reads->push_back(el->expression);
                reads_types.push_back(el->matchType->path->name); }
            this->reads = reads;
        } else if (auto actions = prop->value->to<ActionList>()) {
            for (auto el : *actions->actionList)
                this->actions.push_back(el->name->path->name); } }
}

void InstantiatedBlock::instantiate(std::vector<const CompileTimeValue*> *args) {
    CHECK_NULL(args);
    auto it = args->begin();
    for (auto p : *getConstructorParameters()->getEnumerator()) {
        constantValue.emplace(p, *it);
        ++it;
    }
}

const IR::CompileTimeValue* InstantiatedBlock::getParameterValue(cstring paramName) const {
    auto param = getConstructorParameters()->getDeclByName(paramName);
    BUG_CHECK(param != nullptr, "No parameter named %1%", paramName);
    BUG_CHECK(param->is<IR::Parameter>(), "No parameter named %1%", paramName);
    return getValue(param->getNode());
}

}  // namespace IR
