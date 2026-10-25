// generated from issue1208.p4

control c();
package p(c _c);
package q(c _p);
control empty()() {
  apply {}
}
q(p(empty())) main;
