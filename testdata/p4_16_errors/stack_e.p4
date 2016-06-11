header h {}
struct s {}

control p()
{
    apply {
        h[5] stack;
        s[5] stack1; // non-header illegal in header stack
        
        // out of range indexes
        h b = stack[1231092310293];
        h c = stack[-2];
        h d = stack[6];
    }
}
