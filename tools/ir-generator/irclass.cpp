// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "irclass.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

#include "lib/enumerator.h"
#include "lib/exceptions.h"

namespace P4 {

const char *IrClass::indent = "    ";
IrNamespace &IrNamespace::global() {
    static IrNamespace irn({}, {});
    return irn;
}
static const LookupScope utilScope(nullptr, "Util"_cs);
static const NamedType srcInfoType(Util::SourceInfo(), &utilScope, "SourceInfo"_cs);

IrField *IrField::srcInfoField() {
    static IrField irf(Util::SourceInfo(), &srcInfoType, "srcInfo"_cs, {},
                       IrField::Inline | IrField::Optional);
    return &irf;
}

IrClass *IrClass::nodeClass() {
    static IrClass irc(NodeKind::Abstract, "Node"_cs, {IrField::srcInfoField()});
    return &irc;
}
IrClass *IrClass::vectorClass() {
    static IrClass irc(NodeKind::Template, "Vector"_cs);
    return &irc;
}
IrClass *IrClass::namemapClass() {
    static IrClass irc(NodeKind::Template, "NameMap"_cs);
    return &irc;
}
IrClass *IrClass::nodemapClass() {
    static IrClass irc(NodeKind::Template, "NodeMap"_cs);
    return &irc;
}
IrClass *IrClass::ideclaration() {
    static IrClass irc(NodeKind::Interface, "IDeclaration"_cs);
    return &irc;
}
IrClass *IrClass::indexedVectorClass() {
    static IrClass irc(NodeKind::Template, "IndexedVector"_cs);
    return &irc;
}
bool LineDirective::inhibit = false;

////////////////////////////////////////////////////////////////////////////////////

IrNamespace *IrNamespace::get(IrNamespace *parent, cstring name) {
    IrNamespace *ns = parent ? parent : &global();
    IrNamespace *rv = ns->children[name];
    if (!rv) ns->children[name] = rv = new IrNamespace(ns, name);
    return rv;
}

void IrNamespace::add_class(IrClass *cl) {
    IrNamespace *ns = cl->containedIn ? cl->containedIn : &global();
    if (ns->classes[cl->name])
        throw Util::CompilationError("%1%: Duplicate class name", cl->name);
    else
        ns->classes[cl->name] = cl;
}

std::ostream &operator<<(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) out << ns->parent << ns->name << "::";
    return out;
}

void enter_namespace(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) {
        enter_namespace(out, ns->parent);
        out << "namespace " << ns->name << " {" << std::endl;
    }
}

void exit_namespace(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) {
        exit_namespace(out, ns->parent);
        out << "}  // namespace " << ns->name << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////////

/* sort class definitions so defs come before uses */
void IrDefinitions::toposort() {
    std::vector<IrElement *> sorted;
    std::map<const IrClass *, IrClass *> classes;

    auto visit = [&](const IrClass *cl) -> void {
        auto do_visit = [&](const auto &self, const IrClass *cl) -> void {
            auto it = classes.find(cl);
            if (it != classes.end()) {
                auto *cl = it->second;
                classes.erase(it);
                self(self, cl->concreteParent);
                for (auto *p : cl->parentClasses) self(self, p);
                sorted.push_back(cl);
            }
        };
        do_visit(do_visit, cl);
    };

    for (auto *el : elements)
        if (auto *cl = el->to<IrClass>()) classes.emplace(cl, cl);

    for (auto *el : elements) {
        if (auto *cl = el->to<IrClass>())
            visit(cl);
        else
            sorted.push_back(el);
    }
    elements = std::move(sorted);
}

Util::Enumerator<IrClass *> *IrDefinitions::getClasses() const {
    return Util::enumerate(elements)->as<IrClass *>()->where(
        [](IrClass *e) { return e != nullptr; });
}

namespace {

void generateHeaderPreamble(std::ostream &out) {
    out << "#include <functional>\n"
        << "#include <map>\n\n"
        << "#include \"lib/big_int.h\"        // IWYU pragma: keep\n"
        << "// Special IR classes and types\n"
        << "#include \"ir/dbprint.h\"         // IWYU pragma: keep\n"
        << "#include \"ir/id.h\"              // IWYU pragma: keep\n"
        << "#include \"ir/indexed_vector.h\"  // IWYU pragma: keep\n"
        << "#include \"ir/namemap.h\"         // IWYU pragma: keep\n"
        << "#include \"ir/node.h\"            // IWYU pragma: keep\n"
        << "#include \"ir/nodemap.h\"         // IWYU pragma: keep\n"
        << "#include \"ir/vector.h\"          // IWYU pragma: keep\n"
        << "#include \"lib/ordered_map.h\"    // IWYU pragma: keep\n"
        << std::endl
        << "namespace P4 {\n"
        << std::endl
        << "class JSONLoader;\n"
        << "using NodeFactoryFn = IR::Node*(*)(JSONLoader&);\n"
        << std::endl
        << "namespace IR {\n"
        << "extern std::map<cstring, NodeFactoryFn> unpacker_table;\n"
        << "using namespace P4::literals;\n"
        << "}\n";

    out << "extern template class IR::Vector<IR::Node>;\n"
        << "extern template class IR::IndexedVector<IR::Node>;\n"
        << "}  // namespace P4\n";
}

void generateImplementationPreamble(std::ostream &out, const std::string &header) {
    out << "#include \"" << header << "\"\n"
        << "#include \"ir/ir-inline.h\"\n"
        << "#include \"ir/json_loader.h\"\n"
        << "#include \"lib/algorithm.h\"\n"
        << "#include \"lib/log.h\"\n\n"
        << "using namespace P4;\n";
}

void generateTemplates(const IrClass *cls, std::ostream &out, std::ostream &impl) {
    if (cls->needVector || cls->needIndexedVector) {
        impl << "template class IR::Vector<IR::" << cls->containedIn << cls->name << ">;"
             << std::endl;
        out << "extern template class IR::Vector<IR::" << cls->containedIn << cls->name << ">;"
            << std::endl;
    }
    if (cls->needIndexedVector) {
        impl << "template class IR::IndexedVector<IR::" << cls->containedIn << cls->name << ">;"
             << std::endl;
        out << "extern template class IR::IndexedVector<IR::" << cls->containedIn << cls->name
            << ">;" << std::endl;
    }
}

}  // namespace

void IrDefinitions::generateFactory(std::ostream &impl) const {
    impl << "std::map<cstring, NodeFactoryFn> IR::unpacker_table = {\n";

    bool first = true;
    for (auto cls : *getClasses()) {
        if (cls->kind == NodeKind::Concrete) {
            if (first)
                first = false;
            else
                impl << ",\n";
            impl << "{\"" << cls->name << "\"_cs, NodeFactoryFn(&IR::";
            if (cls->containedIn && cls->containedIn->name) impl << cls->containedIn->name << "::";
            impl << cls->name << "::fromJSON)}";
        }
    }
    impl << " };\n" << std::endl;

    impl << "template class IR::Vector<IR::Node>;\n"
         << "template class IR::IndexedVector<IR::Node>;\n";
}

void IrDefinitions::generate(std::ostream &t, std::ostream &out, std::ostream &impl) const {
    out << "#ifndef IR_GENERATED_H_\n#define IR_GENERATED_H_\n";
    generateHeaderPreamble(out);
    generateImplementationPreamble(impl, "ir/ir-generated.h");
    generateFactory(impl);
    out << "namespace P4 {\n";
    for (auto cls : *getClasses()) generateTemplates(cls, out, impl);
    out << "}  // namespace P4\n";
    for (auto e : elements) {
        e->generate_hdr(out);
        e->generate_impl(impl);
    }
    out << "#endif  // IR_GENERATED_H_\n";
    generateTree(t);
}

void IrDefinitions::generateTree(std::ostream &t) const {
    t << "#pragma once\n"
      << "#include <cstdint>\n"
      << "#include \"lib/rtti.h\"\n";

    t << "#define IRNODE_ALL_SUBCLASSES_AND_DIRECT_AND_INDIRECT_BASES(M, T, D, B, ...) \\"
      << std::endl;
    for (auto cls : *getClasses())
        if (cls->kind != NodeKind::Interface) cls->generateTreeMacro(t);

    t << "T(Vector<IR::Node>, D(Node), ##__VA_ARGS__) \\" << std::endl;
    t << "T(IndexedVector<IR::Node>, "
         "D(Vector<IR::Node>) "
         "B(Node), ##__VA_ARGS__) \\"
      << std::endl;
    for (auto cls : *getClasses()) {
        if (cls->needVector || cls->needIndexedVector)
            t << "T(Vector<IR::" << cls->containedIn << cls->name
              << ">, D(Node), "
                 "##__VA_ARGS__) \\"
              << std::endl;
        if (cls->needIndexedVector)
            // We generate IndexedVector only if needed; we expect users won't use
            // these if they don't want to place them in fields.
            t << "T(IndexedVector<IR::" << cls->containedIn << cls->name
              << ">, "
                 "D(Vector<IR::"
              << cls->containedIn << cls->name
              << ">) "
                 "B(Node), ##__VA_ARGS__) \\"
              << std::endl;
        if (cls->needNameMap) BUG("visitable (non-inline) NameMap not yet implemented");
        if (cls->needNodeMap) BUG("visitable (non-inline) NodeMap not yet implemented");
    }
    t << std::endl;

    t << "namespace P4::IR {" << std::endl;

    // Emit forward declarations
    for (auto *cls : *getClasses()) {
        enter_namespace(t, cls->containedIn);
        cls->declare(t);
        exit_namespace(t, cls->containedIn);
    }

    t << std::endl;

    // Emit node kinds
    // TODO: Probably it would make sense to topo-sort the IDs to optimize the
    // comparison trees generated by a compiler
    t << "enum class NodeKind : RTTI::TypeId {\n"
      << "  Auto = 0,\n"
      << "  INode = 1,\n"
      << "  Node = 2,\n";

    unsigned nkId = 3;
    auto *irNamespace = IrNamespace::get(nullptr, "IR"_cs);
    for (auto *cls : *getClasses())
        t << "  " << cls->qualified_name(irNamespace).replace("::", "_") << " = " << nkId++
          << ",\n";

    // Add some specials:
    t << "  IDeclaration = " << nkId++ << ",\n";
    t << "  VectorBase = " << nkId++ << "\n"
      << "};\n";
    t << "enum class NodeDiscriminator : RTTI::TypeId {\n"
      << "  NodeT = UINT64_C(1),\n"
      << "  VectorT = UINT64_C(1),\n"
      << "  IndexedVectorT = UINT64_C(2),\n"
      << "  Auto = UINT64_C(0xFF)\n"
      << "};\n"
      << " inline bool operator==(RTTI::TypeId lhs, NodeKind rhs) { return lhs == "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator==(NodeKind lhs, RTTI::TypeId rhs) { return RTTI::TypeId(lhs) == "
         "rhs; }\n"
      << " inline bool operator!=(RTTI::TypeId lhs, NodeKind rhs) { return lhs != "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator!=(NodeKind lhs, RTTI::TypeId rhs) { return RTTI::TypeId(lhs) != "
         "rhs; }\n"
      << " inline bool operator==(RTTI::TypeId lhs, NodeDiscriminator rhs) { return lhs == "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator==(NodeDiscriminator lhs, RTTI::TypeId rhs) { return "
         "RTTI::TypeId(lhs) == rhs; }\n"
      << " inline bool operator!=(RTTI::TypeId lhs, NodeDiscriminator rhs) { return lhs != "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator!=(NodeDiscriminator lhs, RTTI::TypeId rhs) { return "
         "RTTI::TypeId(lhs) != rhs; }\n";
    t << "}  // namespace P4::IR" << std::endl;
}

namespace {

// Each definition file owns its declarations and explicit template instantiations.
// Keep the path (not just its basename): extensions may use the same file names.
struct GeneratedPart {
    std::ostringstream header;
    std::ostringstream implementation;
    std::set<std::string> dependencies;
};

std::filesystem::path absolutePath(const std::filesystem::path &path) {
    return std::filesystem::absolute(path).lexically_normal();
}

void writeGeneratedFile(const std::filesystem::path &path, const std::string &contents) {
    // Resolve reset directives in the final file, rather than in a concatenated
    // temporary file. Otherwise an edit in one input shifts other files' #lines.
    std::istringstream input(contents);
    std::ostringstream output;
    std::string line;
    unsigned lineNumber = 1;
    while (std::getline(input, line)) {
        if (line == "#")
            output << "#line " << lineNumber + 1 << " \"" << path.generic_string() << "\"\n";
        else
            output << line << '\n';
        ++lineNumber;
    }
    auto text = output.str();
    std::ifstream previous(path, std::ios::binary);
    if (previous && std::string(std::istreambuf_iterator<char>(previous), {}) == text) return;
    previous.close();

    std::filesystem::create_directories(path.parent_path());
    auto temporary = path;
    temporary += ".tmp";
    std::ofstream file;
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.open(temporary, std::ios::binary | std::ios::trunc);
    file << text;
    file.close();
    std::filesystem::rename(temporary, path);
}

}  // namespace

void IrDefinitions::generateSplit(const std::string &sourceRoot, const std::string &outputRoot,
                                  const std::vector<std::string> &inputs) const {
    const auto root = absolutePath(sourceRoot);
    const auto output = absolutePath(outputRoot);
    std::map<std::string, GeneratedPart> parts;
    std::vector<std::string> order;
    auto relativeName = [&](const std::filesystem::path &path) {
        auto relative = absolutePath(path).lexically_relative(root);
        if (relative.empty() || *relative.begin() == "..")
            throw std::runtime_error("IR input is outside the source root: " + path.string());
        return relative.generic_string();
    };
    for (const auto &input : inputs) {
        auto name = relativeName(input);
        if (!parts.try_emplace(name).second)
            throw std::runtime_error("Duplicate IR input: " + name);
        order.push_back(name);
    }
    auto owner = [&](const IrElement *element) {
        return relativeName(element->srcInfo.toPosition().fileName.c_str());
    };

    for (const auto &name : order) {
        auto &part = parts.at(name);
        part.header << "#pragma once\n#include \"ir/ir-generated-common.h\"\n";
        generateImplementationPreamble(part.implementation, name + ".h");
    }
    for (const auto *element : elements) {
        if (auto *include = element->to<IrInclude>()) {
            auto name = relativeName(root / include->file.c_str());
            if (!parts.count(name))
                throw std::runtime_error(owner(element) +
                                         ": included IR file is not an input: " + name);
            auto &part = parts.at(owner(element));
            auto &stream = include->impl ? part.implementation : part.header;
            stream << "#include \"" << name << ".h\"\n";
            if (!include->impl) part.dependencies.insert(name);
        }
    }

    // A header dependency cycle cannot be fixed by sorting classes in the
    // combined input. Diagnose it before producing partially updated outputs.
    std::set<std::string> visiting, visited;
    auto visit = [&](const auto &self, const std::string &name) -> void {
        if (visited.count(name)) return;
        if (!visiting.insert(name).second)
            throw std::runtime_error("Cyclic IR header dependency involving " + name);
        for (const auto &dependency : parts.at(name).dependencies) self(self, dependency);
        visiting.erase(name);
        visited.insert(name);
    };
    for (const auto &name : order) visit(visit, name);

    // Inheritance requires complete types. Catch missing dependency declarations
    // here instead of reporting confusing errors in the generated C++.
    auto dependsOn = [&](const auto &self, const std::string &name,
                         const std::string &dependency) -> bool {
        if (name == dependency) return true;
        for (const auto &direct : parts.at(name).dependencies)
            if (self(self, direct, dependency)) return true;
        return false;
    };
    for (auto *cls : *getClasses()) {
        for (const auto *parent : cls->parentClasses) {
            if (parent->srcInfo.isValid() && !dependsOn(dependsOn, owner(cls), owner(parent)))
                throw std::runtime_error(owner(cls) + ": missing IR header dependency on " +
                                         owner(parent));
        }
        auto &part = parts.at(owner(cls));
        part.header << "namespace P4 {\n";
        generateTemplates(cls, part.header, part.implementation);
        part.header << "}  // namespace P4\n";
    }
    for (const auto *element : elements) {
        auto &part = parts.at(owner(element));
        element->generate_hdr(part.header);
        element->generate_impl(part.implementation);
    }

    std::ostringstream common, umbrella, registry, tree;
    common << "#pragma once\n";
    generateHeaderPreamble(common);
    umbrella << "#pragma once\n";
    for (const auto &name : order) umbrella << "#include \"" << name << ".h\"\n";
    generateImplementationPreamble(registry, "ir/ir-generated.h");
    generateFactory(registry);
    generateTree(tree);
    for (const auto &name : order) {
        const auto &part = parts.at(name);
        writeGeneratedFile(output / (name + ".h"), part.header.str());
        writeGeneratedFile(output / (name + ".cpp"), part.implementation.str());
    }
    writeGeneratedFile(output / "ir/ir-generated-common.h", common.str());
    writeGeneratedFile(output / "ir/ir-generated.h", umbrella.str());
    writeGeneratedFile(output / "ir/ir-generated.cpp", registry.str());
    writeGeneratedFile(output / "ir/gen-tree-macro.h", tree.str());
}

void IrClass::generateTreeMacro(std::ostream &out) const {
    auto *p = this;
    for (; p && p != nodeClass(); p = p->getParent()) {
        out << "  ";
    }
    BUG_CHECK(p != nullptr, "Falled out of the class hierarchy");
    out << "M(";
    const char *sep = "";
    for (p = this; p; p = p->getParent()) {
        out << sep << p->containedIn << p->name;
        sep = *sep ? ") B(" : ", D(";
    }
    out << "), ##__VA_ARGS__) \\" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////

void EmitBlock::generate_hdr(std::ostream &out) const {
    if (!impl) out << LineDirective(srcInfo, +1) << body << LineDirective();
}
void EmitBlock::generate_impl(std::ostream &out) const {
    if (impl) out << LineDirective(srcInfo, +1) << body << LineDirective();
}

////////////////////////////////////////////////////////////////////////////////////

void IrMethod::generate_proto(std::ostream &out, bool fullname, bool defaults) const {
    if (rtype) {
        if (rtype->isResolved()) out << "const ";
        out << rtype->toString() << " ";
        if (rtype->isResolved()) out << "*";
    }
    if (fullname && !isFriend) out << "IR::" << clss->containedIn << clss->name << "::";
    out << name << "(";
    const char *sep = "";
    for (auto *a : args) {
        out << sep;
        a->generate(out, false);
        if (a->initializer && defaults) out << " = " << a->initializer;
        sep = ", ";
    }
    out << ")" << (isConst ? " const" : "");
}

void IrMethod::generate_hdr(std::ostream &out) const {
    if (srcInfo.isValid()) out << LineDirective(srcInfo);
    out << IrClass::indent;
    if (isStatic) out << "static ";
    if (isVirtual) out << "virtual ";
    if (isFriend) out << "friend ";
    generate_proto(out, false, isUser);
    if (isOverride) out << " override";
    if (inImpl || !body)
        out << ';';
    else
        out << ' ' << body;
    out << std::endl;
    if (name == "visit_children") {
        out << IrClass::indent;
        generate_proto(out, false, isUser);
        out << " const";
        if (isOverride) out << " override";
        out << ";" << std::endl;
    } else if (name == "node_type_name") {
        out << LineDirective(srcInfo) << IrClass::indent << "static " << rtype->toString()
            << " static_type_name() " << body << std::endl;
    }
    if (srcInfo.isValid()) out << LineDirective();
}

void IrMethod::generate_impl(std::ostream &out) const {
    if (!inImpl || !body) return;
    out << LineDirective(srcInfo);
    generate_proto(out, true, false);
    out << " " << body << std::endl;
    if (name == "visit_children") {
        out << LineDirective(srcInfo);
        generate_proto(out, true, false);
        out << " const " << body << std::endl;
    }
    if (srcInfo.isValid()) out << LineDirective();
}

////////////////////////////////////////////////////////////////////////////////////

void IrApply::generate_hdr(std::ostream &out) const {
    out << IrClass::indent << "IRNODE_DECLARE_APPLY_OVERLOAD(" << clss->name << ")" << std::endl;
}

void IrApply::generate_impl(std::ostream &out) const {
    out << "IRNODE_DEFINE_APPLY_OVERLOAD(" << clss->containedIn << clss->name << ", , )"
        << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////

void IrClass::declare(std::ostream &out) const { out << "class " << name << ";" << std::endl; }

std::string IrClass::fullName() const {
    std::stringstream tmp;
    tmp << "IR::" << containedIn << name;
    return tmp.str();
}

cstring IrNamespace::qualified_name(const IrNamespace *in) const {
    cstring rv = name ? name : "IR"_cs;
    if (parent) {
        for (auto i = in; i; i = i->parent) {
            auto sym = i->lookupChild(name);
            if (sym && this != sym) break;
            if (parent == i) return rv;
        }
        rv = parent->qualified_name(in) + "::"_cs + rv;
    }
    return rv;
}

cstring IrClass::qualified_name(const IrNamespace *in) const {
    cstring rv = name;
    if (containedIn) {
        for (auto i = in; i; i = i->parent) {
            auto sym = i->lookupClass(name);
            if (sym && this != sym) break;
            if (containedIn == i) return rv;
        }
        rv = containedIn->qualified_name(in) + "::"_cs + rv;
    }
    return rv;
}

void IrClass::generate_hdr(std::ostream &out) const {
    if (kind != NodeKind::Nested) {
        out << "namespace P4::IR {" << std::endl;
        enter_namespace(out, containedIn);
    }
    for (auto cblock : comments) cblock->generate_hdr(out);
    out << "class " << name;

    bool concreteParent = false;
    for (auto p : parentClasses) {
        if (p->kind != NodeKind::Interface) concreteParent = true;
    }

    const char *sep = " : ";
    if (!concreteParent && kind != NodeKind::Nested) {
        if (kind != NodeKind::Interface)
            out << sep << "public Node";
        else
            out << sep << "public virtual INode";
        sep = ", ";
    }
    for (auto p : parentClasses) {
        out << sep << "public ";
        if (p->kind == NodeKind::Interface) out << "virtual ";
        out << p->qualified_name(containedIn);
        sep = ", ";
    }

    out << " {" << std::endl;

    auto access = IrElement::Private;
    for (auto e : elements) {
        if (e->access != access) out << (access = e->access);
        e->generate_hdr(out);
    }

    if (kind != NodeKind::Interface && kind != NodeKind::Nested)
        out << indent << "IRNODE" << (kind == NodeKind::Abstract ? "_ABSTRACT" : "") << "_SUBCLASS("
            << name << ")" << std::endl;

    auto *irNamespace = IrNamespace::get(nullptr, "IR"_cs);
    if (kind != NodeKind::Nested) {
        out << indent << "DECLARE_TYPEINFO_WITH_TYPEID(" << name
            << ", NodeKind::" << qualified_name(irNamespace).replace("::", "_");
        if (!concreteParent) out << ", " << (kind != NodeKind::Interface ? "Node" : "INode");
        for (const auto *p : parentClasses) out << ", " << p->qualified_name(containedIn);
        out << ");" << std::endl;
    }

    out << "};" << std::endl;
    if (kind != NodeKind::Nested) {
        exit_namespace(out, containedIn);
        out << "}  // namespace P4::IR" << std::endl;
    }
}

void IrClass::generate_impl(std::ostream &out) const {
    for (auto e : elements) e->generate_impl(out);
}

void IrClass::computeConstructorArguments(IrClass::ctor_args_t &args) const {
    if (concreteParent == nullptr) {
        if (kind != NodeKind::Nested) {
            // direct descendant of Node, add srcInfo
            args.emplace_back(IrField::srcInfoField(), IrClass::nodeClass());
        }
    } else {
        concreteParent->computeConstructorArguments(args);
    }

    for (auto field : *getFields())
        if (!field->isStatic && (!field->initializer || field->optional))
            args.emplace_back(field, this);
}

int IrClass::generateConstructor(const ctor_args_t &arglist, const IrMethod *user,
                                 unsigned skip_opt) {
    // Constructor call has the following shape
    // class T : public P, public I1, public I2 {
    //     F1 f1;
    //     F2 f2;
    //     T(PF1 pf1, PF2 pf2, F1 f1, F2 f2) :
    //         P(pf1, pf2), f1(f1), f2(f2)
    //     { validate(); }
    // }
    int optargs = 0;
    std::stringstream body;
    const char *sep = ":\n    ";
    auto parent = getParent() ? getParent()->qualified_name(containedIn) : cstring();
    const char *end_parent = "";
    for (auto &arg : arglist) {
        if (arg.first->optional && (skip_opt & (1U << optargs++))) continue;
        if (arg.second == this) {
            body << end_parent;
            end_parent = "";
        } else if (!parent.isNullOrEmpty()) {
            body << sep << parent;
            parent = ""_cs;
            sep = "(";
            end_parent = ")";
        }
        body << sep << arg.first->name;
        if (arg.second == this) body << "(" << arg.first->name << ")";
        sep = ", ";
    }

    body << end_parent << std::endl << indent << "{";
    if (user)
        body << '\n'
             << LineDirective(user->getSourceInfo()) << user->body << '\n'
             << LineDirective() << indent;
    if (kind != NodeKind::Nested) body << " validate(); ";
    body << "}";
    auto ctor = new IrMethod(name, body.str());
    ctor->clss = this;
    optargs = 0;
    for (auto a : arglist) {
        if (a.first->optional && (skip_opt & (1U << optargs++))) continue;
        ctor->args.push_back(a.first);
    }

    if (kind == NodeKind::Abstract) ctor->access = IrElement::Protected;
    ctor->inImpl = false;
    elements.push_back(ctor);
    return optargs;
}

Util::Enumerator<IrField *> *IrClass::getFields() const {
    return Util::enumerate(elements)->as<IrField *>()->where(
        [](IrField *f) { return f && !f->isStatic; });
}

Util::Enumerator<IrMethod *> *IrClass::getUserMethods() const {
    return Util::enumerate(elements)->as<IrMethod *>()->where(
        [](IrElement *e) { return e != nullptr; });
}

bool IrClass::hasNoDirective(cstring feature) const {
    return Util::enumerate(elements)
        ->where([](IrElement *el) { return el->is<IrNo>(); })
        ->where([feature](IrElement *el) { return el->to<IrNo>()->text == feature; })
        ->any();
}

bool IrClass::shouldSkip(cstring feature) const {
    // Skip if there is a '#no' directive.
    if (hasNoDirective(feature)) {
        return true;
    }
    // Do not skip if the feature is 'validate'.
    if (feature == "validate") {
        return false;
    }
    // Also skip if the user provided an implementation manually
    bool provided = Util::enumerate(elements)
                        ->where([feature](IrElement *e) {
                            const auto *m = e->to<IrMethod>();
                            return m && m->name == feature;
                        })
                        ->any();
    return provided;
}

void IrClass::resolve() {
    if (resolved) return;
    resolved = true;
    for (auto s : parents) {
        const IrClass *p = s->resolve(containedIn);
        if (p == nullptr) throw Util::CompilationError("Could not find class %1%", s);
        if (p->kind != NodeKind::Interface) {
            if (concreteParent == nullptr)
                concreteParent = p;
            else
                BUG("Class %1% has more than 1 non-interface parent: %2% and %3%", this,
                    concreteParent, p);
        }
        parentClasses.push_back(p);
    }
    generateMethods();
    for (auto e : elements) e->resolve();
}

////////////////////////////////////////////////////////////////////////////////////
//
void IrEnumType::generate_hdr(std::ostream &out) const {
    out << "enum " << (isClassEnum ? "class " : "") << name << "\n"
        << LineDirective(srcInfo, +1) << body << ";\n"
        << LineDirective();
}

////////////////////////////////////////////////////////////////////////////////////

void IrField::resolve() { resolveType(type); }

void IrField::resolveType(const Type *type) {
    auto tmpl = dynamic_cast<const TemplateInstantiation *>(type);
    const IrClass *cls = type->resolve(clss ? clss->containedIn : nullptr);
    if (cls) {
        if (tmpl) {
            if (cls->kind != NodeKind::Template)
                throw Util::CompilationError("Template args with non-template class %1%", cls);
            unsigned tmpl_args = (cls == IrClass::nodemapClass() ? 2 : 1);
            if (tmpl->args.size() < tmpl_args)
                throw Util::CompilationError("Wrong number of args for template %1%", cls);
            for (unsigned i = 0; i < tmpl_args; i++) {
                if (auto acl = tmpl->args[i]->resolve(clss ? clss->containedIn : nullptr)) {
                    if (cls == IrClass::vectorClass())
                        acl->needVector = true;
                    else if (cls == IrClass::indexedVectorClass())
                        acl->needIndexedVector = true;
                    else if (cls == IrClass::namemapClass() && !isInline)
                        acl->needNameMap = true;
                    else if (cls == IrClass::nodemapClass() && !isInline)
                        acl->needNodeMap = true;
                } else {
                    throw Util::CompilationError(
                        "%1% template argment %2% is not "
                        "an IR class",
                        cls->name, tmpl->args[i]);
                }
            }
        } else if (cls->kind == NodeKind::Template) {
            throw Util::CompilationError("No args for template %1%", cls);
        }
    }
}

void IrField::generate(std::ostream &out, bool asField) const {
    if (asField) {
        out << IrClass::indent;
        if (isStatic) out << "static ";
        if (isConst) out << "const ";
    }

    const IrClass *cls = type->resolve(clss ? clss->containedIn : nullptr);
    if (cls != nullptr && !isInline) out << "const ";
    out << type->toString();
    if (cls != nullptr && !isInline) out << "*";
    out << " " << name << type->declSuffix();
    if (asField) {
        if (!isStatic) {
            if (!initializer.isNullOrEmpty())
                out << " = " << initializer;
            else if (cls != nullptr && !isInline)
                out << " = nullptr";
        }
        out << ";";
        out << std::endl;
    }
}

void IrField::generate_impl(std::ostream &) const {
    if (!isStatic) return;
    // FIXME -- for now statics are manually generated elsewhere
}

////////////////////////////////////////////////////////////////////////////////////

void IrVariantField::resolve() {
    for (const Type *type : *types) resolveType(type);
}

void IrVariantField::generate(std::ostream &out, bool asField) const {
    if (asField) {
        out << IrClass::indent << "using " << name << "_variant = std::variant<";
        bool first = true;
        for (const Type *type : *types) {
            if (!first) out << ", ";

            // FIXME: Support variant of IR node pointers
            // const IrClass *cls = type->resolve(clss ? clss->containedIn : nullptr);
            // if (cls != nullptr) out << "const ";
            out << type->toString();
            // if (cls != nullptr) out << "*";
            first = false;
        }
        out << ">;" << std::endl << IrClass::indent;

        if (isStatic) out << "static ";
        if (isConst) out << "const ";
    }

    out << name << "_variant " << name;

    if (asField) {
        if (!isStatic && !initializer.isNullOrEmpty()) out << " = " << initializer;

        out << ";";
        out << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////////

void ConstFieldInitializer::generate_hdr(std::ostream &out) const {
    out << IrClass::indent;
    if (name == "precedence")
        out << "int getPrecedence() const override { return " << initializer << "; }" << std::endl;
    else if (name == "stringOp")
        out << "cstring getStringOp() const override { return cstring(" << initializer << "); }"
            << std::endl;
    else
        throw Util::CompilationError("Unexpected constant field %1%", this);
}

}  // namespace P4
