/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Gordon Bailey (gb@gordonbailey.net)
 *
 */

#include <bf_lpm_trie/bf_lpm_trie.h>

#include <algorithm>  // for std::swap
#include <unordered_map>
#include <vector>
#include <tuple>
#include <limits>

#include "bm_sim/lookup_structures.h"
#include "bm_sim/match_key_types.h"
#include "bm_sim/lpm_trie.h"


namespace bm {
namespace {  // anonymous

static_assert(sizeof(uintptr_t) == sizeof(internal_handle_t),
              "Invalid type sizes");

// We don't need or want to export these classes outside of this
// compilation unit.

class LPMTrieStructure : public LPMLookupStructure {
 public:
  explicit LPMTrieStructure(size_t key_width_bytes)
    : trie(key_width_bytes) {
    }

  /* Copy constructor */
  LPMTrieStructure(const LPMTrieStructure &other) = delete;

  /* Move constructor */
  LPMTrieStructure(LPMTrieStructure &&other) noexcept = default;

  ~LPMTrieStructure() = default;

  /* Copy assignment operator */
  LPMTrieStructure &operator=(const LPMTrieStructure &other) = delete;

  /* Move assignment operator */
  LPMTrieStructure &operator=(LPMTrieStructure &&other) noexcept = default;

  bool lookup(const ByteContainer &key_data,
              internal_handle_t *handle) const override {
    return trie.lookup(key_data,
                       reinterpret_cast<uintptr_t *>(handle));
  }

  bool entry_exists(const LPMMatchKey &key) const override {
    return trie.has_prefix(key.data, key.prefix_length);
  }

  void add_entry(const LPMMatchKey &key,
                 internal_handle_t handle) override {
    trie.insert_prefix(key.data, key.prefix_length,
                       reinterpret_cast<uintptr_t>(handle));
  }

  void delete_entry(const LPMMatchKey &key) override {
    trie.delete_prefix(key.data, key.prefix_length);
  }

  void clear() override {
    trie.clear();
  }

 private:
  LPMTrie trie;
};

class ExactMap : public ExactLookupStructure {
 public:
  explicit ExactMap(size_t size) {
    entries_map.reserve(size);
  }

  bool lookup(const ByteContainer &key,
              internal_handle_t *handle) const override {
    const auto entry_it = entries_map.find(key);
    if (entry_it == entries_map.end()) {
      return false;  // Nothing found
    } else {
      *handle = entry_it->second;
      return true;
    }
  }

  bool entry_exists(const ExactMatchKey &key) const override {
    (void) key;
    return entries_map.find(key.data) != entries_map.end();
  }

  void add_entry(const ExactMatchKey &key,
                 internal_handle_t handle) override {
    entries_map[key.data] = handle;  // key is copied, which is not great
  }

  void delete_entry(const ExactMatchKey &key) override {
    entries_map.erase(key.data);
  }

  void clear() override {
    entries_map.clear();
  }

 private:
  std::unordered_map<ByteContainer, internal_handle_t, ByteContainerKeyHash>
    entries_map{};
};

class TernaryMap : public TernaryLookupStructure {
 public:
  explicit TernaryMap(size_t size, size_t nbytes_key)
    : keys(size), nbytes_key(nbytes_key) {}

  bool lookup(const ByteContainer &key_data,
              internal_handle_t *handle) const override {
    auto min_priority = std::numeric_limits<
                          decltype(TernaryMatchKey::priority)>::max();
    bool match;

    const TernaryMatchKey *min_key = nullptr;
    internal_handle_t min_handle = 0;

    for (auto it = keys.begin(); it != keys.end(); ++it) {
      if (it->priority >= min_priority) continue;

      match = true;
      for (size_t byte_index = 0; byte_index < nbytes_key; byte_index++) {
        if (it->data[byte_index] !=
            (key_data[byte_index] & it->mask[byte_index])) {
          match = false;
          break;
        }
      }

      if (match) {
        min_priority = it->priority;
        min_key      = &*it;
        min_handle   = std::distance(keys.begin(), it);
      }
    }

    if (min_key) {
      *handle = min_handle;
      return true;
    }

    return false;
  }

  bool entry_exists(const TernaryMatchKey &key) const override {
    auto handle = find_handle(key);

    return handle != keys.size();
  }

  void add_entry(const TernaryMatchKey &key,
                 internal_handle_t handle) override {
    keys.at(handle) = key;
  }

  void delete_entry(const TernaryMatchKey &key) override {
    auto handle = find_handle(key);

    // Mark this entry as empty by setting its priority to the maximum
    // possible value.
    keys.at(handle).priority = std::numeric_limits<
                              decltype(keys[handle].priority)>::max();
  }

  void clear() override {
    // Quickly mark every entry as empty. This should be faster than doing
    // `clear()` followed by `resize()`, as clear is already O(n)
    for (auto &k : keys) {
      k.priority = std::numeric_limits<decltype(k.priority)>::max();
    }
  }

 private:
  std::vector<TernaryMatchKey> keys;
  size_t nbytes_key;

  internal_handle_t find_handle(const TernaryMatchKey &key) const {
    auto it  = keys.begin();
    auto end = keys.end();
    for (; it != end; ++it) {
      if (it->priority == key.priority &&
          it->data == key.data &&
          it->mask == key.mask) {
        break;
      }
    }
    return std::distance(keys.begin(), it);
  }
};

}  // anonymous namespace

template <>
std::unique_ptr<LookupStructure<ExactMatchKey> >
LookupStructureFactory::create<ExactMatchKey>(
    LookupStructureFactory *f, size_t size, size_t nbytes_key) {
  return f->create_for_exact(size, nbytes_key);
}

template <>
std::unique_ptr<LookupStructure<LPMMatchKey> >
LookupStructureFactory::create<LPMMatchKey>(
    LookupStructureFactory *f, size_t size, size_t nbytes_key) {
  return f->create_for_LPM(size, nbytes_key);
}

template <>
std::unique_ptr<LookupStructure<TernaryMatchKey> >
LookupStructureFactory::create<TernaryMatchKey>(
    LookupStructureFactory *f, size_t size, size_t nbytes_key) {
  return f->create_for_ternary(size, nbytes_key);
}


std::unique_ptr<ExactLookupStructure>
LookupStructureFactory::create_for_exact(size_t size, size_t nbytes_key) {
  (void) nbytes_key;
  return std::unique_ptr<ExactLookupStructure>(new ExactMap(size));
}

std::unique_ptr<LPMLookupStructure>
LookupStructureFactory::create_for_LPM(size_t size, size_t nbytes_key) {
  (void) size;
  return std::unique_ptr<LPMLookupStructure>(new LPMTrieStructure(nbytes_key));
}

std::unique_ptr<TernaryLookupStructure>
LookupStructureFactory::create_for_ternary(size_t size, size_t nbytes_key) {
  return std::unique_ptr<TernaryLookupStructure>(new TernaryMap(size,
                                                                nbytes_key));
}

}  // namespace bm
