
# Lookahead Statement Coverage Algorithm

## Main definitions

**Definition 1**.
*Statement* is a pair <*c*, *s*> where *c* is a Boolean flag (covered or not covered *s*), and *s* is pointer to IR::Statement object of input P4 program after all transformations of the Testgen's midend.

**Definition 2**.
*Fork* is a pair <*c*, *{bi}*>, where *c* is a Boolean flag and *{bi}* is a a vector of branches.

**Definition 3**.
*Branch* *bi* is a pair <*c*, *i*>, where *c* is a Boolean flag and *i* is a order number inside corresponded *fork*. Each *branch* defines a path consisting of all reachable *statements* that follow after that branch.

**Definition 4**.
*Closed branch* is a *branch* where *c* is *true*. In this case there are no reachable *statements* that could be found in corresponded path by control flow. 

**Definition 5**.
*Closed fork* is a *fork* where *c* is *true*. In this case all *branches* defined by it are *closed branches*.

**Definition 8**.
*Trace* is a list of triples <*f*, *i*, *{s}*> where *f* is a *fork* and *i* is a number of selected *branches*, and *{s}* is a set of covered *statements* which were uncovered before the current trace.

**Definition 9**.
*Termination state* is a state where a resulting test of the Testgen have to be successfully generated.

## Algorithm description

The Algorithm contains the following stages.

**Stage 1 - Initial** 
1.  All *statements* are not covered, all *forks* and *branches* are not closed.   

**Stage 2 - Execution** 
1.  If it is a *termination state* then go to *Stage 4 - Saving trace*
2.  If some IR::Statement is applied then corresponded *statement* is marked as covered. If corresponded *statement* was not covered then it should be added to a *{s}* set of a *trace*.
3.  Make a choice inside *fork*.
    3.1. If it is a *closed fork* then go to *Stage 3 - Continue trace*.
    3.2. Collect all non-*closed branches*.
    3.3. Select *branch* according to the strategies from the built list in stage 2.2. Here is the place to implement different heuristics: select branch randomly, select *branch* with the maximum of the uncovered *statements* (it tries to minimize number of the tests), select *branch* which is the nearest to the *termination state* (it makes all tests shorter) etc.
3. Continue execution.

**Stage 3 - Continue trace**
1. if set *{s}* is empty in current *trace* then drop it.
2. Continue *trace* to the nearest *termination state* with a help of breadth-first-search.
3. Save obtained *trace* with help *Stage 4 - Saving trace*. 

**Stage 4 - Saving trace**
1. Mark all elements in {*s*} as covered.
2. Start to analyse each element of a *trace* from the end.
3. If *branch* in a *trace* is *closed* then move to previous element in a *trace*.
4. if *branch* *i* in fork *f* doesn't contain any uncovered *statements* then mark it as *closed branch* (*c = true*).
5. Analyse other *branches* in fork *f*. Update other non-*closed-branches* in a fork *f* (go to 4.4). If all of the *branches* from *f* are *closed* then move to previous element in a *trace* and go to 4.4. Otherwise, stop updating algorithm.
