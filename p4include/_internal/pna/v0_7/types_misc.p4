// BEGIN:enum_PNA_IdleTimeout_t
/// Supported values for the pna_idle_timeout table property
enum PNA_IdleTimeout_t {
    NO_TIMEOUT,
    NOTIFY_CONTROL,
    AUTO_DELETE
};
// END:enum_PNA_IdleTimeout_t

// BEGIN:Match_kinds
match_kind {
    range,   /// Used to represent min..max intervals
    selector, /// Used for dynamic action selection via the ActionSelector extern
    optional /// Either an exact match, or a wildcard matching any value for the entire field
}
// END:Match_kinds
