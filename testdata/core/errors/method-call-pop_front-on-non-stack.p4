// generated from issue313.p4

header header_h {}
struct struct_t {
  .header_h stack;
}
control ctrl(inout struct_t input, out bit<8> output) {
  action act() {
    input.stack.pop_front();
  }
  apply {}
}
