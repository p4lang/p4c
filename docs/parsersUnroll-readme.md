# Loops Unrolling and Header Stack Operation Elimination Algorithm

## Main definitions

**Definition 1**.
P2C Parser is a couple *P = ({S}, {HS})*, where *{S}* is a set of the parser states *S*,
*{HS}* is a set of header stack variables which are used in the parser *P*.

**Definition 2**.
The map *M* is a couple *M = {(HS,i)}* – where *i* is an index of corresponding stack variable evaluated during the symbolic execution. 
Index is evaluated correspondingly to [P2C semantics](https://p4.org/p4-spec/docs/P4-16-v1.2.1.html#sec-header-stacks).

**Definition 3**.
Let ind be a function which returns current number for a parser state with the corresponding set of header stack variables during symbolic execution: *ind:S×M→N*, where *N* is a set of natural number. 

**Definition 4**.
The main data structure of the algorithm is a triple:
1.  current parser’s state, 
2.  state of symbolic execution (formula over the parser program variables),
3.  call number (result of application of ind function). 

The following information is stored into ParserStructure class:
*   table with the name of the states and their representation in *IR::ParserState*,
*   calls graph – graph of parser states calls,
*   a set of header stack variables which are used in each parser’s state.

**Definition 5**.
Let *states path* be a sequence of parser’s state calls. Each such path contains a set of header state variables which are used with the arbitrary arguments in a path p and denote it *{HSp}*. These arguments cannot be as c constants.

## Algorithm description

The algorithm presents bread-first-search for parser’s state calls performing symbolic execution for each statement. 
Algorithm contains the following stages. 

**Stage 1** 
1.  Getting start parser’s state as an input point.   
2.  Generation of *M* set as empty set. 
3.	Let *ind(start,M) = 0*,  (start was never called before that). 
    In the code triples are represented by *ParserStateInfo* (midend/parserUnroll.h, L36). 
4.  Putting initial values into the *main list* (midend/parserUnroll.cpp, L601) for traversal.

**Stage 2** 
1.  Getting *first element* (midend/parserUnroll.cpp, L609) from the main list.
2.  Checking current state for a visited state. 

    *Storage of the called states* contains the traversed state during symbolic execution.
    Current state *Snew* of parser is called *visited state* if the storage of the called states contains the state with the same name and the same set *M*.
 
3.	Checking state which was called in this path.
4.	*Checking* (midend/parserUnroll.cpp, L661) a *reachable* (frontends/p4/callGraph.h, L126) parser’s state which could use a header stack variables from *{HSp}* of current *path*. 
5.	If *Snew* is not a visited and not in this path and there are no such reachable parser’s states then *Snew* could be *eliminated* (midend/parserUnroll.cpp, L616) from the search. 
6.	Otherwise, continue with the **Stage 3**.

**Stage 3**
1.	*Adding* (midend/parserUnroll.cpp, L620) *Snew* to the storage of visited states. 
2.	*Adding* (midend/parserUnroll.cpp, L621) *Snew* to the current path.

**Stage 4**
1.	*Making* (midend/parserUnroll.cpp, L627) symbolic execution of the state.

    During symbolic execution *new name for a state* (midend/parserUnroll.cpp, L555) is generated. 
    If *ind* function returns 0 the name remains the same. For each statement, the algorithm tries to evaluate concrete indexes for header stacks (*IR::ArrayIndex* (midend/parserUnroll.cpp, L118), *IR::Member* (midend/parserUnroll.cpp, L139)) with the updating of map *M*. If new state *Snew* is in the *path* or some states with header stack variables from *{HSp}* are reachable then algorithm generates a new name by usage of the *ind* function. 

2.	If there are no states with the similar names in this path then *generating* (midend/parserUnroll.cpp, L586) the outOfBound *call* ((midend/parserUnroll.h, L198)). 
3.	Otherwise, *adding* (midend/parserUnroll.cpp, L664) result to the main list and continue with the **Stage 2**. 
