// Ensure that errors cannot be demoted to info messages.
@command_line("--Winfo=type-error")
parser p() {
    state accept {
        transition reject;
    }
}
