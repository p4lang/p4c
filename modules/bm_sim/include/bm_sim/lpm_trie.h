#ifndef _BM_LPM_TRIE_H_
#define _BM_LPM_TRIE_H_

#include <bf_lpm_trie/bf_lpm_trie.h>

class LPMTrie {
public:
  LPMTrie(size_t key_width_bytes)
    : key_width_bytes(key_width_bytes) {
    trie = bf_lpm_trie_create(key_width_bytes, true);
  }

  /* Copy constructor */
  LPMTrie(const LPMTrie& other) = delete;

  /* Move constructor */
  LPMTrie(LPMTrie&& other) noexcept
  : key_width_bytes(other.key_width_bytes), trie(other.trie) {}

  ~LPMTrie() {
    bf_lpm_trie_destroy(trie);
  }

  /* Copy assignment operator */
  LPMTrie &operator=(const LPMTrie &other) = delete;
 
  /* Move assignment operator */
  LPMTrie &operator=(LPMTrie &&other) noexcept {
    key_width_bytes = other.key_width_bytes;
    std::swap(trie, other.trie);
    return *this;
  }

  void insert_prefix(const ByteContainer &prefix, int prefix_length,
		     uintptr_t value) {
    bf_lpm_trie_insert(trie, prefix.data(), prefix_length, (value_t) value);
  }

  bool delete_prefix(const ByteContainer &prefix, int prefix_length) {
    return bf_lpm_trie_delete(trie, prefix.data(), prefix_length);
  }

  bool lookup(const ByteContainer &key, uintptr_t *value) const {
    return bf_lpm_trie_get(trie, key.data(), (value_t *) value);
  }

private:
  size_t key_width_bytes;
  bf_lpm_trie_t *trie;
};




#endif
