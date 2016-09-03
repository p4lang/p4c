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
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_BM_SIM_MATCH_UNITS_H_
#define BM_BM_SIM_MATCH_UNITS_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <atomic>
#include <utility>  // for pair<>
#include <memory>

#include "match_key_types.h"
#include "match_error_codes.h"
#include "lookup_structures.h"
#include "bytecontainer.h"
#include "phv.h"
#include "packet.h"
#include "handle_mgr.h"
#include "counters.h"
#include "meters.h"

namespace bm {

class P4Objects;  // forward declaration for deserialize

// using string and not ByteContainer for efficiency
struct MatchKeyParam {
  // order is important, implementation sorts match fields according to their
  // match type based on this order
  // VALID used to be before RANGE, but when RANGE support was added, it was
  // easier (implementation-wise) to put RANGE first. Note that this order only
  // matters for the implementation.
  enum class Type {
    RANGE,
    VALID,
    EXACT,
    LPM,
    TERNARY
  };

  MatchKeyParam(const Type &type, std::string key)
    : type(type), key(std::move(key)) { }

  MatchKeyParam(const Type &type, std::string key, std::string mask)
    : type(type), key(std::move(key)), mask(std::move(mask)) { }

  MatchKeyParam(const Type &type, std::string key, int prefix_length)
    : type(type), key(std::move(key)), prefix_length(prefix_length) { }

  friend std::ostream& operator<<(std::ostream &out, const MatchKeyParam &p);

  static std::string type_to_string(Type t);

  Type type;
  std::string key;  // start for range
  std::string mask{};  // optional, end for range
  int prefix_length{0};  // optional
};


namespace detail {

class MatchKeyBuilderHelper;

}  // namespace detail

// Fields should be pushed in the P4 program (i.e. JSON) order. Internally, they
// will be re-ordered for a more efficient implementation.
class MatchKeyBuilder {
  friend class detail::MatchKeyBuilderHelper;
 public:
  void push_back_field(header_id_t header, int field_offset, size_t nbits,
                       MatchKeyParam::Type mtype, const std::string &name = "");

  void push_back_field(header_id_t header, int field_offset, size_t nbits,
                       const ByteContainer &mask, MatchKeyParam::Type mtype,
                       const std::string &name = "");

  void push_back_valid_header(header_id_t header, const std::string &name = "");

  void apply_big_mask(ByteContainer *key) const;

  void operator()(const PHV &phv, ByteContainer *key) const;

  std::vector<std::string> key_to_fields(const ByteContainer &key) const;

  std::string key_to_string(const ByteContainer &key,
                            std::string separator = "",
                            bool upper_case = false) const;

  void build();

  template <typename E>
  std::vector<MatchKeyParam> entry_to_match_params(const E &entry) const;

  template <typename E>
  E match_params_to_entry(const std::vector<MatchKeyParam> &params) const;

  bool match_params_sanity_check(
      const std::vector<MatchKeyParam> &params) const;

  size_t get_nbytes_key() const { return nbytes_key; }

  const std::string &get_name(size_t idx) const { return name_map.get(idx); }

  size_t max_name_size() const { return name_map.max_size(); }

 private:
  struct KeyF {
    header_id_t header;
    int f_offset;
    MatchKeyParam::Type mtype;
    size_t nbits;
  };

  struct NameMap {
    void push_back(const std::string &name);
    const std::string &get(size_t idx) const;
    size_t max_size() const;

    std::vector<std::string> names{};
    size_t max_s{0};
  };

  // takes ownership of input
  void push_back(KeyF &&input, const ByteContainer &mask,
                 const std::string &name);

  std::vector<KeyF> key_input{};
  size_t nbytes_key{0};
  bool has_big_mask{false};
  ByteContainer big_mask{};
  // maps the position of the field in the original P4 key to its actual
  // position in the implementation-specific key. In the implementation, RANGE
  // match keys come first, followed by VALID, EXACT, then LPM and TERNARY.
  std::vector<size_t> key_mapping{};
  // inverse of key_mapping, could be handy
  std::vector<size_t> inv_mapping{};
  std::vector<size_t> key_offsets{};
  NameMap name_map{};
  bool built{false};
  std::vector<ByteContainer> masks{};
};

namespace MatchUnit {

struct AtomicTimestamp {
  std::atomic<uint64_t> ms_{};

  AtomicTimestamp() { }

  template <typename T>
  explicit AtomicTimestamp(const T &tp) {
    ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      tp.time_since_epoch()).count();
  }

  explicit AtomicTimestamp(uint64_t ms)
    : ms_(ms) { }

  template <typename T>
  void set(const T &tp) {
    ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      tp.time_since_epoch()).count();
  }

  void set(uint64_t ms) {
    ms_ = ms;
  }

  uint64_t get_ms() const {
    return ms_;
  }

  /* don't need these (for now?), so remove them */
  AtomicTimestamp(const AtomicTimestamp &other) = delete;
  AtomicTimestamp &operator=(const AtomicTimestamp &other) = delete;

  /* std::atomic<T> is non-movable so I have to define this myself */
  AtomicTimestamp(AtomicTimestamp &&other)
    : ms_(other.ms_.load()) { }
  AtomicTimestamp &operator=(AtomicTimestamp &&other) {
    ms_ = other.ms_.load();
    return *this;
  }
};

struct EntryMeta {
  typedef Packet::clock clock;

  AtomicTimestamp ts{};
  uint32_t timeout_ms{0};
  Counter counter{};
  uint32_t version{};

  void reset() {
    counter.reset_counter();
    ts.set(clock::now());
  }
};

}  // namespace MatchUnit

class MatchUnitAbstract_ {
 public:
  friend class handle_iterator;
  // Iterator for entry handles
  // having a const / non-const flavor would not make sense here: handles cannot
  // be modified
  class handle_iterator
      : public std::iterator<std::forward_iterator_tag, handle_t> {
   public:
    handle_iterator(const MatchUnitAbstract_ *mu, HandleMgr::const_iterator it);

    const entry_handle_t &operator*() const {
      assert(it != mu->handles.end() && "Invalid iterator dereference.");
      return handle;
    }

    const entry_handle_t *operator->() const {
      assert(it != mu->handles.end() && "Invalid iterator dereference.");
      return &handle;
    }

    bool operator==(const handle_iterator &other) const {
      return (mu == other.mu) && (it == other.it);
    }

    bool operator!=(const handle_iterator &other) const {
      return !(*this == other);
    }

    handle_iterator &operator++();

    const handle_iterator operator++(int) {
      // Use operator++()
      const handle_iterator old(*this);
      ++(*this);
      return old;
    }

   private:
    const MatchUnitAbstract_ *mu{nullptr};
    HandleMgr::const_iterator it;
    entry_handle_t handle{};
  };

 public:
  MatchUnitAbstract_(size_t size, const MatchKeyBuilder &key_builder);

  size_t get_num_entries() const { return num_entries; }

  size_t get_size() const { return size; }

  size_t get_nbytes_key() const { return nbytes_key; }

  bool valid_handle(entry_handle_t handle) const;

  MatchUnit::EntryMeta &get_entry_meta(entry_handle_t handle);
  const MatchUnit::EntryMeta &get_entry_meta(entry_handle_t handle) const;

  void reset_counters();

  void set_direct_meters(MeterArray *meter_array);

  Meter &get_meter(entry_handle_t handle);

  MatchErrorCode set_entry_ttl(entry_handle_t handle, unsigned int ttl_ms);

  void sweep_entries(std::vector<entry_handle_t> *entries) const;

  void dump_key_params(std::ostream *out,
                       const std::vector<MatchKeyParam> &params,
                       int priority = -1) const;

  // TODO(antonin): add an iterator for entries to MatchUnitGeneric<K, V> ?
  handle_iterator handles_begin() const;
  handle_iterator handles_end() const;

 protected:
  MatchErrorCode get_and_set_handle(internal_handle_t *handle);
  MatchErrorCode unset_handle(internal_handle_t handle);
  bool valid_handle_(internal_handle_t handle) const;

  void build_key(const PHV &phv, ByteContainer *key) const {
    match_key_builder(phv, key);
  }

  std::string key_to_string(const ByteContainer &key) const {
    return match_key_builder.key_to_string(key);
  }

  std::string key_to_string_with_names(const ByteContainer &key) const;

  void update_counters(Counter *c, const Packet &pkt) {
    c->increment_counter(pkt);
  }

  void update_ts(MatchUnit::AtomicTimestamp *ts, const Packet &pkt) {
    ts->set(pkt.get_ingress_ts_ms());
  }

 protected:
  ~MatchUnitAbstract_() { }

 protected:
  size_t size{0};
  size_t num_entries{0};
  size_t nbytes_key;
  HandleMgr handles{};
  MatchKeyBuilder match_key_builder;
  std::vector<MatchUnit::EntryMeta> entry_meta{};
  // non-owning pointer, the meter array still belongs to P4Objects
  MeterArray *direct_meters{nullptr};
};

template <typename V>
class MatchUnitAbstract : public MatchUnitAbstract_ {
 public:
  struct MatchUnitLookup {
    MatchUnitLookup(entry_handle_t handle, const V *value)
      : handle(handle), value(value) { }

    bool found() const { return (value != nullptr); }

    static MatchUnitLookup empty_entry() { return MatchUnitLookup(0, nullptr); }

    entry_handle_t handle{0};
    const V *value{nullptr};
  };

 public:
  MatchUnitAbstract(size_t size, const MatchKeyBuilder &match_key_builder)
    : MatchUnitAbstract_(size, match_key_builder) { }

  virtual ~MatchUnitAbstract() { }

  MatchUnitLookup lookup(const Packet &pkt);

  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
                           V value,  // by value for possible std::move
                           entry_handle_t *handle,
                           int priority = -1);

  MatchErrorCode delete_entry(entry_handle_t handle);

  MatchErrorCode modify_entry(entry_handle_t handle, V value);

  MatchErrorCode get_value(entry_handle_t handle, const V **value);

  MatchErrorCode get_entry(entry_handle_t handle,
                           std::vector<MatchKeyParam> *match_key,
                           const V **value, int *priority = nullptr) const;

  // TODO(antonin): move this one level up in class hierarchy?
  // will return an empty string if the handle is not valid
  // otherwise will return a dump of the match entry in a nice format
  // Dumping entry <handle>
  // Match key:
  //   param_1
  //   param_2 ...
  // [Priority: ...]
  // Does not print anything related to the stored value
  std::string entry_to_string(entry_handle_t handle) const;

  MatchErrorCode dump_match_entry(std::ostream *out,
                                  entry_handle_t handle) const;

  void reset_state();

  void serialize(std::ostream *out) const {
    serialize_(out);
  }

  void deserialize(std::istream *in, const P4Objects &objs) {
    deserialize_(in, objs);
  }

 private:
  virtual MatchErrorCode add_entry_(const std::vector<MatchKeyParam> &match_key,
                                    V value,  // by value for possible std::move
                                    entry_handle_t *handle,
                                    int priority) = 0;

  virtual MatchErrorCode delete_entry_(entry_handle_t handle) = 0;

  virtual MatchErrorCode modify_entry_(entry_handle_t handle, V value) = 0;

  virtual MatchErrorCode get_value_(entry_handle_t handle, const V **value) = 0;

  virtual MatchErrorCode get_entry_(entry_handle_t handle,
                                    std::vector<MatchKeyParam> *match_key,
                                    const V **value, int *priority) const = 0;

  virtual MatchErrorCode dump_match_entry_(std::ostream *out,
                                           entry_handle_t handle) const = 0;

  virtual void reset_state_() = 0;

  virtual MatchUnitLookup lookup_key(const ByteContainer &key) const = 0;

  virtual void serialize_(std::ostream *out) const = 0;
  virtual void deserialize_(std::istream *in, const P4Objects &objs) = 0;
};


template <typename K, typename V>
class MatchUnitGeneric : public MatchUnitAbstract<V> {
 public:
  typedef typename MatchUnitAbstract<V>::MatchUnitLookup MatchUnitLookup;
  struct Entry {
    Entry() {}
    Entry(K key, V value)
      : key(std::move(key)), value(std::move(value)) {}
    K key;
    V value;
  };

 public:
  MatchUnitGeneric(size_t size, const MatchKeyBuilder &match_key_builder,
                   LookupStructureFactory *lookup_factory)
    : MatchUnitAbstract<V>(size, match_key_builder), entries(size),
      lookup_structure(
        LookupStructureFactory::create<K>(
          lookup_factory, size, match_key_builder.get_nbytes_key())) {}

 private:
  MatchErrorCode add_entry_(const std::vector<MatchKeyParam> &match_key,
                            V value,  // by value for possible std::move
                            entry_handle_t *handle,
                            int priority) override;

  MatchErrorCode delete_entry_(entry_handle_t handle) override;

  MatchErrorCode modify_entry_(entry_handle_t handle, V value) override;

  MatchErrorCode get_value_(entry_handle_t handle, const V **value) override;

  MatchErrorCode get_entry_(entry_handle_t handle,
                            std::vector<MatchKeyParam> *match_key,
                            const V **value, int *priority) const override;

  MatchErrorCode dump_match_entry_(std::ostream *out,
                                   entry_handle_t handle) const override;

  void reset_state_() override;

  MatchUnitLookup lookup_key(const ByteContainer &key) const override;

  void serialize_(std::ostream *out) const override;
  void deserialize_(std::istream *in, const P4Objects &objs) override;

 private:
  std::vector<Entry> entries{};
  std::unique_ptr<LookupStructure<K>> lookup_structure{nullptr};
};

// Alias all of our concrete MatchUnit types for convenience
// when using them elsewhere.

template <typename V>
using MatchUnitExact = MatchUnitGeneric<ExactMatchKey, V>;

template <typename V>
using MatchUnitLPM = MatchUnitGeneric<LPMMatchKey, V>;

template <typename V>
using MatchUnitTernary = MatchUnitGeneric<TernaryMatchKey, V>;

template <typename V>
using MatchUnitRange = MatchUnitGeneric<RangeMatchKey, V>;


}  // namespace bm

#endif  // BM_BM_SIM_MATCH_UNITS_H_
