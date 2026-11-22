#include <core.p4>

control MyIngress(inout bit r) { apply{} }

control MyEgress(inout bit r);

package top(MyEgress e);
top(MyIngress()) main;

