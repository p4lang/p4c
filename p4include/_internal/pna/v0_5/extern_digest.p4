// BEGIN:Digest_extern
extern Digest<T> {
  Digest();                       /// define a digest stream to the control plane
  void pack(in T data);           /// emit data into the stream
}
// END:Digest_extern
