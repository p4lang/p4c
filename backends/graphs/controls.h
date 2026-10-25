/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_GRAPHS_CONTROLS_H_
#define BACKENDS_GRAPHS_CONTROLS_H_

#include "graphs.h"

namespace P4::graphs {

class ControlGraphs : public Graphs {
 public:
    class ControlStack {
     public:
        Graph *pushBack(Graph &currentSubgraph, const cstring &name);
        Graph *popBack();
        Graph *getSubgraph() const;
        cstring getName(const cstring &name) const;
        bool isEmpty() const;

     private:
        std::vector<cstring> names{};
        std::vector<Graph *> subgraphs{};
    };

    ControlGraphs(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, std::filesystem::path graphsDir);

    bool preorder(const IR::PackageBlock *block) override;
    bool preorder(const IR::ControlBlock *block) override;
    bool preorder(const IR::P4Control *cont) override;
    bool preorder(const IR::BlockStatement *statement) override;
    bool preorder(const IR::IfStatement *statement) override;
    bool preorder(const IR::SwitchStatement *statement) override;
    bool preorder(const IR::MethodCallStatement *statement) override;
    bool preorder(const IR::BaseAssignmentStatement *statement) override;
    bool preorder(const IR::ReturnStatement *) override;
    bool preorder(const IR::ExitStatement *) override;
    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Key *key) override;
    bool preorder(const IR::P4Action *action) override;

    std::vector<Graph *> controlGraphsArray{};

 private:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const cstring graphsDir;
    Parents return_parents{};
    /// We keep a stack of subgraphs; every time we visit a control, we create a
    /// new subgraph and push it to the stack; this new graph becomes the
    /// "current graph" to which we add vertices (e.g. tables).
    ControlStack controlStack{};
    std::optional<cstring> instanceName{};
};

}  // namespace P4::graphs

#endif /* BACKENDS_GRAPHS_CONTROLS_H_ */
