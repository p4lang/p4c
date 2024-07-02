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

// TBD: For add_entry calls to a table with property 'idle_timeout' or
// 'idle_timeout_with_auto_delete' equal to true, there should
// probably be an optional parameter at the end that specifies the new
// entry's initial expire_time_profile_id.

extern bool add_entry<T>(string action_name,
                         in T action_params);

extern FlowId_t allocate_flow_id();


// set_entry_expire_time() may only be called from within an action of
// a table with property 'idle_timeout' or
// 'idle_timeout_with_auto_delete' equal to true.

// Calling it causes the expiration time of the entry to be the one
// that the control plane has configured for the specified
// expire_time_profile_id.

extern void set_entry_expire_time(
    in ExpireTimeProfileId_t expire_time_profile_id);


// restart_expire_timer() may only be called from within an action of
// a table with property 'idle_timeout' or
// 'idle_timeout_with_auto_delete' equal to true.

// Calling it causes the dynamic expiration timer of the entry to be
// reset, so that the entry will remain active from the now until that
// time in the future.

// TBD: Do any targets support a table with one of the idle_timeout
// properties such that matching an entry _does not_ cause this side
// effect to occur?  If not, we could simply document it the way that
// I believe it currently behaves in all existing architectures, which
// is that every hit action implicitly causes the entry's expiration
// timer to be reset to its configured time interval in the future.

extern void restart_expire_timer();


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
    in PNA_Direction_t direction,
    in T n2h_value,
    in T h2n_value)
{
    if (direction == PNA_Direction_t.NET_TO_HOST) {
        return n2h_value;
    } else {
        return h2n_value;
    }
}
*/

@pure
extern T SelectByDirection<T>(
    in PNA_Direction_t direction,
    in T n2h_value,
    in T h2n_value);
