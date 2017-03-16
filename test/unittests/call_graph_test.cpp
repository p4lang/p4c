/*
Copyright 2016 VMware Inc.

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

#include "test.h"
#include "../../frontends/p4/callGraph.h"

namespace Test {

class TestCallGraph : public TestBase {
    template <class T>
    void sameSet(std::unordered_set<T> &set, std::vector<T> vector) {
        ASSERT_EQ(set.size(), vector.size());
        for (T v : vector)
            ASSERT_EQ(set.find(v) != set.end(), true);
    }

    template <class T>
    void sameSet(std::set<T> &set, std::vector<T> vector) {
        ASSERT_EQ(set.size(), vector.size());
        for (T v : vector)
            ASSERT_EQ(set.find(v) != set.end(), true);
    }

    int test1() {
        P4::CallGraph<char> acyclic("acyclic");
        // a->b->c
        // \_____^

        acyclic.calls('a', 'b');
        acyclic.calls('a', 'c');
        acyclic.calls('b', 'c');

        std::vector<char> sorted;
        acyclic.sort(sorted);
        ASSERT_EQ(sorted.size(), 3);
        ASSERT_EQ(sorted.at(0), 'c');
        ASSERT_EQ(sorted.at(1), 'b');
        ASSERT_EQ(sorted.at(2), 'a');
        return SUCCESS;
    }

    int test2() {
        P4::CallGraph<char> cg("test");

        //  a->b->c->d->e->f
        //     ^  ^__|  |
        //     |________|

        cg.calls('a', 'b');
        cg.calls('b', 'c');
        cg.calls('c', 'd');
        cg.calls('d', 'c');  // back-edge
        cg.calls('d', 'e');  // loop-exit-edge
        cg.calls('e', 'b');  // back-edge
        cg.calls('e', 'f');  // loop-exit-edge

        ASSERT_EQ(cg.isCallee('a'), false);
        ASSERT_EQ(cg.isCallee('b'), true);
        ASSERT_EQ(cg.isCaller('b'), true);
        ASSERT_EQ(cg.isCaller('f'), false);

        std::map<char, std::unordered_set<char>> dominators;
        cg.dominators('a', dominators);

        ASSERT_EQ(dominators['a'].size(), 1);
        ASSERT_EQ(*dominators['a'].begin(), 'a');
        sameSet(dominators['b'], {'a', 'b'});
        sameSet(dominators['c'], {'a', 'b', 'c'});
        sameSet(dominators['d'], {'a', 'b', 'c', 'd'});
        sameSet(dominators['e'], {'a', 'b', 'c', 'd', 'e'});
        sameSet(dominators['f'], {'a', 'b', 'c', 'd', 'e', 'f'});

        auto loops = cg.compute_loops('a');
        ASSERT_EQ(loops->loops.size(), 2);
        ASSERT_EQ(loops->isLoopEntryPoint('a'), -1);
        ASSERT_NEQ(loops->isLoopEntryPoint('b'), -1);
        ASSERT_NEQ(loops->isLoopEntryPoint('c'), -1);
        ASSERT_EQ(loops->isLoopEntryPoint('d'), -1);
        unsigned bLoopIndex = loops->isLoopEntryPoint('b');
        auto bloop = loops->loops.at(bLoopIndex);
        sameSet(bloop->body, {'b', 'c', 'd', 'e'});

        unsigned cLoopIndex = loops->isLoopEntryPoint('c');
        auto cloop = loops->loops.at(cLoopIndex);
        sameSet(cloop->body, {'c', 'd'});

        return SUCCESS;
    }

    int test3() {
        //          v--|
        // 0->1->3->4->6
        //    \->2  \->5->10
        //       \_________^
        P4::CallGraph<int> cg("test");
        cg.calls(0, 1);
        cg.calls(1, 2);
        cg.calls(1, 3);
        cg.calls(3, 4);
        cg.calls(4, 5);
        cg.calls(4, 6);
        cg.calls(6, 4);
        cg.calls(2, 10);
        cg.calls(5, 10);

        std::map<int, std::unordered_set<int>> dominators;
        cg.dominators(0, dominators);
        sameSet(dominators[0], {0});
        sameSet(dominators[1], {0,1});
        sameSet(dominators[2], {0,1,2});
        sameSet(dominators[3], {0,1,3});
        sameSet(dominators[4], {0,1,3,4});
        sameSet(dominators[5], {0,1,3,4,5});
        sameSet(dominators[6], {0,1,3,4,6});
        sameSet(dominators[10], {0,1,10});
        return SUCCESS;
    }

    int test4() {
        P4::CallGraph<int> cg("test");
        cg.calls(0, 1);
        cg.calls(2, 3);
        cg.calls(1, 3);
        std::set<int> reachable;
        cg.reachable(0, reachable);
        sameSet(reachable, {0, 1, 3});
        cg.remove(2);
        ASSERT_EQ(cg.size(), 3);
        return SUCCESS;
    }

    int test5() {
        // irreducible graph
        // /-----v
        // 1->2->3
        //    ^__|
        P4::CallGraph<int> cg("test");
        cg.calls(1, 2);
        cg.calls(1, 3);
        cg.calls(2, 3);
        cg.calls(3, 2);
        return SUCCESS;
    }

 public:
    int run() {
        RUNTEST(test1);
        RUNTEST(test2);
        RUNTEST(test3);
        RUNTEST(test4);
        RUNTEST(test5);
        return SUCCESS;
    }
};

}  // namespace Test

int main() {
    Test::TestCallGraph test;
    return test.run();
}
