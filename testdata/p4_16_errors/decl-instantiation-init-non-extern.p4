// generated from issue2795.p4

extern packet_out {}
control ctr(packet_out p);
control c(packet_out p)() {
  apply {}
}
package top(ctr _c);
top(c()) main = {
  error a() {}
};
