// generated from union-key.p4

header_union U {}
struct Headers {
  U u;
}
control c(in Headers h)() {
  table t {
    key = {
      h.u.h1.x : exact;
    }
    actions = {}
  }
  apply {}
}
