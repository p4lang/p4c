# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# -----------------------------------------------------------------------------
# stf_parser.py
#
# Parse an STF file.
# -----------------------------------------------------------------------------

import ply.yacc as yacc
from stf_lexer import STFLexer

# PARSER GRAMMAR --------------------------------------------------------------

# statement : ADD qualified_name priority match_list action [= ID]
#           | ADD qualified_name match_list action [= ID]
#           | REMOVE [ALL | entry]
#           | CHECK_COUNTER ID(id_or_index) [count_type logical_cond number]
#           | EXPECT port expect_data
#           | EXPECT port
#           | NO_PACKET
#           | PACKET port packet_data
#           | SETDEFAULT qualified_name action
#           | WAIT
#
# match_list : match
#            | match_list match
# match : qualified_name COLON number_or_lpm
# number_or_lpm: number SLASH number
#              | number
# number : INT_CONST_DEC
#        | INT_CONST_BIN
#        | INT_CONST_HEX
#        | TERN_CONST_HEX
#
# qualified_name : ID
#                | ID '[' INT_CONST_DEC ']'
#                | qualified_name DOT ID
#
# id_or_index: ID
#            | number
# count_type : BYTES
#            | PACKETS
# logical_cond : EQEQ
#              | NEQ
#              | LEQ
#              | LE
#              | GEQ
#              | GT
#
# action : qualified_name LPAREN args RPAREN
# args :
#      | arg_list
# arg_list: arg
#      | arg_list COMMA arg
# arg : ID COLON number
#
# port : DATA_DEC
#
# priority : INT_CONST_DEC
#
# expect_data : expect_datum
#             | expect_data expect_datum
# packet_data : packet_datum
#             | packet_data packet_datum
#
# expect_datum : packet_datum | DATA_TERN
# packet_datum : DATA_DEC | DATA_HEX

# PARSER ----------------------------------------------------------------------

class STFParser:
    def __init__(self):
        self.lexer = STFLexer()
        self.lexer.build()
        self.tokens = self.lexer.tokens
        self.parser = yacc.yacc(module = self)
        self.errors_cnt = 0

    def parse(self, data = None, filename=''):
        if data is None and filename == '':
            raise "Please specify either a filename or data"

        # if we specified only the filename, initialize the data
        if data is None:
            with file(filename) as f: data = f.read()

        self.lexer.filename = filename
        self.lexer.reset_lineno()
        stf_ast = self.parser.parse(input = data,
                                    lexer = self.lexer)
        self.errors_cnt += self.lexer.errors_cnt
        return stf_ast, self.errors_cnt

    def print_error(self, lineno, lexpos, msg):
        self.errors_cnt += 1
        print "parse error (%s%s:%s): %s" % (
            '%s:' % self.lexer.filename if self.lexer.filename else '',
            lineno,
            lexpos,
            msg)

    def get_filename(self):
        return self.lexer.filename

    def p_error(self, p):
        if p is None:
            self.print_error(self.lexer.get_lineno(), 0,
                             "Unexpected end-of-file.  Missing paren?")
        else:
            self.print_error(p.lineno, self.lexer.get_colno(),
                             "Syntax error while parsing at token '%s' (of type %s)." % (p.value, p.type))
            # Skip to the next statement.
            while True:
                tok = self.parser.token()
                if not tok or tok.type in self.lexer.keywords:
                    break
                self.parser.restart()

    def p_statements_none(self, p):
        'statements : '
        p[0] = []

    def p_statements_one(self, p):
        'statements : statement'
        p[0] = [p[1]]

    def p_statements_many(self, p):
        'statements : statements statement'
        p[0] = p[1] + [p[2]]

    def p_statement_add(self, p):
        'statement : ADD qualified_name match_list action'
        p[0] = (p[1].lower(), p[2], None, p[3], p[4], None)

    def p_statement_add_with_priority(self, p):
        'statement : ADD qualified_name priority match_list action'
        p[0] = (p[1].lower(), p[2], p[3], p[4], p[5], None)

    def p_statement_add_with_id(self, p):
        'statement : ADD qualified_name match_list action EQUAL ID'
        p[0] = (p[1].lower(), p[2], None, p[3], p[4], p[6])

    def p_statement_add_with_priority_and_id(self, p):
        'statement : ADD qualified_name priority match_list action EQUAL ID'
        p[0] = (p[1].lower(), p[2], p[3], p[4], p[5], p[7])

    # remove all entries.
    # \TODO: removal of individual entries
    def p_statement_remove(self, p):
        'statement : REMOVE ALL'
        p[0] = (p[1].lower(), p[2])

    def p_statement_check_counter(self, p):
        'statement : CHECK_COUNTER ID LPAREN id_or_index RPAREN'
        p[0] = (p[1].lower(), p[2], p[4], (None, '==', 0))

    def p_statement_check_counter_with_check(self, p):
        'statement : CHECK_COUNTER ID LPAREN id_or_index RPAREN count_type logical_cond number'
        p[0] = (p[1].lower(), p[2], p[4], (p[6], p[7], p[8]))

    def p_statement_expect(self, p):
        'statement : EXPECT port'
        p[0] = (p[1].lower(), p[2], None)

    def p_statement_expect_data(self, p):
        'statement : EXPECT port expect_data'
        p[0] = (p[1].lower(), p[2], ''.join(p[3]))

    def p_statement_no_packet(self, p):
        'statement : NO_PACKET'
        p[0] = ('expect', None, None)

    def p_statement_packet(self, p):
        'statement : PACKET port packet_data'
        p[0] = (p[1].lower(), p[2], ''.join(p[3]))

    def p_statement_setdefault(self, p):
        'statement : SETDEFAULT qualified_name action'
        p[0] = (p[1].lower(), p[2], p[3])

    def p_statement_wait(self, p):
        'statement : WAIT'
        p[0] = (p[1].lower(),)

    def p_id_or_index(self, p):
        '''id_or_index : ID
                       | number
        '''
        p[0] = p[1]

    def p_count_type(self, p):
        '''count_type : BYTES
                      | PACKETS
        '''
        p[0] = p[1]

    def p_logical_cond(self, p):
        '''logical_cond : EQEQ
                        | NEQ
                        | LEQ
                        | LE
                        | GEQ
                        | GT
        '''
        p[0] = p[1]

    def p_match_list_one(self, p):
        'match_list : match'
        p[0] = [p[1]]

    def p_match_list_many(self, p):
        'match_list : match_list match'
        p[0] = p[1] + [p[2]]

    def p_match(self, p):
        'match : qualified_name COLON number_or_lpm'
        p[0] = (p[1], p[3])

    def p_qualified_name_one(self, p):
        'qualified_name : ID'
        p[0] = p[1]

    # \TODO: how do we handle stacks in either PD or PI??
    # for now we escape the brackets ...
    def p_qualified_name_array(self, p):
        'qualified_name : ID LBRACKET INT_CONST_DEC RBRACKET'
        p[0] = p[1] + '[' + p[3] + ']'

    def p_qualified_name_many(self, p):
        'qualified_name : qualified_name DOT ID'
        p[0] = p[1] + '.' + p[3]

    def p_action(self, p):
        'action : qualified_name LPAREN args RPAREN'
        p[0] = (p[1], p[3])

    def p_args_none(self, p):
        'args :'
        p[0] = []

    def p_args_many(self, p):
        'args : arg_list'
        p[0] = p[1]

    def p_arg_list_one(self, p):
        'arg_list : arg'
        p[0] = [p[1]]

    def p_arg_list_many(self, p):
        'arg_list : arg_list COMMA arg'
        p[0] = p[1] + [p[3]]

    def p_arg(self, p):
        'arg : ID COLON number'
        p[0] = (p[1], p[3])

    def p_priority(self, p):
        'priority : INT_CONST_DEC'
        p[0] = p[1]

    def p_lpm(self, p):
        'number_or_lpm : number SLASH number'
        p[0] = (p[1], p[3])

    def p_number_or_lpm(self, p):
        'number_or_lpm : number'
        p[0] = p[1]

    def p_number(self, p):
        '''number : INT_CONST_DEC
                  | INT_CONST_BIN
                  | TERN_CONST_HEX
                  | INT_CONST_HEX'''
        p[0] = p[1]

    def p_packet_data_port(self, p):
        'port : DATA_DEC'
        p[0] = p[1]

    def p_packet_data_one(self, p):
        'packet_data : packet_datum'
        p[0] = [p[1]]

    def p_packet_data_many(self, p):
        'packet_data : packet_data packet_datum'
        p[0] = p[1] + [p[2]]

    def p_packet_datum(self, p):
        '''packet_datum : DATA_HEX
                        | DATA_DEC'''
        p[0] = p[1]

    def p_expect_data_one(self, p):
        'expect_data : expect_datum'
        p[0] = [p[1]]

    def p_expect_data_many(self, p):
        'expect_data : expect_data expect_datum'
        p[0] = p[1] + [p[2]]

    def p_expect_dataum(self, p):
        '''expect_datum : packet_datum
                        | DATA_TERN'''
        p[0] = p[1]


# TESTING ---------------------------------------------------------------------
#

if __name__ == '__main__':
    data = '''
    ADD test2 100 data.f2:0x04040404 c4_6(val4:0x4040, val5:0x5050, val6:0x6060, port:2)
    add tab1 dstAddr:0xa1a2a3a4a5a6 act(port:2) = A
    # multiple match fields and ids with $
    add test1 0 data.$valid:0b1 data.f1:0x****0101 setb1(val:0x7f, port:2)

    setdefault ethertype_match l2_packet()

    expect 1 *1010101 03030303 0101 0202 0303 0404 0505 0606
    packet 0 01010101 03030303 9999 8888 7777 6666 5555 4444

    # expect with out data
    expect 5

    check_counter cntDum(10) packets == 2
    check_counter cntDum($A)
    wait
    '''

    parser = STFParser()
    stf, errs = parser.parse(data)
    if errs == 0:
        print '\n'.join(map(str, stf))
