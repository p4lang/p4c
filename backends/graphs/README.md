# Graphs Backend

This backend produces visual representations of a P4 program as dot files. For
now it only supports the generation of graphs for top-level control blocks.

## Dependencies

In addition to other p4c dependencies, this backend requires the Boost graph
headers. On a Debian system, they can be installed with `sudo apt-get install
libboost-graph-dev`.

## Usage

```
mkdir out
p4c-graphs <prog.p4> --graphs-dir out
cd out
dot <name>.dot -Tpng > <name>.png
```

## Improvement ideas

  - use different shapes for vertices of different types (e.g. rectangles for
    tables)
  - change representation of controls: instead of adding a vertex labelled with
    the control name when the control is invoked, group the control contents
    into a "subgraph"


## Example

You can look at the control graph generated for the egress control of
[switch.p4](https://github.com/p4lang/switch/tree/f219b4f4e25c2db581f3b91c8da94a7c3ac701a7/p4src)
[here](http://bmv2.org/switch_egress.png).
