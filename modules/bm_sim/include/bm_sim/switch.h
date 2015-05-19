#ifndef _BM_SWITCH_H_
#define _BM_SWITCH_H_

#include <memory>
#include <string>
#include <fstream>

#include "P4Objects.h"
#include "queue.h"
#include "packet.h"
#include "learning.h"
#include "runtime_interface.h"
#include "pre.h"

#include <boost/thread/shared_mutex.hpp>

class Switch : public RuntimeInterface {
public:
  Switch(bool enable_swap = false);

  void init_objects(const std::string &json_path);

  P4Objects *get_p4objects() { return p4objects.get(); }

  virtual int receive(int port_num, const char *buffer, int len) = 0;

  virtual void start_and_return() = 0;

public:
  MatchTable::ErrorCode 
  table_add_entry(const std::string &table_name,
		  const std::vector<MatchKeyParam> &match_key,
		  const std::string &action_name,
		  const ActionData &action_data,
		  entry_handle_t *handle,
		  int priority = -1/*only used for ternary*/) override;

  MatchTable::ErrorCode
  table_set_default_action(const std::string &table_name,
			   const std::string &action_name,
			   const ActionData &action_data) override;
  
  MatchTable::ErrorCode
  table_delete_entry(const std::string &table_name,
		     entry_handle_t handle) override;

  MatchTable::ErrorCode
  table_modify_entry(const std::string &table_name,
		     entry_handle_t handle,
		     const std::string &action_name,
		     const ActionData &action_data) override;

  MatchTable::ErrorCode
  table_read_counters(const std::string &table_name,
		      entry_handle_t handle,
		      MatchTable::counter_value_t *bytes,
		      MatchTable::counter_value_t *packets) override;

  MatchTable::ErrorCode
  table_reset_counters(const std::string &table_name) override;

  RuntimeInterface::ErrorCode
  load_new_config(const std::string &new_config) override;

  RuntimeInterface::ErrorCode
  swap_configs() override;

  LearnEngine *get_learn_engine();

  McPre *get_pre() { return pre.get(); }

protected:
  int swap_requested() { return swap_ordered; }
  // TODO: should I return shared_ptrs instead of raw_ptrs?
  // invalidate all pointers obtained with get_pipeline(), get_parser(),...
  int do_swap();

  // do these methods need any protection
  Pipeline *get_pipeline(const std::string &name) {
    return p4objects->get_pipeline(name);
  }

  Parser *get_parser(const std::string &name) {
    return p4objects->get_parser(name);
  }

  Deparser *get_deparser(const std::string &name) {
    return p4objects->get_deparser(name);
  }

protected:
  std::unique_ptr<McPre> pre{nullptr};

private:
  mutable boost::shared_mutex request_mutex{};
  std::atomic<bool> swap_ordered{false};

  std::shared_ptr<P4Objects> p4objects{nullptr};
  std::shared_ptr<P4Objects> p4objects_rt{nullptr};

  bool enable_swap{false};
};

#endif
