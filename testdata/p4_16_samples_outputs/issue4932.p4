extern void log(string msg);
parser SimpleParser();
package SimpleArch(SimpleParser p);
parser ParserImpl() {
    state start {
        log("Log message" ++ " text");
        transition accept;
    }
}

SimpleArch(ParserImpl()) main;
