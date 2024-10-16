# Slicing Iterator Made Easy

The slicing iterator takes a super cluster and some other constraints, like `packingConflict`,
`pa_container_size`, then returns all possible valid slicings of the input cluster. A slicing
is valid if all super clusters in the result can be allocated to a container group.
The iterator also accepts feedback from the allocation algorithm through the
`invalidate(const SuperCluster::SliceList* sl)` interface,
which tells the iterator to not generate results that contain the input slice list.

All files related to slicing are under `phv/slicing` directory, the general structure is
- `types.h` - basic type definitions for slicing.
- `phv_slicing_split.h` - static functions that can split a SuperCluster according to a schema.
  These are inherited from the previous implementation, and should be refactored later.
- `phv_slicing_iterator.h` - `PImpl` class for slicing iterator, to isolate the implementation and
  allows us to write white-box tests on the actual implementation.
- `phv_slicing_dfs_iterator.h` - implementation of a DFS-based slicing algorithm.

This document explains the DFS-based algorithm, as follows. We start with the basic constraint-free
algorithm, and incrementally enhance it by adding constraints. At each stage, we demonstrate how the
algorithm handles the additional constraint. We assume no knowledge about Tofino. The discussion
will be based on an informal mathematical model, extracted from the hardware details. All unrelated
details of hardware are elided.

## The basic slicing problem

Let's start with a few definitions:
- A *field* is a sequence of bits with a name.
- A *field slice* is a bitrange of a field. For example, `f1<32>[0:7]` represents the first 8 bits
  of `f1`. For slices that encompass an entire field, we use the field as shorthand. For example,
  `f1<32>` is short for `f1<32>[0:31]`.
- A *header* is a list of field slices. This is sometimes referred to as a *slice list*.
The basic problem is to splin N unrelated headers into smaller headers, such that that the size of
each is 8, 16, or 32 bits.

For example, suppose we have four headers A,B,C,D, with this layout:
```text
Headers:
A: [a1<12>, a2<12>, a3<8>]
B: [b1<128>, b2<128>[0:11], b2<128>[12:127]]
C: [c1<1>, c2<2>, c3<5>, c4<8>]
D: [d1<32>, d2<16>[0:7], d2<16>[8:15], d3<8>]
```
A valid slicing can be
```text
slicing: {
    [a1<12>, a2<12>, a3<8>]
    [b1<128>[0:31]],
    [b1<128>[32:63]],
    [b1<128>[64:95]],
    [b1<128>[96:127]],
    [b2<128>[0:11], b2[12:31]]
    [b2<128>[32:63]],
    [b2<128>[64:95]],
    [b2<128>[96:127]],
    [c1<1>, c2<2>, c3<5>, c4<8>],
    [d1<32>],
    [d2<16>[0:7], d2<16>[8:15]],
    [d3<8>],
}
```

The above problem can be solved by a simple DFS algorithm. The algorithm maintains two pieces of
state: a header list that contains headers to be split, and a result list of headers that has been
split. The algorithm is as follows:

1. If the header list is empty, return the result list as a solution.
2. Pick a header that has not yet been split. Remove it from the header list.
3. For N in {8, 16, 32}:
   1. Split out the first N bits of the header. We call the first N bits the *head*, and
      the remaining part the *tail*.
   2. Add the *head* to result list and the *tail* to header list, and recurse.
   3. Restore the state by removing *head* from result and *tail* from header.

Pseudo codes of the DFS:
```c++
// global states
Headers = [A, B, C, D];
Result = []

void dfs() {
    if (Headers.empty()) {
        print("solution: ", Result); // we found a solution
        return;
    }
    auto target_header = Headers.front();
    for (const int split_choice : {8, 16, 32}) {
        // head will be the first split_choice bits of the target_header
        (head, tail, ok) = split_first_n_bits(target_header, split_choice)
        if (!ok) continue;
        if (tail) Headers.append(tail)
        Headers.remove(target_header)
        Result.push_back(head)
        dfs()  // recurse
        Result.pop_back()
        Headers.add(target_header)
        if (tail) Headers.remove(tail)
    }
    return;
}
```

If the input `Headers` is `[A, C]`, then we expect to see:
```text
solution: {
   [a1<12>[0:7]], [a1<12>[8:11], a2<12>[0:3]], [a2<4:11>], [a3<8>],
   [c1<1>, c2<2>, c3<5>], [c4<8>]
}
solution: {
   [a1<12>[0:7]], [a1<12>[8:11], a2<12>[0:3]], [a2<4:11>], [a3<8>],
   [c1<1>, c2<2>, c3<5>, c4<8>]
}
solution: {
   [a1<12>[0:7]], [a1<12>[8:11], a2<12>[0:3]], [a2<4:11>, a3<8>],
   [c1<1>, c2<2>, c3<5>], [c4<8>]
}
solution: {
   [a1<12>[0:7]], [a1<12>[8:11], a2<12>[0:3]], [a2<4:11>, a3<8>],
   [c1<1>, c2<2>, c3<5>, c4<8>]
}
solution: {
   [a1<12>, a2<12>[0:3]], [a2<4:11>], [a3<8>],
   [c1<1>, c2<2>, c3<5>], [c4<8>]
}
...
...

```

The time complexity for finding all solutions is ~O(2^(M/8)), where M is the total bit
sizes of all N headers. Note that there are no constraints on any fields: because
those headers are unrelated, all possible slicings are valid. So, the time complexity
of finding the first solution is O(M).

## Enhancement 1: Rotational constraint on field slices

Let's add a new constraint to field slices. Define a rotational cluster as a set of field slices
that all must be allocated to same-sized containers. That is, in the slicing result, the slices
must reside in lists that have the same bit-length. We also require that the field slices in each
rotational cluster be split at the same bit.

For example, here is a header list with rotational constraints:
```text
Headers:
A: [a1<12>, a2<12>, a3<8>]
B: [b1<128>, b2<128>[0:11], b2<128>[12:127]]
C: [c1<1>, c2<2>, c3<5>, c4<8>]
D: [d1<32>, d2<16>[0:7], d2<16>[8:15], d3<8>]

Rotational Cluster Constraints:
{ a3<8>, d2<16>[8:15], c4<8>   }
{ a1<12>, b2<128>[0:11] }
```

The rotational cluster `{ a1<12>, b2<128>[0:11] }` means that if we split `a1` at the 8th bit, then
we must also split `b2<128>[0:11]` at the 8th bit. Making this split would result in two additional
rotational-cluster constraints:
```text
{ a1<12>[0:7], b2<128>[0:7]   }
{ a1<12>[8:11], b2<128>[8:11] }
```

This constraint transitively links different headers into a set that is called a *super cluster*.
Because of this constraint, some slicings are no longer valid. For example, the 3rd solution
```text
solution: {
   [a1<12>[0:7]],
   [a1<12>[8:11], a2<12>[0:3]],
   [a2<4:11>, a3<8>],
   [c1<1>, c2<2>, c3<5>],
   [c4<8>]
}
```
is now invalid because `c4` ends up in a 8-bit list while `a3` is in a 16-bit list.

We refine the previous algorithm with some pruning strategies to speed up the search. These pruning
strategies are early validations for detecting whether the current search state is doomed to produce
an invalid result.

### After-split constraint
Let's introduce the *after-split constraint*. An after-split constraint of a field slice represents
a requirement on the bit-size of the list containing the field slice in the output. We can deduce
after-split constraints for field slices in the result list, as they won't be split again. For
example, suppose the current state is
```text
Header List: [
    [c1<1>, c2<2>, c3<5>, c4<8>]
]
Result List: [
    [a1<12>, a2<12>, a3<8>]
]
```
then `a1`, `a2`, and `a3` will be in a 32-bit list for any sub-states derived. The implementation
prints out after-split constraints with LOG5:
```
a1 must =32,
a2 must =32,
a3 must =32,
```
(if we re-define after-split constraints so that they are propagated through rotational
clusters, then the second and third pruning strategies are redundant with the first.)

### Pruning strategy: Conflicting after-split constraints
If there are field slices in the same rotational cluster with incompatible after-split constraints,
then the current search state can be pruned.

Let's use the example from the above:
```text
Headers:
A: [a1<12>, a2<12>, a3<8>]
B: [b1<128>, b2<128>[0:11], b2<128>[12:127]]
C: [c1<1>, c2<2>, c3<5>, c4<8>]
D: [d1<32>, d2<16>[0:7], d2<16>[8:15], d3<8>]

Rotational Cluster Constraints:
{ a3<8>, d2<16>[8:15], c4<8>   }
{ a1<12>, b2<128>[0:11] }
```

If the current state is
```text
Header List: [
    [b1<128>, b2<128>[0:11], b2<128>[12:127]]
    [d1<32>, d2<16>[0:7], d2<16>[8:15], d3<8>]
]
Result List: [
   [a1<12>[0:7]],
   [a1<12>[8:11], a2<12>[0:3]],
   [a2<4:11>, a3<8>],
   [c1<1>, c2<2>, c3<5>],
   [c4<8>]
]
```
then we know there is a conflict on `a3` and `c4` because they are in the same super cluster, but
their after-split constraints are incompatible: `a3` is in a 16-bit list, while `c4` is in an 8-bit
list.
```text
a1[0:7]  must =8
a1[8:11] must =8
a2[0:3]  must =8
a2[4:11] must =16
a3       must =16 !conflicts
c1       must =8
c2       must =8
c3       must =8
c4       must =8  !conflicts

// pasted from above
Rotational Cluster Constraints:
{ a3<8>, d2<16>[8:15], c4<8>   }
```
At this point, we should backtrack instead of further splitting header B or D in the header list. By
pruning the search here, we eliminate *2^37* invalid results!

The time complexity for this strategy is O(N), where N is the number of field slices. It can
eliminate O(2^(K/8)) invalid solutions, where K is the number of bits remaining in the header list.
Note that this strategy will prune more invalid states if K is larger. We will talk about this in
[Optimization: Search order].

### Pruning strategy: Slice-list size limitation
Since headers (i.e., slice lists) only get smaller as the search progresses, we can check the
after-split constraints on a field slice against the bit-length of the slice list containing that
slice. That is, if an after-split constraint on a field slice *f1* requires a list larger than the
list currently holding slice *f2*, and *f1* and *f2* are in the same rotational cluster, then
current state is invalid and can be pruned.

Example:
```text
Header List: [
    [b1<128>, b2<128>[0:11], b2<128>[12:127]]
    [c1<1>, c2<2>, c3<5>, c4<8>]
    [d1<32>, d2<16>[0:7], d2<16>[8:15], d3<8>]
]
Result List: [
    [a1<12>, a2<12>, a3<8>]
]

After split constraint:
a3       must =32

// pasted from the above
Rotational Cluster Constraints:
{ a3<8>, d2<16>[8:15], c4<8>   }

```
1. a3 and c4 are in a rotational cluster.
2. a3 will be in a 32-bit list.
3. c4 resides in a 16-bit list, can never be allocated to 32-bit list, backtrack!

Time Complexity: O(n), where n equals to the number of field slices.

### Pruning strategy: Not all constraints are satisfiable
Consider a header `[f1<24>, f2<8>]`, and two after-split constraints enforced by
other field slices in the same rotational cluster with f1/f2, `f1 must =32, f2 must =16`,
there is no valid solution, because only one after-split constraint can be satisfied.
Generally, after we collect after-split constraints for all
field slices in a list, we can check whether they can all be satisfied on that header/slicelist.

To check whether all after-split constraints of a slice list can be satisfied, we create a bitmap
that has the same size as the list, representing after-split constraints on each bit of the list.
Then for each field slice, the algorithm will seek back to the left-most possible starting bit,
*leftmost*, then mark bits in \[leftmost, leftmost + after-split-constraint.size\]
to the after-split constraints by taking an intersection of constraint of the field and
the constraint recorded on the bitmap.
If the intersection of two constraints on the bit is empty, then it means there exists
two constraints that cannot be satisfied together. Please refer to
`DfsItrContext::dfs_prune_unsat_slicelist_constraints` in the implementation for more details.

Example:
```text
Header List: [
   [f1<24>, f2<8>]
]
Result List: [
   ***
]

After split constraint:
f1 must =32, f2 must =16

constraint bitmap:
0                              31
................................
WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW   <-- f1
................HHHHHHHHHHHHHHHH   <-- f2
                ^
                |
            conflicts!

```

The time complexity is O(M), where M is the total number of bits of the input super cluster.

## Enhancement 2: Super-cluster slice-list size constraint
Recall that in Enhancement 1, we stated that rotational clusters transitively link
headers into a super cluster. We add another constraint: all slicelists in a super cluster
must have a same size. We say that a super cluster is well-formed if it satisfies this constraint.

Example:
```text
well-formed:
[f1<8>]
[f3<32>[0:7]]
{f1, f3[0:7]}

not well-formed:
[f1<8>, f2<8>]
[f3<32>[0:7], f3[8:31]]
{f1, f3[0:7]}
```

### Pruning strategy: Malformed cluster
This strategy is simple. If we find a super cluster whose slice lists are all in the
result list, we check whether this cluster is well-formed.

### Same container group constraint
For slices of a field with `same_container_group` constraint, all those slices
will end up in a same supercluster, so they share same aftersplit constraints
just slices in a rotation cluster. AfterSplitConstraint of any fieldslice will be propagated
to all other slices of the same field.

## Enhancement 3: Metadata list
The above discussion assumes that the size of a slice list must be one of the 8, 16, or 32.
In this enhancement, we introduce a new type of list, whose size is not subject to this constraint.

Metadata is a special type of field that has virtual padding at the beginning of the field.
The size of the virtual padding can any integer from 0 to 7.
On Tofino, this virtual padding corresponds to the alignment constraint, and is called `Alignment`
in the code as well. In this document, we prefer the term "virtual padding". We write
`fieldname^5<8>` to represent a metadata field that is 8-bits long, preceded by 5 bits of virtual
padding.

Metadata fields form metadata slice lists, and we cannot mix metadata fields with
normal fields to form a list. Metadata can be in a rotational cluster with other normal fields.
Define the *virtual size* of a metadata list to be the combined length of the slicess in the list,
plus the size of the virtual padding on the first field slice. The size of a metadata list is not
limited to 8, 16, or 32; instead, its virtual size cannot exceed 32 bits.

Example:
```text
[m1^5<8>] ==> ok, virtual size 13 bits long.
[m2^3<30>] ==> bad, virtual 33 bits long.
```

### Updated rotational and super-cluster slice-list size constraints
Metadata field slices in a rotational cluster, are not required to be in same-sized lists.
However, if there are normal fields in the rotational cluster, then the virtual size of those
metadata lists cannot exceed the size of list normal fields(they are supposed to be equal).

Also, for super-cluster slice-list constraint, the constraint becomes that normal slice lists must
have the same size, and metadata list's virtual size cannot be larger than that size.

(Not sure I follow this section. Examples would help.)

### Enhanced after-split constraint
Although it seems that metadata type complicates the problem, we can handle them by simply
enhancing our after-split constraint. Previously, an after-split constraint can only specify
the 'exact' requirement. We introduce a less restricted form of after-split constraint, which
specifies a minimum size of the list that a metadata resides in.

```c++
struct AfterSplitConstraint {
    enum class ConstraintType {
        EXACT = 0,  // must be placed in container of the size.
        MIN = 1,    // must be placed in container at least the size.
        NONE = 2,   // no new constraint.
    };
    ConstraintType t;
    int size;

    // returns the intersection of two AfterSplitConstraint.
    // e.g. MIN(8)   ^ EXACT(32) = EXACT(32)
    //      MIN(8)   ^ MIN(16)   = MIN(16)
    //      EXACT(8) ^ EXACT(16) = boost::none
    boost::optional<AfterSplitConstraint> intersect(const AfterSplitConstraint& other) const;
};
```

With the enhanced `AfterSplitConstraint` class, the above pruning strategies still apply.

### Pruning strategy: metadata list joining two exact containers slicelists with different sizes
When a medatada list joins two exact containers lists of different sizes into one supercluster,
we should prune early because the supercluster cannot be well-formed already.
For example,
```text
sl_1: [f1<16>, f2<8>, f3<8>[0:1], f3<8>[2:7]], total 32, exact.
sl_2: [f2'<8>, f4<8>[0:3], f4<8>[4:7]], total 16, exact.
sl_3: [md1<2>, pad<2>, md2<4>]
rotational clusters:
{f3[0:1], md1}, {f4<8>[0:3], md2}
```
sl3 will join sl1 and sl2 into one super cluster, and we can infer that
this cluster is invalid because there are a 32-bit exact list and a 16-bit exact list.

## Enhancement 4: Pack conflicts between fields
Two fields exhibit a pack conflict when they cannot end up in the same list. This constraint is
orthogonal to the above constraints. The algorithm checks all splitted slice lists to ensure that no
slice list contains a pair of pack-conflicted fields.

Because we only split at byte boundaries (i.e., we only split out the first 8, 16, or 32 bits from
the front of the list), fields that share a byte will inevitably end up in the same list. For these
fields, we skip the pack-confict check. Doing so is consistent with the action-PHV constraint check
in the PHV-allocation algorithm.

```text
pack conflicts:
(f1, f2), (f2, f3), (f3, f4)
ok:
[f1<8>, f4<8>]
[f1<4>, f2<4>] --> sharing a byte
[f1<20>[0:10], f1<20>[11:18], f1[19:19], f2[0:11]] -->sharing a byte.

not ok:
[f1<8>, f2<8>]
[f1<4>, f2<4>, f3<8>] --> (f1,f3) and (f2,f3) were not sharing a byte
```

## Enhancement 5: Container-size pragma
The `@pa_container_size` pragma specifies the sizes of lists that contain a
field after slicing. This pragma is surprisingly powerful in that it supports two
modes.

The pragma is used in exact-size mode if the specified container sizes add up to the size of the
field (e.g., `@pa_container_size(f1<32>, 32)` or `@pa_container_size(f1<32>, 16, 8, 8)`).
To handle this constraint, before doing DFS, we do a pre-slicing following the specified schema,
that fully splits out those smaller chunks of fields.

For example,
```text
@pa_container_size(b2, 32, 32, 32, 32)
@pa_container_size(d1, 32)
Headers:
A: [a1<12>, a2<12>, a3<8>]
B: [b1<128>, b2<128>[0:11], b2<128>[12:127]]
C: [c1<1>, c2<2>, c3<5>, c4<8>]
D: [d1<32>, d2<16>[0:7], d2<16>[8:15], d3<8>]
```
headers will be pre-sliced into
```text
Headers:
   [a1<12>, a2<12>, a3<8>]
   [b1<128>]
   [b2<128>[0:11], b2<128>[12:31]]      *
   [b2<128>[32:63]]                     *
   [b2<128>[64:95]]                     *
   [b2<128>[96:127]]                    *
   [c1<1>, c2<2>, c3<5>, c4<8>]
   [d1<32>]                             *
   [d2<16>[0:7], d2<16>[8:15], d3<8>]
```

The pragma also has an up-casting mode, wherein the sum of container sizes is larger than the size
of the field (e.g., `@pa_container_size(f1<20>, 32)` or `@pa_container_size(f2<20>, 16, 8)`).
In this mode, we can only do a limited pre-slicing. For example, with the first example usage, we
don't know where to split around `f1` so that it ends up in a 32-bit list: it can either be 12 bits
before `f1` or 12 bits after `f1`. The good news is, for `f2` in the second example, we can know
where to split even in up-casting mode. The code implements this by giving the tailing field slice
an after-split constraint.

(Not following this last point. An example would help.)

## Optimization: Search order
A general optimization for a DFS algorithm is to schedule the search order at each step to
form a thinner search tree by leveraging pruning strategies.
For example, `A*` sorts its search frontier according to `f(n) = h(n) + g(n)`. In our slicing DFS
algorithm, the decision at each step is to choose N and X, and split out first N bits from list X.
For picking the X, similar to [Algorithm X](https://en.wikipedia.org/wiki/Knuth%27s_Algorithm_X),
we want to choose the list with most constraints. There is a class called
`NextSplitTargetMetrics` that can sort lists by constraints.
For picking the N, the `make_choices` function will return a sorted list where preferred values of N
are at front of the list.
