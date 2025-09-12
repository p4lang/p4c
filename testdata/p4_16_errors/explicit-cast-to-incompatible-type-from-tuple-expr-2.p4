// generated from serenum.p4

enum bit<16> EthTypes {
  IPv4 = 2048,
};
header Ethernet {
  EthTypes type;
}
struct Headers {
  Ethernet eth;
}
control c(inout Headers h) {
  apply {
    h.eth.type = (EthTypes) { };
  }
}
