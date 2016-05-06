#include "moveConstructors.h"

namespace P4 {

struct ConstructorMap {
    // Maps a constructor to the temporary used to hold its value.
    ordered_map<const IR::ConstructorCallExpression*, cstring> tmpName;

    void clear() { tmpName.clear(); }
    void add(const IR::ConstructorCallExpression* expression, cstring name)
    { CHECK_NULL(expression); tmpName[expression] = name; }
    bool empty() const { return tmpName.empty(); }
};

namespace {
// Converts constructor call expressions that appear
// within the bodies of P4Parser and P4Control blocks
// into Declaration_Instance.  This is needed for implementing
// copy-in/copy-out in inlining, since
// constructed objects do not have assignment operations.
// For example:
// extern T {}
// control c()(T t) {  apply { ... } }
// control d() {
//    c(T()) cinst;
//    apply { ... }
// }
// is converted to
// extern T {}
// control c()(T t) {  apply { ... } }
// control d() {
//    T() tmp;
//    c(tmp) cinst;
//    apply { ... }
// }
class MoveConstructorsImpl : public Transform {
    enum class Region {
        InParserStateful,
        InControlStateful,
        InBody,
        Outside
    };

    ReferenceMap*   refMap;
    ConstructorMap  cmap;
    Region          convert;

 public:
    explicit MoveConstructorsImpl(ReferenceMap* refMap) :
            refMap(refMap), convert(Region::Outside) {}

    const IR::Node* preorder(IR::P4Parser* parser) override {
        cmap.clear();
        convert = Region::InParserStateful;
        visit(parser->stateful);
        convert = Region::InBody;
        visit(parser->states);
        convert = Region::Outside;
        prune();
        return parser;
    }

    const IR::Node* preorder(IR::IndexedVector<IR::Declaration>* declarations) override {
        if (convert != Region::InParserStateful)
            return declarations;

        bool changes = false;
        auto result = new IR::Vector<IR::Declaration>();
        for (auto s : *declarations) {
            visit(s);
            for (auto e : cmap.tmpName) {
                auto cce = e.first;
                auto decl = new IR::Declaration_Instance(
                    cce->srcInfo, e.second, cce->constructedType,
                    cce->arguments, IR::Annotations::empty, nullptr);
                result->push_back(decl);
                changes = true;
            }
            result->push_back(s);
            cmap.clear();
        }
        prune();
        if (changes)
            return result;
        return declarations;
    }

    const IR::Node* postorder(IR::P4Parser* parser) override {
        if (cmap.empty())
            return parser;
        auto newDecls = new IR::IndexedVector<IR::Declaration>();
        for (auto e : cmap.tmpName) {
            auto cce = e.first;
            auto decl = new IR::Declaration_Instance(
                cce->srcInfo, e.second, cce->constructedType,
                cce->arguments, IR::Annotations::empty, nullptr);
            newDecls->push_back(decl);
        }
        newDecls->append(*parser->stateful);
        auto result = new IR::P4Parser(parser->srcInfo, parser->name, parser->type,
                                       parser->constructorParams, newDecls, parser->states);
        return result;
    }

    const IR::Node* preorder(IR::P4Control* control) override {
        cmap.clear();
        convert = Region::InControlStateful;
        auto newDecls = new IR::IndexedVector<IR::Declaration>();
        bool changes = false;
        for (auto decl : *control->stateful) {
            visit(decl);
            for (auto e : cmap.tmpName) {
                auto cce = e.first;
                auto inst = new IR::Declaration_Instance(
                    cce->srcInfo, e.second, cce->constructedType,
                    cce->arguments, IR::Annotations::empty, nullptr);
                newDecls->push_back(inst);
                changes = true;
            }
            newDecls->push_back(decl);
            cmap.clear();
        }
        convert = Region::InBody;
        visit(control->body);
        convert = Region::Outside;
        prune();
        if (changes) {
            auto result = new IR::P4Control(control->srcInfo, control->name, control->type,
                                            control->constructorParams, newDecls,
                                            control->body);
            return result;
        }
        return control;
    }

    const IR::Node* postorder(IR::P4Control* control) override {
        if (cmap.empty())
            return control;
        auto newDecls = new IR::IndexedVector<IR::Declaration>();
        for (auto e : cmap.tmpName) {
            auto cce = e.first;
            auto decl = new IR::Declaration_Instance(
                cce->srcInfo, e.second, cce->constructedType,
                cce->arguments, IR::Annotations::empty, nullptr);
            newDecls->push_back(decl);
        }
        for (auto s : *control->stateful)
            newDecls->push_back(s);
        auto result = new IR::P4Control(control->srcInfo, control->name, control->type,
                                        control->constructorParams, newDecls,
                                        control->body);
        return result;
    }

    const IR::Node* preorder(IR::P4Table* table) override
    { prune(); return table; }  // skip

    const IR::Node* postorder(IR::ConstructorCallExpression* expression) override {
        if (convert == Region::Outside)
            return expression;
        auto tmpvar = refMap->newName("tmp");
        auto tmpref = new IR::PathExpression(IR::ID(expression->srcInfo, tmpvar));
        cmap.add(expression, tmpvar);
        return tmpref;
    }
};
}  // namespace

MoveConstructors::MoveConstructors(bool isv1) {
    ReferenceMap* refMap = new ReferenceMap();

    passes.emplace_back(new P4::ResolveReferences(refMap, isv1));
    passes.emplace_back(new MoveConstructorsImpl(refMap));
}

}  // namespace P4
