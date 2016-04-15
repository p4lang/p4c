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
    : entries(size), nbytes_key(nbytes_key) {}

  bool lookup(const ByteContainer &key_data,
              internal_handle_t *handle) const override {
    auto min_priority =
        std::numeric_limits<decltype(VectorEntry::priority)>::max();
    bool match;

    const VectorEntry *min_entry = nullptr;
    internal_handle_t min_handle = 0;

    for (const VectorEntry *entry = head; entry; entry = entry->next) {
      if (entry->priority >= min_priority) continue;

      match = true;
      for (size_t byte_index = 0; byte_index < nbytes_key; byte_index++) {
        if (entry->key->data[byte_index] !=
            (key_data[byte_index] & entry->key->mask[byte_index])) {
          match = false;
          break;
        }
      }

      if (match) {
        min_priority = entry->priority;
        min_entry = entry;
        // a bit sad that this cast is needed, almost makes me want to do the
        // pointer arithmetic by hand
        min_handle = std::distance(const_cast<const VectorEntry *>(head),
                                   entry);
      }
    }

    if (min_entry) {
      *handle = min_handle;
      return true;
    }

    return false;
  }

  bool entry_exists(const TernaryMatchKey &key) const override {
    const VectorEntry *entry = find_vec_entry(key);
    return (entry != nullptr);
  }

  void add_entry(const TernaryMatchKey &key,
                 internal_handle_t handle) override {
    VectorEntry &entry = entries.at(handle);
    entry.priority = key.priority;
    entry.key = &key;

    if (handle == 0) {
      entry.prev = nullptr;
      entry.next = head;
      head = &entry;
      if (entry.next) entry.next->prev = &entry;
    } else {
      assert(&entry != head);
      // this uses knowledge about how the match unit allocates handles;
      // basically if a handle is allocated, in means all handles before it have
      // already been allocated.
      VectorEntry &prev_entry = entries[handle - 1];
      entry.prev = &prev_entry;
      entry.next = prev_entry.next;
      prev_entry.next = &entry;
      if (entry.next) entry.next->prev = &entry;
    }
  }

  void delete_entry(const TernaryMatchKey &key) override {
    const VectorEntry *entry = find_vec_entry(key);
    assert(entry);
    if (entry->prev)
      entry->prev->next = entry->next;
    else
      head = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
  }

  void clear() override {
    head = nullptr;
  }

 private:
  struct VectorEntry {
    int priority;  // duplicated on purpose (for efficiency although debatable)
    const TernaryMatchKey *key;
    VectorEntry *prev;
    VectorEntry *next;
  };

  VectorEntry *head{nullptr};
  std::vector<VectorEntry> entries;
  size_t nbytes_key;

  VectorEntry *find_vec_entry(const TernaryMatchKey &key) const {
    for (VectorEntry *entry = head; entry; entry = entry->next) {
      if (entry->priority == key.priority &&
          entry->key->data == key.data &&
          entry->key->mask == key.mask) {
        return entry;
      }
    }
    return nullptr;
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
