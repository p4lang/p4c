// Ensure that errors cannot be disabled.
@command_line("--Wdisable=type-error")
parser p() {
    state accept {
        transition reject;
    }
}
