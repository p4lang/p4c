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

#ifndef BM_BM_SIM_ACTION_PROFILE_H_
#define BM_BM_SIM_ACTION_PROFILE_H_

// shared_mutex will only be available in C++-14, so for now I'm using boost
#include <boost/thread/shared_mutex.hpp>

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "action_entry.h"
#include "calculations.h"
#include "handle_mgr.h"
#include "match_error_codes.h"
#include "ras.h"

namespace bm {

class ActionProfile : public NamedP4Object {
  friend class MatchTableIndirect;
  friend class MatchTableIndirectWS;

 public:
  using mbr_hdl_t = uint32_t;
  using grp_hdl_t = uint32_t;

  using hash_t = unsigned int;

  class IndirectIndex {
   public:
    IndirectIndex() { }

    bool is_mbr() const { return ((index >> 24) == _mbr); }
    bool is_grp() const { return ((index >> 24) == _grp); }
    unsigned int get() const { return (index & _index_mask); }
    unsigned int get_mbr() const { assert(is_mbr()); return get(); }
    unsigned int get_grp() const { assert(is_grp()); return get(); }

    void dump(std::ostream *stream) const {
      if (is_grp())
        (*stream) << "group(" << get() << ")";
      else
        (*stream) << "member(" << get() << ")";
    }

    friend std::ostream& operator<<(std::ostream &out, const IndirectIndex &i) {
      i.dump(&out);
      return out;
    }

    void serialize(std::ostream *out) const;
    void deserialize(std::istream *in, const P4Objects &objs);

    static IndirectIndex make_mbr_index(unsigned int index) {
      assert(index <= _index_mask);
      return IndirectIndex((_mbr << 24) | index);
    }

    static IndirectIndex make_grp_index(unsigned int index) {
      assert(index <= _index_mask);
      return IndirectIndex((_grp << 24) | index);
    }

   private:
    explicit IndirectIndex(unsigned int index) : index(index) { }

    static const unsigned char _mbr = 0x00;
    static const unsigned char _grp = 0x01;
    static const unsigned int _index_mask = 0x00FFFFFF;

    unsigned int index{0};
  };

  struct Member {
    mbr_hdl_t mbr;
    const ActionFn *action_fn;
    ActionData action_data;
  };

  struct Group {
    grp_hdl_t grp;
    std::vector<mbr_hdl_t> mbr_handles;
  };

  class GroupSelectionIface {
   public:
    virtual ~GroupSelectionIface() { }

    virtual void add_member_to_group(grp_hdl_t grp, mbr_hdl_t mbr) = 0;
    virtual void remove_member_from_group(grp_hdl_t grp, mbr_hdl_t mbr) = 0;
    virtual mbr_hdl_t get_from_hash(grp_hdl_t grp, hash_t h) const = 0;
    virtual void reset() = 0;
  };

  ActionProfile(const std::string &name, p4object_id_t id, bool with_selection);

  bool has_selection() const;

  void set_hash(std::unique_ptr<Calculation> h) {
    hash = std::move(h);
  }

  // give the target some control over the member selection process
  void set_group_selector(std::shared_ptr<GroupSelectionIface> selector);

  // runtime interface

  MatchErrorCode add_member(const ActionFn *action_fn,
                            ActionData action_data,  // move it
                            mbr_hdl_t *mbr);

  MatchErrorCode delete_member(mbr_hdl_t mbr);

  MatchErrorCode modify_member(mbr_hdl_t mbr,
                               const ActionFn *action_fn,
                               ActionData action_data);

  MatchErrorCode create_group(grp_hdl_t *grp);

  MatchErrorCode delete_group(grp_hdl_t grp);

  MatchErrorCode add_member_to_group(mbr_hdl_t mbr, grp_hdl_t grp);

  MatchErrorCode remove_member_from_group(mbr_hdl_t mbr, grp_hdl_t grp);

  // end of runtime interface

  MatchErrorCode get_member(mbr_hdl_t mbr, Member *member) const;

  std::vector<Member> get_members() const;

  MatchErrorCode get_group(grp_hdl_t grp, Group *group) const;

  std::vector<Group> get_groups() const;

  size_t get_num_members() const;
  size_t get_num_groups() const;

  MatchErrorCode get_num_members_in_group(grp_hdl_t grp, size_t *nb) const;

  void reset_state();

  void serialize(std::ostream *out) const;
  void deserialize(std::istream *in, const P4Objects &objs);

 private:
  using ReadLock = boost::shared_lock<boost::shared_mutex>;
  using WriteLock = boost::unique_lock<boost::shared_mutex>;

  class IndirectIndexRefCount {
   public:
    using count_t = unsigned int;

   public:
    void set(const IndirectIndex &index, count_t value) {
      unsigned int i = index.get();
      auto &v = (index.is_mbr()) ? mbr_count : grp_count;
      assert(i <= v.size());
      if (i == v.size())
        v.push_back(value);
      else
        v[i] = value;
    }

    count_t get(const IndirectIndex &index) const {
      unsigned int i = index.get();
      return (index.is_mbr()) ? mbr_count[i] : grp_count[i];
    }

    void increase(const IndirectIndex &index) {
      unsigned int i = index.get();
      if (index.is_mbr())
        mbr_count[i]++;
      else
        grp_count[i]++;
    }

    void decrease(const IndirectIndex &index) {
      unsigned int i = index.get();
      if (index.is_mbr())
        mbr_count[i]--;
      else
        grp_count[i]--;
    }

    void serialize(std::ostream *out) const;
    void deserialize(std::istream *in);

   private:
    std::vector<count_t> mbr_count{};
    std::vector<count_t> grp_count{};
  };

  class GroupInfo {
   public:
    using iterator = RandAccessUIntSet::iterator;
    using const_iterator = RandAccessUIntSet::const_iterator;

    MatchErrorCode add_member(mbr_hdl_t mbr);
    MatchErrorCode delete_member(mbr_hdl_t mbr);
    bool contains_member(mbr_hdl_t mbr) const;
    size_t size() const;
    mbr_hdl_t get_nth(size_t n) const;

    // iterators
    iterator begin() { return mbrs.begin(); }
    const_iterator begin() const { return mbrs.begin(); }
    iterator end() { return mbrs.end(); }
    const_iterator end() const { return mbrs.end(); }

    void serialize(std::ostream *out) const;
    void deserialize(std::istream *in);

   private:
    RandAccessUIntSet mbrs{};
  };

  class GroupMgr : public GroupSelectionIface {
   public:
    void add_member_to_group(grp_hdl_t grp, mbr_hdl_t mbr) override;
    void remove_member_from_group(grp_hdl_t grp, mbr_hdl_t mbr) override;

    mbr_hdl_t get_from_hash(grp_hdl_t grp, hash_t h) const override;

    void reset() override;

    void insert_group(grp_hdl_t grp);

    size_t group_size(grp_hdl_t grp) const;

    GroupInfo &at(grp_hdl_t grp);
    const GroupInfo &at(grp_hdl_t grp) const;

    void clear();

   private:
    std::vector<GroupInfo> groups{};
  };

 private:
  mbr_hdl_t choose_from_group(grp_hdl_t grp, const Packet &pkt) const;

  MatchErrorCode get_member_(mbr_hdl_t handle, Member *member) const;

  MatchErrorCode get_group_(grp_hdl_t grp, Group *group) const;

  void entries_insert(mbr_hdl_t mbr, ActionEntry &&entry);

  // lock can be acquired by MatchTableIndirect
  ReadLock lock_read() const { return ReadLock(t_mutex); }
  WriteLock lock_write() const { return WriteLock(t_mutex); }

  // this method is called by MatchTableIndirect and assumes that the provided
  // index is correct
  void dump_entry(std::ostream *out, const IndirectIndex &index) const;

  // called by MatchTableIndirect for reference counting (a member cannot be
  // deleted if match entries are pointing to it)
  void ref_count_increase(const IndirectIndex &index);
  void ref_count_decrease(const IndirectIndex &index);

  bool is_valid_mbr(mbr_hdl_t mbr) const {
    return mbr_handles.valid_handle(mbr);
  }

  bool is_valid_grp(grp_hdl_t grp) const {
    return grp_handles.valid_handle(grp);
  }

  bool group_is_empty(grp_hdl_t grp) const;

  const ActionEntry &lookup(const Packet &pkt,
                            const IndirectIndex &index) const;

 private:
  mutable boost::shared_mutex t_mutex{};
  bool with_selection;
  std::vector<ActionEntry> action_entries{};
  IndirectIndexRefCount index_ref_count{};
  HandleMgr mbr_handles{};
  HandleMgr grp_handles{};
  size_t num_members{0};
  size_t num_groups{0};
  GroupMgr grp_mgr{};
  std::shared_ptr<GroupSelectionIface> grp_selector_{nullptr};
  GroupSelectionIface *grp_selector{&grp_mgr};
  std::unique_ptr<Calculation> hash{nullptr};
};

}  // namespace bm

#endif  // BM_BM_SIM_ACTION_PROFILE_H_
