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

#ifndef _BM_MATCH_TABLES_H_
#define _BM_MATCH_TABLES_H_

#include <vector>
#include <type_traits>
#include <iostream>

#include "match_units.h"
#include "actions.h"
#include "ras.h"
#include "calculations.h"

class MatchTableAbstract : public NamedP4Object
{
public:
  typedef Counter::counter_value_t counter_value_t;

  struct ActionEntry {
    ActionEntry() { }

    ActionEntry(ActionFnEntry action_fn, const ControlFlowNode *next_node)
      : action_fn(std::move(action_fn)), next_node(next_node) { }

    void dump(std::ostream &stream) const {
      action_fn.dump(stream);
    }

    friend std::ostream& operator<<(std::ostream &out, const ActionEntry &e) {
      e.dump(out);
      return out;
    }

    ActionEntry(const ActionEntry &other) = delete;
    ActionEntry &operator=(const ActionEntry &other) = delete;
    
    ActionEntry(ActionEntry &&other) /*noexcept*/ = default;
    ActionEntry &operator=(ActionEntry &&other) /*noexcept*/ = default;

    ActionFnEntry action_fn{};
    const ControlFlowNode *next_node{nullptr};
  };

public:
  MatchTableAbstract(const std::string &name, p4object_id_t id,
		     size_t size, bool with_counters, bool with_ageing,
		     MatchUnitAbstract_ *mu)
    : NamedP4Object(name, id), size(size),
      with_counters(with_counters), with_ageing(with_ageing),
      match_unit_(mu) { }

  virtual ~MatchTableAbstract() { }

  const ControlFlowNode *apply_action(Packet *pkt);

  virtual const ActionEntry &lookup(const Packet &pkt, bool *hit,
				    entry_handle_t *handle) = 0;

  virtual size_t get_num_entries() const = 0;

  virtual bool is_valid_handle(entry_handle_t handle) const = 0;

  virtual void dump(std::ostream &stream) const = 0;

  void reset_state();

  void set_next_node(p4object_id_t action_id, const ControlFlowNode *next_node) {
    next_nodes[action_id] = next_node;
  }

  MatchErrorCode query_counters(entry_handle_t handle,
				counter_value_t *bytes,
				counter_value_t *packets) const;
  MatchErrorCode reset_counters();
  MatchErrorCode write_counters(entry_handle_t handle,
				counter_value_t bytes,
				counter_value_t packets);

  MatchErrorCode set_entry_ttl(entry_handle_t handle, unsigned int ttl_ms);

  void sweep_entries(std::vector<entry_handle_t> &entries) const;

  MatchTableAbstract(const MatchTableAbstract &other) = delete;
  MatchTableAbstract &operator=(const MatchTableAbstract &other) = delete;
  
  MatchTableAbstract(MatchTableAbstract &&other) = delete;
  MatchTableAbstract &operator=(MatchTableAbstract &&other) = delete;

protected:
  typedef boost::shared_lock<boost::shared_mutex> ReadLock;
  typedef boost::unique_lock<boost::shared_mutex> WriteLock;

protected:
  const ControlFlowNode *get_next_node(p4object_id_t action_id) const {
    return next_nodes.at(action_id);
  }

  ReadLock lock_read() const { return ReadLock(t_mutex); }
  WriteLock lock_write() const { return WriteLock(t_mutex); }
  void unlock(ReadLock &lock) const { lock.unlock(); }
  void unlock(WriteLock &lock) const { lock.unlock(); }

protected:
  size_t size{0};

  std::atomic_bool with_counters{false};
  std::atomic_bool with_ageing{false};

  std::unordered_map<p4object_id_t, const ControlFlowNode *> next_nodes{};

private:
  virtual void reset_state_() = 0;

private:
  mutable boost::shared_mutex t_mutex{};
  MatchUnitAbstract_ *match_unit_{nullptr};
};

// MatchTable is exposed to the runtime for configuration

class MatchTable : public MatchTableAbstract
{
public:
  typedef MatchTableAbstract::ActionEntry ActionEntry;

public:
  MatchTable(const std::string &name, p4object_id_t id,
	     std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit,
	     bool with_counters = false, bool with_ageing = false) 
    : MatchTableAbstract(name, id, match_unit->get_size(),
			 with_counters, with_ageing,
			 match_unit.get()),
      match_unit(std::move(match_unit)) { }

  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			   const ActionFn *action_fn,
			   ActionData action_data, // move it
			   entry_handle_t *handle,
			   int priority = -1);

  MatchErrorCode delete_entry(entry_handle_t handle);

  MatchErrorCode modify_entry(entry_handle_t handle,
			      const ActionFn *action_fn,
			      ActionData action_data);

  MatchErrorCode set_default_action(const ActionFn *action_fn,
				    ActionData action_data);

  const ActionEntry &lookup(const Packet &pkt, bool *hit,
			    entry_handle_t *handle) override;

  size_t get_num_entries() const override {
    return match_unit->get_num_entries();
  }

  bool is_valid_handle(entry_handle_t handle) const override {
    return match_unit->valid_handle(handle);
  }

  void dump(std::ostream &stream) const override;

public:
  static std::unique_ptr<MatchTable> create(
    const std::string &match_type, 
    const std::string &name, p4object_id_t id,
    size_t size, const MatchKeyBuilder &match_key_builder,
    bool with_counters, bool with_ageing
  );

private:
  void reset_state_() override;

private:
  ActionEntry default_entry{};
  std::unique_ptr<MatchUnitAbstract<ActionEntry> > match_unit;

};

class MatchTableIndirect : public MatchTableAbstract
{
public:
  typedef MatchTableAbstract::ActionEntry ActionEntry;

  typedef uintptr_t mbr_hdl_t;

  class IndirectIndex {
  public:
    IndirectIndex() { }

    bool is_mbr() const { return ((index >> 24) == _mbr); }
    bool is_grp() const { return ((index >> 24) == _grp); }
    unsigned int get() const { return (index & _index_mask); }
    unsigned int get_mbr() const { assert(is_mbr()); return get(); }
    unsigned int get_grp() const { assert(is_grp()); return get(); }

    void dump(std::ostream &stream) const {
      if(is_grp())
	stream << "group(" << get() << ")";
      else
	stream << "member(" << get() << ")";
    }

    static IndirectIndex make_mbr_index(unsigned int index) {
      assert(index <= _index_mask);
      return IndirectIndex((_mbr << 24) | index);
    }

    static IndirectIndex make_grp_index(unsigned int index) {
      assert(index <= _index_mask);
      return IndirectIndex((_grp << 24) | index);
    }

    // template<typename UT, typename = typename std::enable_if<std::is_unsigned<UT>::value, UT>::type> operator UT() {
    //   static_assert(sizeof(UT) >= sizeof(unsigned int), "cannot convert");
    //   return static_cast<UT>(i);
    // }

  private:
    IndirectIndex(unsigned int index) : index(index) { }

    static const unsigned char _mbr = 0x00;
    static const unsigned char _grp = 0x01;
    static const unsigned int _index_mask = 0x00FFFFFF;

    unsigned int index{0};
  };

  class IndirectIndexRefCount {
  public:
    typedef unsigned int count_t;

  public:
    void set(const IndirectIndex &index, count_t value) {
      unsigned int i = index.get();
      auto &v = (index.is_mbr()) ? mbr_count : grp_count;
      assert(i <= v.size());
      if(i == v.size())
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
      if(index.is_mbr())
	mbr_count[i]++;
      else
	grp_count[i]++;
    }

    void decrease(const IndirectIndex &index) {
      unsigned int i = index.get();
      if(index.is_mbr())
	mbr_count[i]--;
      else
	grp_count[i]--;
    }
    
  private:
    std::vector<count_t> mbr_count{};
    std::vector<count_t> grp_count{};
  };

public:
  MatchTableIndirect(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit,
    bool with_counters = false, bool with_ageing = false
  ) 
    : MatchTableAbstract(name, id, match_unit->get_size(),
			 with_counters, with_ageing,
			 match_unit.get()),
      match_unit(std::move(match_unit)) { }

  MatchErrorCode add_member(const ActionFn *action_fn,
			    ActionData action_data, // move it
			    mbr_hdl_t *mbr);

  MatchErrorCode delete_member(mbr_hdl_t mbr);

  MatchErrorCode modify_member(mbr_hdl_t mbr,
			       const ActionFn *action_fn,
			       ActionData action_data);

  MatchErrorCode add_entry(const std::vector<MatchKeyParam> &match_key,
			   mbr_hdl_t mbr,
			   entry_handle_t *handle,
			   int priority = -1);

  MatchErrorCode delete_entry(entry_handle_t handle);

  MatchErrorCode modify_entry(entry_handle_t handle, mbr_hdl_t mbr);

  MatchErrorCode set_default_member(mbr_hdl_t mbr);

  const ActionEntry &lookup(const Packet &pkt, bool *hit,
			    entry_handle_t *handle) override;

  size_t get_num_entries() const override {
    return match_unit->get_num_entries();
  }

  bool is_valid_handle(entry_handle_t handle) const override {
    return match_unit->valid_handle(handle);
  }

  void dump(std::ostream &stream) const override;

  size_t get_num_members() const {
    return num_members;
  }

public:
  static std::unique_ptr<MatchTableIndirect> create(
    const std::string &match_type, 
    const std::string &name, p4object_id_t id,
    size_t size, const MatchKeyBuilder &match_key_builder,
    bool with_counters, bool with_ageing
  );

protected:
  bool is_valid_mbr(mbr_hdl_t mbr) const {
    return mbr_handles.valid_handle(mbr);
  }

  void reset_state_() override;

  void dump_(std::ostream &stream) const;

protected:
  IndirectIndex default_index{};
  IndirectIndexRefCount index_ref_count{};
  HandleMgr mbr_handles{};
  std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit;
  std::vector<ActionEntry> action_entries{};

private:
  void entries_insert(mbr_hdl_t mbr, ActionEntry &&entry);

private:
  size_t num_members{0};
};

class MatchTableIndirectWS : public MatchTableIndirect
{
public:
  typedef uintptr_t grp_hdl_t;

  typedef unsigned int hash_t;

public:
  MatchTableIndirectWS(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchUnitAbstract<IndirectIndex> > match_unit,
    bool with_counters = false, bool with_ageing = false
  ) 
    : MatchTableIndirect(name, id, std::move(match_unit),
			 with_counters, with_ageing) { }

  void set_hash(std::unique_ptr<Calculation> h) {
    hash = std::move(h);
  }

  MatchErrorCode create_group(grp_hdl_t *grp);

  MatchErrorCode delete_group(grp_hdl_t grp);

  MatchErrorCode add_member_to_group(mbr_hdl_t mbr, grp_hdl_t grp);

  MatchErrorCode remove_member_from_group(mbr_hdl_t mbr, grp_hdl_t grp);

  MatchErrorCode add_entry_ws(const std::vector<MatchKeyParam> &match_key,
			      grp_hdl_t grp,
			      entry_handle_t *handle,
			      int priority = -1);

  MatchErrorCode modify_entry_ws(entry_handle_t handle, grp_hdl_t grp);

  MatchErrorCode set_default_group(grp_hdl_t grp);

  const ActionEntry &lookup(const Packet &pkt, bool *hit,
			    entry_handle_t *handle) override;

  size_t get_num_groups() const {
    return num_groups;
  }

  MatchErrorCode get_num_members_in_group(grp_hdl_t grp, size_t *nb) const;

  void dump(std::ostream &stream) const override;

public:
  static std::unique_ptr<MatchTableIndirectWS> create(
    const std::string &match_type, 
    const std::string &name, p4object_id_t id,
    size_t size, const MatchKeyBuilder &match_key_builder,
    bool with_counters, bool with_ageing
  );

protected:
  bool is_valid_grp(grp_hdl_t grp) const {
    return grp_handles.valid_handle(grp);
  }

  size_t get_grp_size(grp_hdl_t grp) const {
    return group_entries[grp].size();
  }

private:
  void groups_insert(grp_hdl_t grp);

  mbr_hdl_t choose_from_group(grp_hdl_t grp, const Packet &pkt) const;

  void reset_state_() override;

private:
  class GroupInfo {
  public:
    typedef RandAccessUIntSet::iterator iterator;
    typedef RandAccessUIntSet::const_iterator const_iterator;

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

    void dump(std::ostream &stream) const;

  private:
    RandAccessUIntSet mbrs{};
  };

private:
  HandleMgr grp_handles{};
  size_t num_groups{0};
  std::vector<GroupInfo> group_entries{};
  std::unique_ptr<Calculation> hash{nullptr};
};

#endif
