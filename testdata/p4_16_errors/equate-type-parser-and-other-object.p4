// generated from issue1205-bmv2.p4

control C();
package V1Switch(C c);
parser MyP()() {
  state start {}
}
V1Switch(MyP()) main;
