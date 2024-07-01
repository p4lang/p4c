// The following extern functions are "forwarding" functions -- they
// all set the destination of the packet.  Calling one of them
// overwrites and replaces the effect of any earlier call to any of
// the functions in this set.  Only the last one executed will
// actually take effect for the packet.

// + drop_packet
// + send_to_port


// drop_packet() - Cause the packet to be dropped when it finishes
// completing the main control.
//
// Invoking drop_packet() is supported only within the main control.

extern void drop_packet();

extern void send_to_port(PortId_t dest_port);

extern void mirror_packet(MirrorSlotId_t mirror_slot_id,
                          MirrorSessionId_t mirror_session_id);

// TBD: Does it make sense to have a data plane add of a hit action
// that has in, out, or inout parameters?
//
// TBD: Should we require the return value?  Can most targets
// implement it?  If not, consider having two separate variants of
// add_entry, one with no return value (i.e. type void).  Such a
// variant of add_entry seems difficult to use correctly, if it is
// possible for entries to fail to be added.

// BEGIN:add_entry_extern_function
// The bit width of this type is allowed to be different for different
// target devices.  It must be at least a 1-bit wide type.

typedef bit<1> AddEntryErrorStatus_t;

const AddEntryErrorStatus_t ADD_ENTRY_SUCCESS = 0;
const AddEntryErrorStatus_t ADD_ENTRY_NOT_DONE = 1;

// Targets may define target-specific non-0 constants of type
// AddEntryErrorStatus_t if they wish.

// The add_entry() extern function causes an entry, i.e. a key and its
// corresponding action and action parameter values, to be added to a
// table from the data plane, i.e. without the control plane having to
// take any action at all to cause the table entry to be added.
//
// The key of the new entry added will always be the same as the key
// that was just looked up in the table, and experienced a miss.
//
// `action_name` is the name of an action that must satisfy these
// restrictions:
// + It must be an action that is in the list specified as the
//   `actions` property of the table.
// + It must be possible for this action to be the action of an entry
//   added to the table, e.g. it is an error if the action has the
//   annotation `@defaultonly`.
// + The action to be added must not itself contain a call to
//   add_entry(), or anything else that is not supported in a table's
//   "hit action".
//
// Type T must be a struct type whose field names have the same name
// as the parameters of the action being added, in the same order, and
// have the same type as the corresponding action parameters.
//
// `action_params` will become the action parameters of the new entry
// to be added.
//
// `expire_time_profile_id` is the initial expire time profile id of
// the entry added.
//
// The return value will be ADD_ENTRY_SUCCESS if the entry was
// successfully added, otherwise it will be some other value not equal
// to ADD_ENTRY_SUCCESS.  Targets are allowed to define only one
// failure return value, or several if they wish to provide more
// detail on the reason for the failure to add the entry.
//
// It is NOT defined by PNA, and need not be supported by PNA
// implementations, to call add_entry() within an action that is added
// as an entry of a table, i.e. as a "hit action".  It is only defined
// if called within an action that is the default_action, i.e. a "miss
// action" of a table.
//
// For tables with `add_on_miss = true`, some PNA implementations
// might only support `default_action` with the `const` qualifier.
// However, if a PNA implementation can support run-time modifiable
// default actions for such a table, some of which call add_entry()
// and some of which do not, the behavior of such an implementation is
// defined by PNA, and this may be a useful feature.

extern AddEntryErrorStatus_t add_entry<T>(
    string action_name,
    in T action_params,
    in ExpireTimeProfileId_t expire_time_profile_id);
// END:add_entry_extern_function

// The following call to add_entry_if():
//
//     add_entry_if(expr, action_name, action_params, expire_time_profile_id);
//
// has exactly the same behavior as the following expression:
//
//     (expr) ? add_entry(action_name, action_params, expire_time_profile_id)
//            : ADD_ENTRY_NOT_DONE;
//
// and it has the same restrictions on where it can appear in a P4
// program as that equivalent code.
//
// Rationale: At the time PNA was being designed in 2022, there were
// P4 targets, including the BMv2 software switch in the repository
// https://github.com/p4lang/behavioral-model, that did not fully
// support `if` statements within P4 actions.  add_entry_if() enables
// writing P4 code without `if` statements within P4 actions that
// would otherwise require an `if` statement to express the desired
// behavior.  Admittedly, this is a work-around for targets with
// limited support for `if` statements within P4 actions.  See
// https://github.com/p4lang/pna/issues/63 for more details.

extern AddEntryErrorStatus_t add_entry_if<T>(
    in bool do_add_entry,
    string action_name,
    in T action_params,
    in ExpireTimeProfileId_t expire_time_profile_id);

extern FlowId_t allocate_flow_id();


// set_entry_expire_time() may only be called from within an action of
// a table with property 'pna_idle_timeout' having a value of
// `NOTIFY_CONTROL` or `AUTO_DELETE`.

// Calling it causes the expiration time profile id of the matched
// entry to become equal to expire_time_profile_id.

// It _also_ behaves as if restart_expire_timer() was called.

extern void set_entry_expire_time(
    in ExpireTimeProfileId_t expire_time_profile_id);


// restart_expire_timer() may only be called from within an action of
// a table with property 'pna_idle_timeout' having a value of
// `NOTIFY_CONTROL` or `AUTO_DELETE`.

// Calling it causes the dynamic expiration timer of the entry to be
// reset, so that the entry will remain active from the now until that
// time in the future.

// TODO: Note that there are targets that support a table with
// property pna_idle_timeout equal to `NOTIFY_CONTROL` or
// `AUTO_DELETE` such that matching an entry _does not_ cause this
// side effect to occur, i.e. it is possible to match an entry and
// _not_ do what `restart_expire_timer()` does.  We should document
// this explicitly as common for PNA devices, if that is agreed upon.
// Andy is pretty sure that PSA devices were not expected to have this
// option.

// Note that for the PSA architecture with psa_idle_timeout =
// PSA_IdleTimeout_t.NOTIFY_CONTROL, I believe there was an implicit
// assumption that _every_ time a table entry was matched, the target
// behaved as if restart_expire_timer() was called, but there was no
// such extern function defined by PSA.

// The proposal for PNA is that it is possible to match an entry, but
// _not_ call restart_expire_timer(), and this will cause the data
// plane _not_ to restart the table entry's expiration time.  That is,
// the expiration time of the entry will continue to be the same as
// before it was matched.

extern void restart_expire_timer();

// The effect of this call:
//
//     update_expire_info(a, b, c)
//
// is exactly the same as the effect of the following code:
//
//    if (a) {
//        if (b) {
//            set_entry_expire_time(c);
//        } else {
//            restart_expire_timer();
//        }
//    }

extern void update_expire_info(
    in bool update_aging_info,
    in bool update_expire_time,
    in ExpireTimeProfileId_t expire_time_profile_id);


// Set the expire time of the matched entry in the table to the value
// specified in the parameter expire_time_profile_id, if condition
// in the first parameter evaluates to true. Otherwise, the function
// has no effect.
//
// @param condition  The boolean expression to evaluate to determine
//                   if the expire time will be set.
// @param expire_time_profile_id
//                   The expire time to set for the matched entry,
//                   if the data and value parameters are equal.
//
// Examples:
// set_expire_time_if(hdr.tcp.flags == TCP_FLG_SYN &&
//                    meta.direction == OUTBOUND,
//                    tcp_connection_start_time_profile_id);
// set_expire_time_if(hdr.tcp.flags == TCP_FLG_ACK,
//                    tcp_connection_continuation_time_protile_id);
// set_expire_time_if(hdr.tcp.flags == TCP_FLG_FIN,
//                    tcp_connection_close_time_profile_id);

extern void set_entry_expire_time_if(
    in bool condition,
    in ExpireTimeProfileId_t expire_time_profile_id);


// SelectByDirection is a simple pure function that behaves exactly as
// the P4_16 function definition given in comments below.  It is an
// extern function to ensure that the front/mid end of the p4c
// compiler leaves occurrences of it as is, visible to target-specific
// compiler back end code, so targets have all information needed to
// optimize it as they wish.

// One example of its use is in table key expressions, for tables
// where one wishes to swap IP source/destination addresses for
// packets processed in the different directions.

/*
T SelectByDirection<T>(
    in bool is_network_port,
    in T from_net_value,
    in T from_host_value)
{
    if (is_network_port) {
        return from_net_value;
    } else {
        return from_host_value;
    }
}
*/

// A typical call would look like this example:
//
// SelectByDirection(is_net_port(istd.input_port), hdr.ipv4.src_addr,
//                               hdr.ipv4.dst_addr)

@pure
extern T SelectByDirection<T>(
    in bool is_network_port,
    in T from_net_value,
    in T from_host_value);
