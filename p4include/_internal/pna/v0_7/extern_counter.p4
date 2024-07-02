// BEGIN:CounterType_defn
enum PNA_CounterType_t {
    PACKETS,
    BYTES,
    PACKETS_AND_BYTES
}
// END:CounterType_defn

// BEGIN:Counter_extern
/// Indirect counter with n_counters independent counter values, where
/// every counter value has a data plane size specified by type W.

@noWarn("unused")
extern Counter<W, S> {
  Counter(bit<32> n_counters, PNA_CounterType_t type);
  void count(in S index);
}
// END:Counter_extern

// BEGIN:DirectCounter_extern
@noWarn("unused")
extern DirectCounter<W> {
  DirectCounter(PNA_CounterType_t type);
  void count();
}
// END:DirectCounter_extern
