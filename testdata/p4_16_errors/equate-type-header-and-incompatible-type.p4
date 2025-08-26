// generated from shift-int-const.p4

header hdr_t {}
control c(inout hdr_t hdr)() {
  apply {}
}
control C(inout list<bool> hdr);
package P(C c);
P(c()) main;
