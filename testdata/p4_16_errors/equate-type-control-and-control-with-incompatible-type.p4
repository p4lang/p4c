// generated from shift-int-const.p4

header hdr_t {}
control C(inout bool b);
package P(C c);
control c(inout hdr_t hdr)() {
  apply {}
}
P(c()) main;
