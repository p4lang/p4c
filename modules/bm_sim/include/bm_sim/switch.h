#ifndef _BM_SWITCH_H_
#define _BM_SWITCH_H_

#include <memory>
#include <string>
#include <fstream>

#include "P4Objects.h"
#include "queue.h"
#include "packet.h"
#include "runtime_interface.h"

class Switch : public RuntimeInterface {
public:
  Switch();

  void init_objects(const std::string &json_path);

  P4Objects *get_p4objects() { return p4objects.get(); }

  virtual int receive(int port_num, const char *buffer, int len) = 0;

  virtual void start_and_return() = 0;

public:
  int table_add_entry(const std::string &table_name,
		      ExactMatchEntry &&entry,
		      entry_handle_t *handle) override;
  int table_add_entry(const std::string &table_name,
		      LongestPrefixMatchEntry &&entry,
		      entry_handle_t *handle) override;
  int table_add_entry(const std::string &table_name,
		      TernaryMatchEntry &&entry,
		      entry_handle_t *handle) override;

  int table_add_exact_match_entry(const std::string &table_name,
				  const std::string &action_name,
				  const ByteContainer &key,
				  const ActionData &action_data,
				  entry_handle_t *handle) override;

  int table_add_lpm_entry(const std::string &table_name,
			  const std::string &action_name,
			  const ByteContainer &key,
			  unsigned int prefix_length,
			  const ActionData &action_data,
			  entry_handle_t *handle) override;

  int table_add_ternary_match_entry(const std::string &table_name,
				    const std::string &action_name,
				    const ByteContainer &key,
				    const ByteContainer &mask,
				    int priority,
				    const ActionData &action_data,
				    entry_handle_t *handle) override;

  int table_set_default_action(const std::string &table_name,
			       const std::string &action_name,
			       const ActionData &action_data) override;

  int table_delete_entry(const std::string &table_name,
			 entry_handle_t handle) override;

protected:
  std::unique_ptr<P4Objects> p4objects;
};

#endif
