// Ensure that errors cannot be demoted to warnings.
@command_line("--Wwarn=type-error")
parser p() {
    state accept {
        transition reject;
    }
}
