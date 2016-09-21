/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 17 "../frontends/p4/p4-parse.ypp" /* yacc.c:339  */
 /* -*-C++-*- */
// some of these includes are needed by lex-generated file
// which is #included below (p4-lex.c)

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <iostream>
#include <map>
#include <cerrno>

#include "lib/null.h"
#include "lib/error.h"
#include "lib/cstring.h"
#include "lib/gmputil.h"
#include "frontends/p4-14/p4-14-parse.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"
#include "frontends/p4/symbol_table.h"
#include "frontends/common/constantParsing.h"

#undef PACKAGE  // autoconf wants to define this macro that we want to use as a token

#ifndef YYDEBUG
#define YYDEBUG 1
#endif

#define YYLTYPE Util::SourceInfo
#define YYLLOC_DEFAULT(Cur, Rhs, N)                                     \
    ((Cur) = (N) ? YYRHSLOC(Rhs, 1) + YYRHSLOC(Rhs, N)                  \
                 : Util::SourceInfo(YYRHSLOC(Rhs, 0).getEnd()))

// Program IR built here
static IR::IndexedVector<IR::Node> *declarations;
static void yyerror(const char *fmt, ...);
static void checkShift(Util::SourceInfo f, Util::SourceInfo r);

static Util::ProgramStructure structure;

namespace {  // anonymous namespace
static int yylex();


#line 113 "../frontends/p4/p4-parse.cpp" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    LE = 258,
    GE = 259,
    SHL = 260,
    AND = 261,
    OR = 262,
    NE = 263,
    EQ = 264,
    TRUE = 265,
    FALSE = 266,
    THIS = 267,
    ABSTRACT = 268,
    ACTION = 269,
    ACTIONS = 270,
    APPLY = 271,
    BOOL = 272,
    BIT = 273,
    CONST = 274,
    CONTROL = 275,
    DATA = 276,
    DEFAULT = 277,
    DONTCARE = 278,
    ELSE = 279,
    ENUM = 280,
    T_ERROR = 281,
    EXIT = 282,
    EXTERN = 283,
    HEADER = 284,
    HEADER_UNION = 285,
    IF = 286,
    IN = 287,
    INOUT = 288,
    INT = 289,
    KEY = 290,
    MASK = 291,
    SELECT = 292,
    MATCH_KIND = 293,
    OUT = 294,
    PACKAGE = 295,
    PARSER = 296,
    RANGE = 297,
    RETURN = 298,
    STATE = 299,
    STRUCT = 300,
    SWITCH = 301,
    TABLE = 302,
    TRANSITION = 303,
    TYPEDEF = 304,
    VARBIT = 305,
    VOID = 306,
    IDENTIFIER = 307,
    TYPE = 308,
    NAMESPACE = 309,
    STRING_LITERAL = 310,
    INTEGER = 311,
    PP = 312,
    PREFIX = 313,
    THEN = 314
  };
#endif
/* Tokens.  */
#define LE 258
#define GE 259
#define SHL 260
#define AND 261
#define OR 262
#define NE 263
#define EQ 264
#define TRUE 265
#define FALSE 266
#define THIS 267
#define ABSTRACT 268
#define ACTION 269
#define ACTIONS 270
#define APPLY 271
#define BOOL 272
#define BIT 273
#define CONST 274
#define CONTROL 275
#define DATA 276
#define DEFAULT 277
#define DONTCARE 278
#define ELSE 279
#define ENUM 280
#define T_ERROR 281
#define EXIT 282
#define EXTERN 283
#define HEADER 284
#define HEADER_UNION 285
#define IF 286
#define IN 287
#define INOUT 288
#define INT 289
#define KEY 290
#define MASK 291
#define SELECT 292
#define MATCH_KIND 293
#define OUT 294
#define PACKAGE 295
#define PARSER 296
#define RANGE 297
#define RETURN 298
#define STATE 299
#define STRUCT 300
#define SWITCH 301
#define TABLE 302
#define TRANSITION 303
#define TYPEDEF 304
#define VARBIT 305
#define VOID 306
#define IDENTIFIER 307
#define TYPE 308
#define NAMESPACE 309
#define STRING_LITERAL 310
#define INTEGER 311
#define PP 312
#define PREFIX 313
#define THEN 314

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 64 "../frontends/p4/p4-parse.ypp" /* yacc.c:355  */

    YYSTYPE() {}
    IR::IndexedVector<IR::Type_Var>       *nm;
    IR::IndexedVector<IR::Parameter>      *params;
    IR::IndexedVector<IR::StatOrDecl>     *statOrDecls;
    IR::IndexedVector<IR::Declaration>    *decls;
    IR::IndexedVector<IR::ParserState>    *parserStates;
    IR::IndexedVector<IR::StructField>    *structFields;
    IR::IndexedVector<IR::TableProperty>  *properties;
    IR::IndexedVector<IR::Declaration_ID> *ids;
    IR::IndexedVector<IR::ActionListElement> *actions;
    IR::Vector<IR::Annotation>  *annoVec;
    IR::Vector<IR::Expression>  *exprs;
    IR::Vector<IR::SelectCase>  *selectCases;
    IR::Vector<IR::SwitchCase>  *switchCases;
    IR::Vector<IR::Type>        *types;
    IR::Vector<IR::Method>      *methods;
    IR::Vector<IR::KeyElement>  *keyElements;
    IR::ID*                     id;
    cstring                     str;
    bool                        b;
    const IR::Type              *TypePtr;
    IR::Node                    *Node;
    IR::Annotations             *annos;
    IR::StringLiteral           *strlit;
    IR::ActionListElement       *ActionListElement;
    IR::Annotation              *Annotation;
    IR::BlockStatement          *BlockStatement;
    IR::Constant                *Constant;
    IR::Declaration             *Declaration;
    IR::Direction               dir;
    IR::Expression              *Expression;
    IR::IDeclaration            *decl;
    IR::KeyElement              *KeyElement;
    IR::Method                  *Method;
    IR::Function                *Function;
    IR::Parameter               *Parameter;
    IR::ParserState             *ParserState;
    IR::PathPrefix              *PathPrefix;
    IR::SelectCase              *SelectCase;
    IR::Statement               *Statement;
    IR::StatOrDecl              *StatOrDecl;
    IR::StructField             *StructField;
    IR::SwitchCase              *SwitchCase;
    IR::TableProperty           *TableProperty;
    IR::Type_Control            *Type_Control;
    IR::Type_Declaration        *Type_Declaration;
    IR::Type_Name               *Type_Name;
    IR::TypeParameters          *TypeParameters;
    IR::Type_Parser             *Type_Parser;

#line 320 "../frontends/p4/p4-parse.cpp" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;
int yyparse (void);



/* Copy the second part of user declarations.  */
#line 202 "../frontends/p4/p4-parse.ypp" /* yacc.c:358  */

static void
symbol_print(FILE* file, int type, YYSTYPE value)
{
    switch (type)
    {
    case IDENTIFIER:
    case DONTCARE:
    case TYPE:
    case NAMESPACE:
        fprintf(file, "%s", value.str.c_str());
    default:
        break;
    }
}

#define YYPRINT(file, type, value)   symbol_print(file, type, value)

#line 369 "../frontends/p4/p4-parse.cpp" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2044

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  85
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  119
/* YYNRULES -- Number of rules.  */
#define YYNRULES  277
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  569

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   314

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    83,     2,     2,     2,    70,    62,     2,
      73,    79,    68,    66,    57,    67,    75,    69,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    59,    77,
      63,    80,    64,    58,    78,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    74,     2,    72,    61,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    81,    60,    82,    84,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    65,    71,    76
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   233,   233,   235,   236,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   252,   253,   254,   255,   256,   260,
     261,   265,   266,   270,   271,   275,   276,   280,   282,   286,
     290,   291,   292,   293,   297,   298,   297,   304,   307,   310,
     313,   320,   324,   325,   329,   330,   334,   335,   339,   339,
     341,   341,   346,   347,   352,   359,   368,   369,   373,   374,
     375,   379,   380,   379,   387,   389,   393,   398,   399,   403,
     404,   408,   409,   413,   414,   418,   424,   425,   429,   434,
     435,   436,   441,   446,   447,   451,   452,   453,   459,   467,
     468,   467,   475,   476,   480,   481,   482,   483,   484,   488,
     494,   495,   494,   499,   503,   504,   508,   510,   508,   517,
     518,   519,   521,   519,   530,   531,   532,   533,   537,   538,
     542,   546,   550,   556,   557,   558,   559,   560,   561,   565,
     566,   567,   572,   573,   577,   579,   583,   584,   588,   589,
     593,   594,   595,   596,   597,   601,   602,   603,   604,   608,
     608,   613,   613,   618,   618,   623,   624,   628,   629,   633,
     633,   638,   642,   646,   648,   652,   654,   656,   658,   666,
     669,   673,   677,   681,   685,   686,   690,   692,   697,   698,
     699,   700,   701,   702,   703,   707,   711,   712,   716,   721,
     722,   726,   727,   731,   732,   736,   737,   738,   739,   745,
     749,   756,   758,   762,   767,   772,   776,   783,   784,   788,
     794,   796,   800,   802,   812,   822,   825,   831,   836,   837,
     841,   847,   851,   852,   856,   858,   862,   866,   868,   872,
     873,   877,   878,   879,   880,   881,   882,   886,   887,   888,
     889,   890,   891,   892,   893,   894,   895,   896,   897,   898,
     899,   900,   901,   904,   905,   906,   907,   908,   909,   910,
     911,   912,   913,   914,   915,   916,   917,   918,   919,   920,
     921,   922,   923,   924,   925,   929,   931,   933
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LE", "GE", "SHL", "AND", "OR", "NE",
  "EQ", "TRUE", "FALSE", "THIS", "ABSTRACT", "ACTION", "ACTIONS", "APPLY",
  "BOOL", "BIT", "CONST", "CONTROL", "DATA", "DEFAULT", "DONTCARE", "ELSE",
  "ENUM", "T_ERROR", "EXIT", "EXTERN", "HEADER", "HEADER_UNION", "IF",
  "IN", "INOUT", "INT", "KEY", "MASK", "SELECT", "MATCH_KIND", "OUT",
  "PACKAGE", "PARSER", "RANGE", "RETURN", "STATE", "STRUCT", "SWITCH",
  "TABLE", "TRANSITION", "TYPEDEF", "VARBIT", "VOID", "IDENTIFIER", "TYPE",
  "NAMESPACE", "STRING_LITERAL", "INTEGER", "','", "'?'", "':'", "'|'",
  "'^'", "'&'", "'<'", "'>'", "PP", "'+'", "'-'", "'*'", "'/'", "'%'",
  "PREFIX", "']'", "'('", "'['", "'.'", "THEN", "';'", "'@'", "')'", "'='",
  "'{'", "'}'", "'!'", "'~'", "$accept", "input", "declaration", "name",
  "optAnnotations", "annotations", "annotation", "parameterList",
  "nonEmptyParameterList", "parameter", "direction",
  "packageTypeDeclaration", "$@1", "$@2", "instantiation",
  "objInitializer", "objDeclarations", "objDeclaration",
  "optCompileParameters", "pathPrefix", "$@3", "$@4", "relativePathPrefix",
  "nonEmptyRelativePathPrefix", "parserDeclaration", "parserLocalElements",
  "parserLocalElement", "parserTypeDeclaration", "$@5", "$@6",
  "parserStates", "parserState", "parserStatements", "parserStatement",
  "transitionStatement", "stateExpression", "selectExpression",
  "selectCaseList", "selectCase", "keysetExpression",
  "tupleKeysetExpression", "simpleExpressionList",
  "simpleKeysetExpression", "controlDeclaration", "controlTypeDeclaration",
  "$@7", "$@8", "controlLocalDeclarations", "controlLocalDeclaration",
  "controlBody", "externDeclaration", "$@9", "$@10", "methodPrototypes",
  "functionPrototype", "$@11", "$@12", "methodPrototype", "$@13", "$@14",
  "typeRef", "typeName", "headerStackType", "specializedType", "baseType",
  "typeOrVoid", "optTypeParameters", "typeParameterList", "typeArg",
  "typeArgumentList", "typeDeclaration", "derivedTypeDeclaration",
  "headerTypeDeclaration", "$@15", "structTypeDeclaration", "$@16",
  "headerUnionDeclaration", "$@17", "structFieldList", "structField",
  "enumDeclaration", "$@18", "errorDeclaration", "matchKindDeclaration",
  "identifierList", "typedefDeclaration",
  "assignmentOrMethodCallStatement", "emptyStatement", "exitStatement",
  "returnStatement", "conditionalStatement", "statement", "blockStatement",
  "statOrDeclList", "switchStatement", "switchCases", "switchCase",
  "switchLabel", "statementOrDeclaration", "tableDeclaration",
  "tablePropertyList", "tableProperty", "keyElementList", "keyElement",
  "actionList", "actionRef", "actionDeclaration", "variableDeclaration",
  "constantDeclaration", "optInitializer", "initializer",
  "functionDeclaration", "argumentList", "nonEmptyArgList", "argument",
  "expressionList", "member", "lvalue", "expression", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,    44,    63,    58,
     124,    94,    38,    60,    62,   312,    43,    45,    42,    47,
      37,   313,    93,    40,    91,    46,   314,    59,    64,    41,
      61,   123,   125,    33,   126
};
# endif

#define YYPACT_NINF -471

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-471)))

#define YYTABLE_NINF -223

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -471,   102,  -471,  -471,    25,   -53,  1964,    46,     5,   480,
      63,    64,  -471,  -471,   392,  -471,  1076,   417,  -471,    91,
    -471,    88,  -471,  -471,    52,  -471,   165,  -471,   100,   107,
    -471,  -471,  -471,  -471,  -471,  -471,  -471,  -471,  -471,  -471,
    -471,   109,  -471,  -471,   106,   392,  -471,  -471,  -471,  -471,
    -471,  -471,  -471,   -32,   112,  -471,   392,   136,   392,   204,
     116,   392,   392,   148,   197,  -471,   134,   392,   237,   392,
     392,   392,   392,   392,   392,   392,   480,  -471,   154,  -471,
     173,   185,  -471,   -37,  -471,   160,  -471,   176,   265,  1314,
    -471,   182,  -471,     2,   196,  -471,   196,   214,    16,  -471,
    -471,   215,  -471,  -471,  -471,   101,   206,  1314,   232,   392,
    -471,  -471,  -471,  -471,  -471,  -471,  -471,   392,   392,   265,
     197,   231,   128,   228,   257,  -471,  -471,  -471,  -471,  -471,
    -471,  -471,  -471,  1314,  1314,  1314,  1314,  1314,  1314,  -471,
     493,   243,   142,   240,   271,  -471,  1680,  1486,  -471,   392,
    -471,   392,  -471,  -471,  -471,  -471,  -471,   197,  -471,   260,
     677,   -37,   256,   196,   258,   261,   262,   196,   196,   269,
    -471,  -471,   277,   146,   283,  -471,  -471,  -471,   237,  -471,
     116,   480,   235,   222,   222,    65,   697,    49,  1680,   222,
     222,  -471,   265,   773,   392,  1314,  1314,  1314,  1314,  1314,
    1314,  1314,  1314,  1314,  1314,  1314,  1314,  1005,  1058,  1314,
    1314,  1314,  1314,  1314,  1314,   265,  1314,   773,  -471,  -471,
    -471,   155,   276,   268,  -471,  -471,  -471,   280,  1314,  -471,
     392,  -471,  -471,  -471,  -471,  -471,   392,  -471,   392,  -471,
      89,   480,  -471,  -471,   166,  -471,   249,  -471,  -471,   116,
      85,  -471,  -471,  -471,  -471,  -471,  -471,  1314,  -471,  1314,
    -471,   282,  -471,  -471,  -471,   -20,  -471,    48,    48,   745,
    1769,  1723,   978,   978,  1559,  1796,  1810,   416,   243,   161,
      48,  1314,    48,   868,   538,   538,   222,   222,   222,   284,
    1443,  -471,   392,  -471,  -471,   -37,   285,   292,  1680,   300,
      67,   184,   192,   302,   305,   216,    30,  -471,   392,   349,
    -471,   338,  -471,   303,   306,   320,  -471,   392,   222,  1680,
    -471,  -471,   322,  1314,   333,    48,  -471,  1314,  -471,  -471,
    1140,   332,  -471,  -471,   -37,  -471,  -471,   237,  -471,  -471,
     -37,   -37,  -471,  -471,   322,   334,   303,  1314,   336,  -471,
    -471,   -30,  -471,   337,  1696,   265,  1577,  1964,   -27,  -471,
    -471,   339,  -471,  -471,  1874,   344,   859,   347,   350,   340,
    -471,   361,  -471,  -471,  1919,   -37,    12,  1240,  -471,   360,
    -471,   370,  1969,  -471,  -471,  -471,  -471,   371,   376,  1133,
     377,  -471,  -471,  -471,    11,  -471,   493,  -471,  -471,  -471,
    -471,  -471,  -471,  -471,  -471,  -471,  -471,  -471,    96,  -471,
     381,   382,  -471,  -471,  -471,   643,  -471,  -471,   375,   380,
     384,    18,     0,  -471,  -471,   378,  -471,  -471,   116,   100,
    -471,  -471,  -471,   388,  -471,  1314,  -471,  1347,  1314,  -471,
     197,   265,  1314,   773,  1314,  -471,  -471,   318,   378,  -471,
     386,   392,  -471,  -471,   391,   393,   394,   410,   396,  -471,
    -471,  -471,   -37,   776,  -471,   798,   164,   409,  1461,  -471,
    1368,   400,   422,  -471,  -471,   392,  -471,    12,   116,  -471,
     413,  1314,   424,   875,   423,   432,   433,  1314,  -471,  -471,
    1314,  -471,    23,   392,   226,   434,  1186,  1314,   438,   440,
     392,   496,  -471,   265,  -471,  1607,   -35,  -471,   448,  -471,
     452,  -471,  -471,  -471,  1650,   454,  -471,  -471,   875,   131,
     453,  -471,   455,   265,  -471,   392,  -471,  -471,  -471,  -471,
    -471,  -471,  -471,   479,   462,  1261,   461,   116,    61,  -471,
    -471,  1314,   952,  -471,   482,  -471,  -471,  1534,  -471,   466,
    -471,   487,   568,  -471,  -471,   392,  1314,  1314,  -471,  1314,
     470,  1680,  1680,   -17,  -471,  -471,  1314,  -471,  -471
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,    19,     1,   123,   125,   124,    50,     0,     0,    19,
       0,   118,    48,     4,     0,     3,     0,    20,    21,     0,
      11,     0,    52,     8,    46,    10,    46,     6,     0,   115,
     117,   116,   114,     9,   140,   145,   147,   146,   148,    12,
      13,     0,     7,     5,     0,     0,    17,    15,   124,    16,
      18,   130,    14,   131,     0,   129,     0,     0,     0,     0,
      20,     0,     0,     0,    50,    52,    23,     0,    50,     0,
       0,     0,     0,     0,     0,     0,    19,    22,     0,   144,
     119,     0,    51,    19,   142,     0,   143,     0,    50,    50,
     141,     0,   163,     0,   132,   103,   132,     0,     0,   166,
     168,     0,   137,   136,   138,     0,    49,    50,     0,     0,
      89,   159,   149,   153,    34,    61,   151,     0,     0,    50,
      50,     0,    33,     0,    26,    27,    56,    92,   239,   240,
     241,   238,   237,    50,    50,    50,    50,    50,    50,   242,
       0,     0,   115,     0,   223,   224,   226,     0,   126,     0,
     161,     0,   101,   106,   127,   162,   128,    50,   122,     0,
       0,    19,     0,   132,     0,     0,     0,   132,   132,     0,
     165,   167,     0,     0,    54,    30,    32,    31,    50,    47,
      19,    19,    19,   251,   250,     0,     0,     0,   227,   248,
     249,   243,    50,     0,     0,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    50,     0,   120,   164,
     134,     0,     0,     0,   139,    53,    24,     0,    50,    90,
       0,   155,   155,    35,    62,   155,     0,   121,     0,    28,
       0,    20,    59,    57,    19,    64,     0,    60,    58,    19,
       0,    97,    93,    96,    95,    98,    94,    50,   247,    50,
     246,     0,   230,   229,   252,     0,   225,   261,   262,   259,
     271,   272,   265,   266,     0,   269,   268,   267,   136,     0,
     263,    50,   264,   270,   257,   258,   254,   255,   256,     0,
       0,   253,     0,   133,   104,    19,     0,     0,   220,     0,
       0,    19,    19,     0,     0,    19,     0,    29,     0,     0,
      55,     0,    65,   218,     0,     0,    99,     0,   277,   228,
     276,    38,     0,    50,     0,   260,   275,    50,   244,   135,
      50,     0,   186,   217,    19,   160,   150,    50,   156,   154,
      19,    19,   152,    37,     0,     0,   218,    50,     0,   186,
      88,     0,    42,     0,   273,    50,     0,    50,   118,   102,
     131,     0,   105,   107,    19,     0,     0,     0,     0,     0,
      67,     0,   219,   216,    19,    19,    19,    50,    40,     0,
     245,     0,    50,   111,   109,   108,   232,     0,     0,    50,
       0,   172,   214,   231,     0,   198,     0,   178,   180,   183,
     182,   179,   197,   181,   184,   187,   195,   196,     0,    91,
       0,     0,    36,    63,    39,    50,   215,   185,     0,     0,
       0,     0,    19,   201,    41,    50,    45,    43,    19,   129,
      44,   274,   110,     0,   173,    50,   174,     0,    50,   233,
      50,    50,    50,     0,    50,   158,   157,     0,    50,    68,
       0,     0,    69,    70,     0,     0,     0,     0,     0,   200,
     202,   221,    19,     0,   175,     0,     0,     0,     0,   234,
       0,     0,     0,    72,    74,     0,    66,    19,    19,   207,
       0,    50,     0,    19,     0,     0,     0,    50,   235,   171,
      50,    73,    19,     0,    19,     0,    50,    50,     0,     0,
       0,   176,   189,    50,   169,     0,     0,   199,   212,   204,
       0,   210,   203,   208,     0,     0,   206,   112,    19,     0,
       0,   236,     0,    50,   211,     0,   205,   113,   177,   194,
     188,   193,   190,     0,     0,    50,     0,    19,   192,   170,
      79,    50,    50,    76,     0,    80,    81,    85,   213,     0,
     191,     0,    85,    75,    77,     0,    50,    50,   209,    50,
       0,    86,    87,     0,    83,    78,    50,    82,    84
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -471,  -471,  -471,    -6,    24,     3,   -15,  -132,  -471,   368,
    -471,  -471,  -471,  -471,     9,   207,  -471,  -471,   524,    17,
    -471,  -471,   488,  -471,  -471,  -471,  -471,  -471,  -471,  -471,
    -471,   310,  -471,  -471,  -471,  -471,  -471,  -471,    21,  -471,
    -471,  -471,  -470,  -471,  -471,  -471,  -471,  -471,  -471,  -471,
    -471,  -471,  -471,  -471,  -282,  -471,  -471,  -471,  -471,  -471,
      15,   551,  -471,  -471,  -471,  -471,   -91,  -471,   402,  -113,
    -471,     4,  -471,  -471,  -471,  -471,  -471,  -471,   -78,  -471,
    -471,  -471,  -471,  -471,   -52,  -471,   149,  -471,  -471,  -471,
    -471,  -464,  -246,   217,  -471,  -471,  -471,  -471,  -471,  -471,
      90,  -405,  -471,  -471,  -471,    71,   387,  -170,    19,   224,
    -333,  -471,  -118,  -471,   383,    92,  -208,  -471,   389
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    15,   139,   122,    60,    18,   123,   124,   125,
     178,    19,   167,   303,   395,   353,   377,   427,    85,   140,
      65,    22,    81,    82,    23,   181,   243,    24,   168,   304,
     244,   245,   415,   449,   450,   473,   474,   542,   543,   544,
     545,   563,   546,    25,    26,   163,   299,   182,   252,   315,
      27,    94,   222,   330,    54,   223,   385,   362,   433,   527,
     141,   142,    30,    31,    32,    56,   152,   221,   104,   105,
      33,    34,    35,   165,    36,   169,    37,   166,   301,   338,
      38,   164,    39,    40,    93,    41,   397,   398,   399,   400,
     401,   402,   403,   364,   404,   519,   532,   533,   405,   253,
     422,   423,   496,   513,   494,   495,    42,   406,   407,   348,
     297,   430,   143,   144,   145,   187,   264,   408,   146
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      53,   172,    77,   316,    17,   153,    98,   173,    66,   291,
      20,   247,   255,    62,   372,   419,    28,   460,    21,   501,
      43,    55,   259,    21,    61,    16,    21,   419,    45,   227,
      68,  -100,    78,    59,    21,   420,   382,   457,   419,    92,
     566,    14,   -25,   375,   522,    77,  -132,   420,   361,  -100,
      96,   376,    92,   198,   528,    99,   100,   321,   420,   149,
     322,   108,   567,   110,   111,   112,   113,   114,   115,   116,
     458,   551,   229,   149,   261,   381,   233,   234,    14,   103,
     118,    21,   459,   109,   150,    21,    58,   460,    44,   564,
      14,   117,   349,    21,   279,   428,   568,   289,   155,    67,
      59,    14,     2,   162,    68,   507,   259,   343,    68,    57,
     344,   170,   171,   209,   210,   211,   212,   213,   214,     3,
       4,   215,   216,   217,   149,    83,    63,    64,     5,    84,
       6,   260,   317,   308,   191,   103,     7,    21,   192,    14,
       8,    80,   -19,   219,   257,   220,    46,    47,   498,   335,
     185,     9,    10,   529,   302,    11,   -50,   305,   157,   440,
     175,   176,    91,   331,   515,   158,    49,   177,    79,   441,
     442,   443,   103,    88,    21,    50,   444,    12,   300,    13,
      14,    89,   461,    52,   241,   241,    90,   263,   265,    95,
     242,   251,    97,   238,    14,    21,   246,   246,    21,    21,
     248,   256,   365,   157,   101,   240,   250,   107,   367,   368,
     237,   263,   292,   530,     3,     4,    89,   193,   157,   293,
     102,   157,   278,    48,    92,   324,    77,   119,   485,    70,
     306,     7,   307,    71,    72,   469,   120,   379,    83,   121,
     313,   126,    86,   418,    14,   453,   148,    10,   310,    75,
      11,   249,     3,     4,     3,     4,   309,   127,    21,   151,
     159,    48,    14,    48,    46,    47,   336,   383,   311,     7,
      14,     7,    12,   314,   339,   128,   129,   130,   154,   156,
      46,    47,     3,     4,    49,    10,   329,    10,    11,   -50,
      11,    48,   550,    50,    14,   215,   216,   217,   342,     7,
      49,    52,   345,   346,    14,   161,   174,   179,   509,    50,
      12,   351,    12,    14,   180,    10,   192,    52,    11,   194,
     131,   132,    88,   467,   360,   337,   337,   466,   195,   337,
     482,   133,   134,    46,    47,   225,   228,   -53,   135,   230,
      12,   295,   231,   232,  -222,    55,   136,    21,   137,   138,
     235,   360,   366,    49,    21,   471,   236,   294,   393,   296,
     411,   320,    50,   326,    46,    47,   332,   241,   393,   333,
      52,   360,    55,   334,    21,   340,   220,   241,   341,   246,
     425,   396,   308,   347,    49,   520,   426,   349,   394,   246,
     439,   396,   429,    50,    21,     3,     4,   103,   394,    21,
     421,    52,   350,   352,    48,   536,   355,    46,    47,   393,
      77,   363,     7,   373,   378,   370,   384,   414,   448,   196,
     197,   198,   119,   409,   201,   202,   412,    49,    10,   413,
     451,    11,   396,    77,     3,     4,    50,   263,   416,   431,
      78,   472,    21,    48,    52,   313,   421,   432,   434,   435,
     438,     7,   314,    12,   454,   103,    14,    21,   445,   446,
     455,   462,   480,   475,   456,    21,    76,    10,   476,   346,
      11,   -50,   477,   490,   478,   479,   481,   393,   147,   207,
     208,   209,   210,   211,   212,   213,   214,   508,   486,   215,
     216,   217,    12,   497,   439,    14,   160,     3,     4,   491,
     500,   421,   493,   499,   502,   503,    48,   314,    46,    47,
     504,   511,   393,   531,     7,   516,   421,   517,   493,   537,
     518,   523,   183,   184,   186,   188,   189,   190,    49,   524,
      10,   526,   534,    11,   -50,   500,   535,    50,   538,   539,
     548,   555,   314,   558,   559,    52,    80,   565,   239,   560,
      87,   369,    29,   106,   312,    12,   185,    29,    14,   224,
      29,   549,   314,   554,   452,   510,   374,   492,    29,   254,
     371,   196,   197,   198,   199,   200,   201,   202,   266,     0,
       0,     0,   506,     0,     0,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   280,   282,   283,   284,
     285,   286,   287,   288,   556,   290,   212,   213,   214,     0,
     557,   215,   216,   217,     0,    29,     0,   298,     0,    29,
       0,     0,     0,     0,     0,     0,   203,    29,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,     0,
       0,   215,   216,   217,     0,     0,   318,   258,   319,     0,
       0,     0,     0,     0,     0,   386,     0,     0,    46,    47,
       3,     4,     0,     0,     0,     0,     0,     0,     0,    48,
     325,    29,     0,     0,     0,     0,     0,     7,    49,     0,
     196,   197,   198,   199,   200,   201,   202,    50,     0,     0,
       0,   447,     0,    10,     0,    52,    11,     0,     0,     0,
     196,   197,   198,   199,   200,   201,   202,     0,    29,     0,
       0,     0,   354,     0,     0,     0,   356,     0,    12,     0,
       0,    14,     0,     0,     0,   -71,     0,     0,     0,    29,
       0,     0,    29,    29,     0,   203,   298,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,     0,     0,
     215,   216,   217,     0,     0,   203,   226,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,     0,     0,
     215,   216,   217,     0,     0,     0,   258,     0,   437,   196,
     197,   198,   199,   200,   201,   202,     0,     0,    46,    47,
       0,     0,    29,     0,     0,     0,     0,     0,     0,     0,
       0,   196,   197,   198,   199,   200,   201,   202,    49,     0,
     209,   210,   211,   212,   213,   214,     0,    50,   215,   216,
     217,     0,     0,     0,   463,    52,   262,   465,     0,     0,
       0,   468,     0,   470,   203,     0,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,     0,     0,   215,
     216,   217,     0,     0,     0,   483,   203,     0,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,     0,
     298,   215,   216,   217,    46,    47,   505,   484,     0,   188,
       0,    29,     0,     0,     0,   514,   298,   386,    29,     0,
      46,    47,     0,     0,    49,     0,     0,     0,     0,     0,
       0,     0,   387,    50,     0,     0,   388,     0,    29,     0,
      49,    52,   410,     0,     0,    29,     0,     0,   389,    50,
       0,   390,     0,     0,   547,    29,     0,    52,    29,   -50,
     552,   547,     0,    29,   210,   211,   212,   213,   214,     0,
       0,   215,   216,   217,     0,   561,   562,     0,   547,     0,
      12,     0,   391,    14,     0,   547,     0,     0,     0,     0,
       0,     0,   128,   129,   130,     0,    29,    46,    47,     3,
       4,     0,     0,     0,   540,     0,    29,     0,    48,     0,
       0,   196,   197,   198,     0,     0,     7,    49,     0,     0,
       0,    29,     0,     0,     0,     0,    50,     0,     0,    29,
       0,     0,    10,     0,    52,    11,     0,   131,   132,     0,
       0,     0,     0,     0,     0,   128,   129,   130,   133,   134,
      46,    47,     3,     4,     0,   541,     0,    12,   102,     0,
       0,    48,     0,   136,   553,   137,   138,     0,     0,     7,
      49,   207,   208,   209,   210,   211,   212,   213,   214,    50,
       0,   215,   216,   217,     0,    10,     0,    52,    11,     0,
     131,   132,     0,     0,     0,     0,     0,     0,   128,   129,
     130,   133,   134,    46,    47,     3,     4,     0,   135,     0,
      12,     0,     0,     0,    48,     0,   136,     0,   137,   138,
      67,     0,     7,    49,     0,    68,    69,     0,     0,     0,
       0,    70,    50,     0,     0,    71,    72,     0,    10,     0,
      52,    11,     0,   131,   132,     0,    73,    74,     0,     0,
       0,    75,   281,     0,   133,   134,     0,     0,     0,     0,
       0,   135,     0,    12,     0,     0,     0,     0,     0,   136,
       0,   137,   138,   128,   129,   130,     0,     0,    46,    47,
       3,     4,     0,   357,     0,    46,    47,     3,     4,    48,
       0,     0,     0,     0,     0,     0,    48,     7,    49,     0,
       0,     0,     0,     0,     7,    49,     0,    50,     0,     0,
       0,     0,     0,    10,    50,    52,    11,     0,   131,   132,
      10,    51,    52,   358,     0,     0,   128,   129,   130,   133,
     134,    46,    47,     3,     4,     0,   135,     0,    12,     0,
     436,     0,    48,     0,   136,    12,   137,   138,     0,     0,
       7,    49,   359,     0,     0,     0,     0,     0,     0,     0,
      50,     0,     0,     0,     0,     0,    10,     0,    52,    11,
       0,   131,   132,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   133,   134,     0,    46,    47,     3,     4,   135,
       0,    12,     0,     0,     0,     0,    48,   136,   512,   137,
     138,   128,   129,   130,     7,    49,    46,    47,     3,     4,
       0,     0,     0,   540,    50,     0,     0,    48,     0,     0,
      10,    51,    52,    11,     0,     7,    49,     0,     0,     0,
       0,     0,     0,     0,     0,    50,     0,     0,     0,     0,
       0,    10,     0,    52,    11,    12,   131,   132,    14,     0,
       0,     0,   424,     0,   128,   129,   130,   133,   134,    46,
      47,     3,     4,     0,   541,     0,    12,     0,     0,     0,
      48,     0,   136,     0,   137,   138,     0,     0,     7,    49,
     196,   197,   198,   199,   200,   201,   202,     0,    50,     0,
       0,     0,     0,     0,    10,     0,    52,    11,     0,   131,
     132,   196,   197,   198,   199,   200,   201,   202,     0,     0,
     133,   134,     0,     0,     0,     0,     0,   135,     0,    12,
       0,     0,     0,     0,     0,   136,     0,   137,   138,     0,
       0,     0,     0,     0,     0,   203,     0,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,     0,     0,
     215,   216,   217,     0,   464,     0,   203,     0,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,     0,
       0,   215,   216,   217,     0,   489,   196,   197,   198,   199,
     200,   201,   202,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   196,   197,   198,   199,   200,   201,
     202,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   196,
     197,   198,   199,   200,   201,   202,     0,     0,     0,     0,
       0,   203,   327,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,     0,   328,   215,   216,   217,   203,
     487,   204,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,     0,   488,   215,   216,   217,   196,   197,   198,
     199,   200,   201,   202,   203,     0,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,     0,   218,   215,
     216,   217,   196,   197,   198,   199,   200,   201,   202,     0,
     556,     0,     0,     0,     0,     0,   557,     0,     0,     0,
     196,   197,   198,   199,   200,   201,   202,     0,     0,     0,
       0,     0,   203,     0,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,     0,     0,   215,   216,   217,
     196,   197,   198,   199,   200,   201,   202,   203,   323,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
       0,     0,   215,   216,   217,   203,     0,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,     0,   380,
     215,   216,   217,   196,   197,   198,   199,   200,   201,   202,
       0,     0,     0,     0,     0,   203,     0,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,     0,   521,
     215,   216,   217,   196,   197,   198,   199,   200,   201,   202,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   196,
     197,   198,   199,   200,   201,   202,     0,     0,   203,   525,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,     0,     0,   215,   216,   217,   196,   197,   198,   199,
       0,   201,   202,     0,     0,     0,     0,     0,   203,     0,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,     0,     0,   215,   216,   217,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,     0,     0,   215,
     216,   217,   196,   197,   198,     0,     0,   201,   202,     0,
       0,     0,     0,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,     0,     0,   215,   216,   217,   196,
     197,   198,     0,     0,   201,   202,     0,     0,     0,     0,
       0,     0,     0,   196,   197,   198,     0,     0,   201,   202,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
       0,     0,   215,   216,   217,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,     0,     0,   215,
     216,   217,   206,   207,   208,   209,   210,   211,   212,   213,
     214,     0,     0,   215,   216,   217,   386,     0,     0,    46,
      47,     3,     4,     0,     0,     0,     0,     0,     0,     0,
      48,   387,     0,     0,     0,   388,     0,     0,     7,    49,
       0,     0,     0,     0,     0,     0,     0,   389,    50,     0,
     390,     0,     0,     0,    10,     0,    52,    11,   -50,     0,
       0,   386,     0,     0,    46,    47,     3,     4,     0,     0,
       0,     0,     0,     0,     0,    48,   387,     0,     0,    12,
     388,   391,    14,     7,    49,     0,   392,     0,     0,     0,
       0,     0,   389,    50,     0,   390,     0,     0,     0,    10,
       0,    52,    11,   -50,     0,     0,     0,     0,     0,    46,
      47,     3,     4,     0,    46,    47,     3,     4,     0,     0,
      48,     0,   102,     0,    12,    48,   391,    14,     7,    49,
       0,   417,     0,     7,    49,     0,     0,     0,    50,     0,
       0,     0,     0,    50,    10,    51,    52,    11,     0,    10,
       0,    52,    11,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    12,
       0,     0,     0,     0,    12
};

static const yytype_int16 yycheck[] =
{
       6,   119,    17,   249,     1,    96,    58,   120,    14,   217,
       1,   181,   182,     9,   347,    15,     1,   422,     1,   483,
       1,     6,    57,     6,     9,     1,     9,    15,    81,   161,
      19,    63,    17,     9,    17,    35,    63,    19,    15,    45,
      57,    78,    79,    73,    79,    60,    73,    35,   330,    81,
      56,    81,    58,     5,   518,    61,    62,    77,    35,    57,
      80,    67,    79,    69,    70,    71,    72,    73,    74,    75,
      52,   541,   163,    57,   192,   357,   167,   168,    78,    64,
      76,    64,    82,    68,    82,    68,    81,   492,    63,   559,
      78,    76,    81,    76,   207,   377,   566,   215,    82,    14,
      76,    78,     0,   109,    19,    82,    57,    77,    19,    63,
      80,   117,   118,    65,    66,    67,    68,    69,    70,    17,
      18,    73,    74,    75,    57,    73,    63,    63,    26,    77,
      28,    82,    47,    44,   140,   120,    34,   120,    73,    78,
      38,    53,    81,   149,    79,   151,    15,    16,   481,    82,
     135,    49,    50,    22,   232,    53,    54,   235,    57,    63,
      32,    33,    56,   295,   497,    64,    35,    39,    77,    73,
      74,    75,   157,    73,   157,    44,    80,    75,   230,    77,
      78,    74,   428,    52,   181,   182,    77,   193,   194,    77,
     181,   182,    56,   178,    78,   178,   181,   182,   181,   182,
     181,   182,   334,    57,    56,   181,   182,    73,   340,   341,
      64,   217,    57,    82,    17,    18,    74,    75,    57,    64,
      23,    57,   207,    26,   230,    64,   241,    73,    64,    25,
     236,    34,   238,    29,    30,   443,    63,   355,    73,    54,
     246,    81,    77,   375,    78,   415,    64,    50,    82,    45,
      53,    16,    17,    18,    17,    18,   241,    81,   241,    63,
      54,    26,    78,    26,    15,    16,    82,   358,   244,    34,
      78,    34,    75,   249,    82,    10,    11,    12,    64,    64,
      15,    16,    17,    18,    35,    50,   292,    50,    53,    54,
      53,    26,   538,    44,    78,    73,    74,    75,    82,    34,
      35,    52,   308,   309,    78,    73,    75,    79,    82,    44,
      75,   317,    75,    78,    57,    50,    73,    52,    53,    79,
      55,    56,    73,   441,   330,   301,   302,   440,    57,   305,
     462,    66,    67,    15,    16,    75,    80,    54,    73,    81,
      75,    73,    81,    81,    79,   330,    81,   330,    83,    84,
      81,   357,   337,    35,   337,    37,    79,    81,   364,    79,
     366,    79,    44,    79,    15,    16,    81,   364,   374,    77,
      52,   377,   357,    73,   357,    73,   382,   374,    73,   364,
     377,   364,    44,    80,    35,   503,   377,    81,   364,   374,
     396,   374,   377,    44,   377,    17,    18,   382,   374,   382,
     376,    52,    82,    81,    26,   523,    73,    15,    16,   415,
     425,    79,    34,    77,    77,    81,    77,    77,   415,     3,
       4,     5,    73,    79,     8,     9,    79,    35,    50,    79,
     415,    53,   415,   448,    17,    18,    44,   443,    77,    79,
     425,   447,   425,    26,    52,   451,   422,    77,    77,    73,
      73,    34,   428,    75,    79,   440,    78,   440,    77,    77,
      80,    73,    52,   448,    80,   448,    49,    50,    82,   475,
      53,    54,    81,    73,    81,    81,    80,   483,    89,    63,
      64,    65,    66,    67,    68,    69,    70,   493,    79,    73,
      74,    75,    75,    80,   500,    78,   107,    17,    18,    77,
     483,   477,   478,    79,    81,    73,    26,   483,    15,    16,
      77,    77,   518,   519,    34,    77,   492,    77,   494,   525,
      24,    73,   133,   134,   135,   136,   137,   138,    35,    77,
      50,    77,    79,    53,    54,   518,    81,    44,    59,    77,
      79,    59,   518,    77,    57,    52,    53,    77,   180,   555,
      26,   344,     1,    65,   244,    75,   541,     6,    78,   157,
       9,   537,   538,   542,   415,   494,   349,   477,    17,   182,
     346,     3,     4,     5,     6,     7,     8,     9,   195,    -1,
      -1,    -1,   490,    -1,    -1,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,    36,   216,    68,    69,    70,    -1,
      42,    73,    74,    75,    -1,    64,    -1,   228,    -1,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    58,    76,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      -1,    73,    74,    75,    -1,    -1,   257,    79,   259,    -1,
      -1,    -1,    -1,    -1,    -1,    12,    -1,    -1,    15,    16,
      17,    18,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,
     281,   120,    -1,    -1,    -1,    -1,    -1,    34,    35,    -1,
       3,     4,     5,     6,     7,     8,     9,    44,    -1,    -1,
      -1,    48,    -1,    50,    -1,    52,    53,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,   157,    -1,
      -1,    -1,   323,    -1,    -1,    -1,   327,    -1,    75,    -1,
      -1,    78,    -1,    -1,    -1,    82,    -1,    -1,    -1,   178,
      -1,    -1,   181,   182,    -1,    58,   347,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    -1,
      73,    74,    75,    -1,    -1,    58,    79,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    -1,
      73,    74,    75,    -1,    -1,    -1,    79,    -1,   389,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    15,    16,
      -1,    -1,   241,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,     7,     8,     9,    35,    -1,
      65,    66,    67,    68,    69,    70,    -1,    44,    73,    74,
      75,    -1,    -1,    -1,   435,    52,    53,   438,    -1,    -1,
      -1,   442,    -1,   444,    58,    -1,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    -1,    73,
      74,    75,    -1,    -1,    -1,    79,    58,    -1,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
     481,    73,    74,    75,    15,    16,   487,    79,    -1,   490,
      -1,   330,    -1,    -1,    -1,   496,   497,    12,   337,    -1,
      15,    16,    -1,    -1,    35,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    27,    44,    -1,    -1,    31,    -1,   357,    -1,
      35,    52,    53,    -1,    -1,   364,    -1,    -1,    43,    44,
      -1,    46,    -1,    -1,   535,   374,    -1,    52,   377,    54,
     541,   542,    -1,   382,    66,    67,    68,    69,    70,    -1,
      -1,    73,    74,    75,    -1,   556,   557,    -1,   559,    -1,
      75,    -1,    77,    78,    -1,   566,    -1,    -1,    -1,    -1,
      -1,    -1,    10,    11,    12,    -1,   415,    15,    16,    17,
      18,    -1,    -1,    -1,    22,    -1,   425,    -1,    26,    -1,
      -1,     3,     4,     5,    -1,    -1,    34,    35,    -1,    -1,
      -1,   440,    -1,    -1,    -1,    -1,    44,    -1,    -1,   448,
      -1,    -1,    50,    -1,    52,    53,    -1,    55,    56,    -1,
      -1,    -1,    -1,    -1,    -1,    10,    11,    12,    66,    67,
      15,    16,    17,    18,    -1,    73,    -1,    75,    23,    -1,
      -1,    26,    -1,    81,    82,    83,    84,    -1,    -1,    34,
      35,    63,    64,    65,    66,    67,    68,    69,    70,    44,
      -1,    73,    74,    75,    -1,    50,    -1,    52,    53,    -1,
      55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    10,    11,
      12,    66,    67,    15,    16,    17,    18,    -1,    73,    -1,
      75,    -1,    -1,    -1,    26,    -1,    81,    -1,    83,    84,
      14,    -1,    34,    35,    -1,    19,    20,    -1,    -1,    -1,
      -1,    25,    44,    -1,    -1,    29,    30,    -1,    50,    -1,
      52,    53,    -1,    55,    56,    -1,    40,    41,    -1,    -1,
      -1,    45,    64,    -1,    66,    67,    -1,    -1,    -1,    -1,
      -1,    73,    -1,    75,    -1,    -1,    -1,    -1,    -1,    81,
      -1,    83,    84,    10,    11,    12,    -1,    -1,    15,    16,
      17,    18,    -1,    13,    -1,    15,    16,    17,    18,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    26,    34,    35,    -1,
      -1,    -1,    -1,    -1,    34,    35,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    50,    44,    52,    53,    -1,    55,    56,
      50,    51,    52,    53,    -1,    -1,    10,    11,    12,    66,
      67,    15,    16,    17,    18,    -1,    73,    -1,    75,    -1,
      77,    -1,    26,    -1,    81,    75,    83,    84,    -1,    -1,
      34,    35,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    50,    -1,    52,    53,
      -1,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    66,    67,    -1,    15,    16,    17,    18,    73,
      -1,    75,    -1,    -1,    -1,    -1,    26,    81,    82,    83,
      84,    10,    11,    12,    34,    35,    15,    16,    17,    18,
      -1,    -1,    -1,    22,    44,    -1,    -1,    26,    -1,    -1,
      50,    51,    52,    53,    -1,    34,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    50,    -1,    52,    53,    75,    55,    56,    78,    -1,
      -1,    -1,    82,    -1,    10,    11,    12,    66,    67,    15,
      16,    17,    18,    -1,    73,    -1,    75,    -1,    -1,    -1,
      26,    -1,    81,    -1,    83,    84,    -1,    -1,    34,    35,
       3,     4,     5,     6,     7,     8,     9,    -1,    44,    -1,
      -1,    -1,    -1,    -1,    50,    -1,    52,    53,    -1,    55,
      56,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      66,    67,    -1,    -1,    -1,    -1,    -1,    73,    -1,    75,
      -1,    -1,    -1,    -1,    -1,    81,    -1,    83,    84,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    -1,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    -1,
      73,    74,    75,    -1,    77,    -1,    58,    -1,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      -1,    73,    74,    75,    -1,    77,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    -1,    72,    73,    74,    75,     3,     4,     5,
       6,     7,     8,     9,    58,    -1,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    72,    73,
      74,    75,     3,     4,     5,     6,     7,     8,     9,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    42,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    58,    -1,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    -1,    -1,    73,    74,    75,
       3,     4,     5,     6,     7,     8,     9,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    -1,    73,    74,    75,    58,    -1,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      73,    74,    75,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    58,    -1,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    72,
      73,    74,    75,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    -1,    73,    74,    75,     3,     4,     5,     6,
      -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    58,    -1,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    -1,    73,    74,    75,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    -1,    73,
      74,    75,     3,     4,     5,    -1,    -1,     8,     9,    -1,
      -1,    -1,    -1,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    -1,    73,    74,    75,     3,
       4,     5,    -1,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,    -1,    -1,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    -1,    73,    74,    75,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    -1,    73,
      74,    75,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    -1,    73,    74,    75,    12,    -1,    -1,    15,
      16,    17,    18,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      26,    27,    -1,    -1,    -1,    31,    -1,    -1,    34,    35,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    43,    44,    -1,
      46,    -1,    -1,    -1,    50,    -1,    52,    53,    54,    -1,
      -1,    12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    26,    27,    -1,    -1,    75,
      31,    77,    78,    34,    35,    -1,    82,    -1,    -1,    -1,
      -1,    -1,    43,    44,    -1,    46,    -1,    -1,    -1,    50,
      -1,    52,    53,    54,    -1,    -1,    -1,    -1,    -1,    15,
      16,    17,    18,    -1,    15,    16,    17,    18,    -1,    -1,
      26,    -1,    23,    -1,    75,    26,    77,    78,    34,    35,
      -1,    82,    -1,    34,    35,    -1,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    44,    50,    51,    52,    53,    -1,    50,
      -1,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    75,
      -1,    -1,    -1,    -1,    75
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    86,     0,    17,    18,    26,    28,    34,    38,    49,
      50,    53,    75,    77,    78,    87,    89,    90,    91,    96,
      99,   104,   106,   109,   112,   128,   129,   135,   145,   146,
     147,   148,   149,   155,   156,   157,   159,   161,   165,   167,
     168,   170,   191,   193,    63,    81,    15,    16,    26,    35,
      44,    51,    52,    88,   139,   145,   150,    63,    81,    89,
      90,   145,   156,    63,    63,   105,    88,    14,    19,    20,
      25,    29,    30,    40,    41,    45,    49,    91,   145,    77,
      53,   107,   108,    73,    77,   103,    77,   103,    73,    74,
      77,    56,    88,   169,   136,    77,    88,    56,   169,    88,
      88,    56,    23,   145,   153,   154,   107,    73,    88,   145,
      88,    88,    88,    88,    88,    88,    88,   145,   156,    73,
      63,    54,    89,    92,    93,    94,    81,    81,    10,    11,
      12,    55,    56,    66,    67,    73,    81,    83,    84,    88,
     104,   145,   146,   197,   198,   199,   203,   203,    64,    57,
      82,    63,   151,   151,    64,    82,    64,    57,    64,    54,
     203,    73,    88,   130,   166,   158,   162,    97,   113,   160,
      88,    88,   197,   154,    75,    32,    33,    39,    95,    79,
      57,   110,   132,   203,   203,   145,   203,   200,   203,   203,
     203,    88,    73,    75,    79,    57,     3,     4,     5,     6,
       7,     8,     9,    58,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    73,    74,    75,    72,    88,
      88,   152,   137,   140,   153,    75,    79,    92,    80,   151,
      81,    81,    81,   151,   151,    81,    79,    64,   145,    94,
      89,    90,    99,   111,   115,   116,   145,   192,   193,    16,
      89,    99,   133,   184,   191,   192,   193,    79,    79,    57,
      82,   197,    53,    88,   201,    88,   199,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   145,   154,
     203,    64,   203,   203,   203,   203,   203,   203,   203,   197,
     203,   201,    57,    64,    81,    73,    79,   195,   203,   131,
     169,   163,   163,    98,   114,   163,    88,    88,    44,   145,
      82,    89,   116,    88,    89,   134,   177,    47,   203,   203,
      79,    77,    80,    59,    64,   203,    79,    59,    72,    88,
     138,    92,    81,    77,    73,    82,    82,    89,   164,    82,
      73,    73,    82,    77,    80,    88,    88,    80,   194,    81,
      82,    88,    81,   100,   203,    73,   203,    13,    53,    82,
      88,   139,   142,    79,   178,    92,   145,    92,    92,   100,
      81,   194,   195,    77,   178,    73,    81,   101,    77,   197,
      72,   139,    63,   151,    77,   141,    12,    27,    31,    43,
      46,    77,    82,    88,    89,    99,   104,   171,   172,   173,
     174,   175,   176,   177,   179,   183,   192,   193,   202,    79,
      53,    88,    79,    79,    77,   117,    77,    82,    92,    15,
      35,    89,   185,   186,    82,    90,    99,   102,   139,   145,
     196,    79,    77,   143,    77,    73,    77,   203,    73,    88,
      63,    73,    74,    75,    80,    77,    77,    48,    90,   118,
     119,   145,   171,   192,    79,    80,    80,    19,    52,    82,
     186,   177,    73,   203,    77,   203,   154,   197,   203,   201,
     203,    37,    88,   120,   121,   145,    82,    81,    81,    81,
      52,    80,    92,    79,    79,    64,    79,    59,    72,    77,
      73,    77,   185,    89,   189,   190,   187,    80,   195,    79,
     104,   176,    81,    73,    77,   203,   200,    82,    88,    82,
     190,    77,    82,   188,   203,   195,    77,    77,    24,   180,
     197,    72,    79,    73,    77,    59,    77,   144,   176,    22,
      82,    88,   181,   182,    79,    81,   197,    88,    59,    77,
      22,    73,   122,   123,   124,   125,   127,   203,    79,    89,
     177,   127,   203,    82,   123,    59,    36,    42,    77,    57,
      88,   203,   203,   126,   127,    77,    57,    79,   127
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    85,    86,    86,    86,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    88,    88,    88,    88,    88,    89,
      89,    90,    90,    91,    91,    92,    92,    93,    93,    94,
      95,    95,    95,    95,    97,    98,    96,    99,    99,    99,
      99,   100,   101,   101,   102,   102,   103,   103,   105,   104,
     106,   104,   107,   107,   108,   109,   110,   110,   111,   111,
     111,   113,   114,   112,   115,   115,   116,   117,   117,   118,
     118,   119,   119,   120,   120,   121,   122,   122,   123,   124,
     124,   124,   125,   126,   126,   127,   127,   127,   128,   130,
     131,   129,   132,   132,   133,   133,   133,   133,   133,   134,
     136,   137,   135,   135,   138,   138,   140,   141,   139,   142,
     142,   143,   144,   142,   145,   145,   145,   145,   146,   146,
     147,   148,   148,   149,   149,   149,   149,   149,   149,   150,
     150,   150,   151,   151,   152,   152,   153,   153,   154,   154,
     155,   155,   155,   155,   155,   156,   156,   156,   156,   158,
     157,   160,   159,   162,   161,   163,   163,   164,   164,   166,
     165,   167,   168,   169,   169,   170,   170,   170,   170,   171,
     171,   171,   172,   173,   174,   174,   175,   175,   176,   176,
     176,   176,   176,   176,   176,   177,   178,   178,   179,   180,
     180,   181,   181,   182,   182,   183,   183,   183,   183,   184,
     184,   185,   185,   186,   186,   186,   186,   187,   187,   188,
     189,   189,   190,   190,   191,   192,   192,   193,   194,   194,
     195,   196,   197,   197,   198,   198,   199,   200,   200,   201,
     201,   202,   202,   202,   202,   202,   202,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     1,     2,     2,     5,     0,     1,     1,     3,     4,
       1,     1,     1,     0,     0,     0,     9,     7,     6,     9,
       8,     3,     0,     2,     1,     1,     0,     3,     0,     3,
       0,     2,     0,     3,     3,     6,     0,     2,     1,     1,
       1,     0,     0,     9,     1,     2,     7,     0,     2,     1,
       1,     0,     2,     2,     1,     7,     1,     2,     4,     1,
       1,     1,     5,     1,     3,     1,     3,     3,     7,     0,
       0,     9,     0,     2,     1,     1,     1,     1,     1,     1,
       0,     0,     8,     3,     0,     2,     0,     0,     8,     2,
       3,     0,     0,     8,     1,     1,     1,     1,     1,     2,
       4,     5,     4,     1,     1,     1,     4,     4,     4,     1,
       1,     1,     0,     3,     1,     3,     1,     1,     1,     3,
       1,     2,     2,     2,     2,     1,     1,     1,     1,     0,
       7,     0,     7,     0,     7,     0,     2,     4,     4,     0,
       7,     4,     4,     1,     3,     4,     3,     4,     3,     5,
       8,     4,     1,     2,     2,     3,     5,     7,     1,     1,
       1,     1,     1,     1,     1,     4,     0,     2,     7,     0,
       2,     3,     2,     1,     1,     1,     1,     1,     1,     9,
       6,     1,     2,     5,     5,     6,     5,     0,     2,     5,
       2,     3,     2,     5,     9,     5,     4,     7,     0,     2,
       1,     2,     0,     1,     1,     3,     1,     1,     3,     1,
       1,     1,     1,     2,     3,     4,     6,     1,     1,     1,
       1,     1,     1,     2,     4,     6,     3,     3,     2,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       4,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     7,     4,     4,     4
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 235 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { declarations->push_back((yyvsp[0].Node)->getNode()); }
#line 2262 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 4:
#line 236 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    {}
#line 2268 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 5:
#line 240 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Declaration); }
#line 2274 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 6:
#line 241 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Node); }
#line 2280 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 7:
#line 242 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Declaration); }
#line 2286 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 8:
#line 243 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Type_Declaration); }
#line 2292 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 9:
#line 244 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Type_Declaration); }
#line 2298 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 10:
#line 245 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Type_Declaration); }
#line 2304 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 11:
#line 246 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Declaration); }
#line 2310 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 12:
#line 247 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Node); }
#line 2316 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 13:
#line 248 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[0].Node); }
#line 2322 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 14:
#line 252 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.id) = new IR::ID((yylsp[0]), (yyvsp[0].str)); }
#line 2328 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 15:
#line 253 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.id) = new IR::ID((yylsp[0]), "apply"); }
#line 2334 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 16:
#line 254 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.id) = new IR::ID((yylsp[0]), "key"); }
#line 2340 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 17:
#line 255 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.id) = new IR::ID((yylsp[0]), "actions"); }
#line 2346 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 18:
#line 256 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.id) = new IR::ID((yylsp[0]), "state"); }
#line 2352 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 19:
#line 260 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.annos) = IR::Annotations::empty; }
#line 2358 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 20:
#line 261 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.annos) = new IR::Annotations((yylsp[0]), (yyvsp[0].annoVec)); }
#line 2364 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 21:
#line 265 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.annoVec) = new IR::Vector<IR::Annotation>(); (yyval.annoVec)->push_back((yyvsp[0].Annotation)); }
#line 2370 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 22:
#line 266 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.annoVec) = (yyvsp[-1].annoVec); (yyval.annoVec)->push_back((yyvsp[0].Annotation)); }
#line 2376 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 23:
#line 270 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Annotation) = new IR::Annotation((yylsp[-1]), *(yyvsp[0].id), nullptr); }
#line 2382 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 24:
#line 271 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Annotation) = new IR::Annotation((yylsp[-4]), *(yyvsp[-3].id), (yyvsp[-1].Expression)); }
#line 2388 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 25:
#line 275 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.params) = new IR::IndexedVector<IR::Parameter>(); }
#line 2394 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 26:
#line 276 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[0].params); }
#line 2400 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 27:
#line 280 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.params) = new IR::IndexedVector<IR::Parameter>();
                                            (yyval.params)->push_back((yyvsp[0].Parameter)); }
#line 2407 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 28:
#line 282 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[-2].params); (yyval.params)->push_back((yyvsp[0].Parameter)); }
#line 2413 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 29:
#line 286 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Parameter) = new IR::Parameter((yylsp[-3]) + (yylsp[0]), *(yyvsp[0].id), (yyvsp[-3].annos), (yyvsp[-2].dir), (yyvsp[-1].TypePtr)); }
#line 2419 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 30:
#line 290 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.dir) = IR::Direction::In; }
#line 2425 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 31:
#line 291 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.dir) = IR::Direction::Out; }
#line 2431 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 32:
#line 292 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.dir) = IR::Direction::InOut; }
#line 2437 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 33:
#line 293 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.dir) = IR::Direction::None; }
#line 2443 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 34:
#line 297 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pushContainerType(*(yyvsp[0].id), false); }
#line 2449 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 35:
#line 298 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareTypes((yyvsp[0].TypeParameters)->parameters); }
#line 2455 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 36:
#line 299 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto pl = new IR::ParameterList((yylsp[-1]), (yyvsp[-1].params));
                                             (yyval.Type_Declaration) = new IR::Type_Package((yylsp[-6]), *(yyvsp[-6].id), (yyvsp[-8].annos), (yyvsp[-4].TypeParameters), pl); }
#line 2462 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 37:
#line 305 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::Declaration_Instance((yylsp[-1]), *(yyvsp[-1].id), new IR::Annotations((yyvsp[-6].annoVec)),
                                                         (yyvsp[-5].TypePtr), (yyvsp[-3].exprs), nullptr); }
#line 2469 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 38:
#line 308 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::Declaration_Instance((yylsp[-1]), *(yyvsp[-1].id), IR::Annotations::empty,
                                                         (yyvsp[-5].TypePtr), (yyvsp[-3].exprs), nullptr); }
#line 2476 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 39:
#line 311 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::Declaration_Instance((yylsp[-3]), *(yyvsp[-3].id), new IR::Annotations((yyvsp[-8].annoVec)),
                                                         (yyvsp[-7].TypePtr), (yyvsp[-5].exprs), (yyvsp[-1].BlockStatement)); }
#line 2483 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 40:
#line 314 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::Declaration_Instance((yylsp[-3]), *(yyvsp[-3].id), IR::Annotations::empty,
                                                         (yyvsp[-7].TypePtr), (yyvsp[-5].exprs), (yyvsp[-1].BlockStatement)); }
#line 2490 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 41:
#line 320 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.BlockStatement) = new IR::BlockStatement((yylsp[-2])+(yylsp[0]), IR::Annotations::empty, (yyvsp[-1].statOrDecls)); }
#line 2496 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 42:
#line 324 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.statOrDecls) = new IR::IndexedVector<IR::StatOrDecl>(); }
#line 2502 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 43:
#line 325 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.statOrDecls) = (yyvsp[-1].statOrDecls); (yyvsp[-1].statOrDecls)->push_back((yyvsp[0].Declaration)); }
#line 2508 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 44:
#line 329 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2514 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 45:
#line 330 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2520 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 46:
#line 334 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.params) = new IR::IndexedVector<IR::Parameter>(); }
#line 2526 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 47:
#line 335 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.params) = (yyvsp[-1].params); }
#line 2532 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 48:
#line 339 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.startAbsolutePath(); }
#line 2538 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 49:
#line 340 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.clearPath(); (yyval.PathPrefix) = (yyvsp[0].PathPrefix); (yyval.PathPrefix)->setAbsolute(); }
#line 2544 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 50:
#line 341 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.startRelativePath(); }
#line 2550 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 51:
#line 342 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.clearPath(); (yyval.PathPrefix) = (yyvsp[0].PathPrefix); }
#line 2556 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 52:
#line 346 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.PathPrefix) = new IR::PathPrefix(); }
#line 2562 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 53:
#line 347 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pathAppendNamespace((yyvsp[-1].str));
                                         (yyval.PathPrefix) = new IR::PathPrefix((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].PathPrefix), IR::ID((yylsp[-1]), (yyvsp[-1].str))); }
#line 2569 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 54:
#line 352 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pathAppendNamespace((yyvsp[-1].str));
                                         (yyval.PathPrefix) = new IR::PathPrefix((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].PathPrefix), IR::ID((yylsp[-1]), (yyvsp[-1].str))); }
#line 2576 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 55:
#line 361 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop();
                               auto pl = new IR::ParameterList((yylsp[-4]), (yyvsp[-4].params));
                               (yyval.Type_Declaration) = new IR::P4Parser((yyvsp[-5].Type_Parser)->name.srcInfo, (yyvsp[-5].Type_Parser)->name,
                                                     (yyvsp[-5].Type_Parser), pl, (yyvsp[-2].decls), (yyvsp[-1].parserStates));}
#line 2585 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 56:
#line 368 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.decls) = new IR::IndexedVector<IR::Declaration>(); }
#line 2591 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 57:
#line 369 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.decls) = (yyvsp[-1].decls); (yyval.decls)->push_back((yyvsp[0].Declaration)); }
#line 2597 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 58:
#line 373 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2603 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 59:
#line 374 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2609 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 60:
#line 375 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2615 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 61:
#line 379 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pushContainerType(*(yyvsp[0].id), false); }
#line 2621 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 62:
#line 380 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareTypes((yyvsp[0].TypeParameters)->parameters); }
#line 2627 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 63:
#line 382 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto pl = new IR::ParameterList((yylsp[-1]), (yyvsp[-1].params));
                                        (yyval.Type_Parser) = new IR::Type_Parser((yylsp[-6]), *(yyvsp[-6].id), (yyvsp[-8].annos), (yyvsp[-4].TypeParameters), pl); }
#line 2634 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 64:
#line 387 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.parserStates) = new IR::IndexedVector<IR::ParserState>();
                                        (yyval.parserStates)->push_back((yyvsp[0].ParserState)); }
#line 2641 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 65:
#line 389 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.parserStates) = (yyvsp[-1].parserStates); (yyval.parserStates)->push_back((yyvsp[0].ParserState)); }
#line 2647 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 66:
#line 394 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.ParserState) = new IR::ParserState((yylsp[-4]), *(yyvsp[-4].id), (yyvsp[-6].annos), (yyvsp[-2].statOrDecls), (yyvsp[-1].Expression)); }
#line 2653 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 67:
#line 398 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.statOrDecls) = new IR::IndexedVector<IR::StatOrDecl>(); }
#line 2659 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 68:
#line 399 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.statOrDecls) = (yyvsp[-1].statOrDecls); (yyvsp[-1].statOrDecls)->push_back((yyvsp[0].StatOrDecl)); }
#line 2665 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 69:
#line 403 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StatOrDecl) = (yyvsp[0].Statement); }
#line 2671 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 70:
#line 404 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StatOrDecl) = (yyvsp[0].Declaration); }
#line 2677 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 71:
#line 408 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = nullptr; }
#line 2683 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 72:
#line 409 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 2689 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 73:
#line 413 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::PathExpression(*(yyvsp[-1].id)); }
#line 2695 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 74:
#line 414 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 2701 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 75:
#line 419 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::SelectExpression((yylsp[-6]) + (yylsp[0]),
                                      new IR::ListExpression((yylsp[-4]), (yyvsp[-4].exprs)), std::move(*(yyvsp[-1].selectCases))); }
#line 2708 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 76:
#line 424 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.selectCases) = new IR::Vector<IR::SelectCase>(); (yyval.selectCases)->push_back((yyvsp[0].SelectCase)); }
#line 2714 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 77:
#line 425 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.selectCases) = (yyvsp[-1].selectCases); (yyval.selectCases)->push_back((yyvsp[0].SelectCase)); }
#line 2720 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 78:
#line 429 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto expr = new IR::PathExpression(*(yyvsp[-1].id));
                                      (yyval.SelectCase) = new IR::SelectCase((yylsp[-3]) + (yylsp[-1]), (yyvsp[-3].Expression), expr); }
#line 2727 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 79:
#line 434 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::DefaultExpression((yylsp[0])); }
#line 2733 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 80:
#line 435 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::ListExpression((yylsp[0]), (yyvsp[0].exprs)); }
#line 2739 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 81:
#line 436 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 2745 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 82:
#line 442 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = (yyvsp[-1].exprs); (yyvsp[-1].exprs)->insert((yyvsp[-1].exprs)->begin(), (yyvsp[-3].Expression)); }
#line 2751 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 83:
#line 446 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = new IR::Vector<IR::Expression>(); (yyval.exprs)->push_back((yyvsp[0].Expression)); }
#line 2757 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 84:
#line 447 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = (yyvsp[-2].exprs); (yyval.exprs)->push_back((yyvsp[0].Expression)); }
#line 2763 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 85:
#line 451 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 2769 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 86:
#line 452 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Mask((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 2775 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 87:
#line 453 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Range((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 2781 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 88:
#line 461 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop();
                              auto pl = new IR::ParameterList((yylsp[-5]), (yyvsp[-5].params));
                              (yyval.Type_Declaration) = new IR::P4Control((yyvsp[-6].Type_Control)->name.srcInfo, (yyvsp[-6].Type_Control)->name, (yyvsp[-6].Type_Control), pl, (yyvsp[-3].decls), (yyvsp[-1].BlockStatement)); }
#line 2789 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 89:
#line 467 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pushContainerType(*(yyvsp[0].id), false); }
#line 2795 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 90:
#line 468 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareTypes((yyvsp[0].TypeParameters)->parameters); }
#line 2801 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 91:
#line 470 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto pl = new IR::ParameterList((yylsp[-1]), (yyvsp[-1].params));
                                (yyval.Type_Control) = new IR::Type_Control((yylsp[-6]), *(yyvsp[-6].id), (yyvsp[-8].annos), (yyvsp[-4].TypeParameters), pl); }
#line 2808 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 92:
#line 475 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.decls) = new IR::IndexedVector<IR::Declaration>(); }
#line 2814 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 93:
#line 476 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.decls) = (yyvsp[-1].decls); (yyval.decls)->push_back((yyvsp[0].Declaration)); }
#line 2820 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 94:
#line 480 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2826 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 95:
#line 481 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2832 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 96:
#line 482 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2838 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 97:
#line 483 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2844 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 98:
#line 484 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = (yyvsp[0].Declaration); }
#line 2850 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 99:
#line 488 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.BlockStatement) = (yyvsp[0].BlockStatement); }
#line 2856 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 100:
#line 494 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pushContainerType(*(yyvsp[0].id), true); }
#line 2862 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 101:
#line 495 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareTypes((yyvsp[0].TypeParameters)->parameters); }
#line 2868 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 102:
#line 497 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop();
                                         (yyval.Node) = new IR::Type_Extern((yylsp[-6]), *(yyvsp[-6].id), (yyvsp[-4].TypeParameters), (yyvsp[-1].methods)); }
#line 2875 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 103:
#line 499 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = (yyvsp[-1].Method); }
#line 2881 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 104:
#line 503 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.methods) = new IR::Vector<IR::Method>(); }
#line 2887 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 105:
#line 504 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.methods) = (yyvsp[-1].methods); (yyvsp[-1].methods)->push_back((yyvsp[0].Method)); }
#line 2893 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 106:
#line 508 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pushNamespace("", (yylsp[-1]), false);
                     structure.declareTypes((yyvsp[0].TypeParameters)->parameters); }
#line 2900 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 107:
#line 510 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop(); }
#line 2906 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 108:
#line 511 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto params = new IR::ParameterList((yylsp[-2]), (yyvsp[-2].params));
                                          auto mt = new IR::Type_Method((yylsp[-6]), (yyvsp[-5].TypeParameters), (yyvsp[-7].TypePtr), params);
                                          (yyval.Method) = new IR::Method((yylsp[-6]), *(yyvsp[-6].id), mt, false); }
#line 2914 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 109:
#line 517 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Method) = (yyvsp[-1].Method); }
#line 2920 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 110:
#line 518 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Method) = (yyvsp[-1].Method); (yyval.Method)->setAbstract(); }
#line 2926 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 111:
#line 519 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pushNamespace("", (yylsp[-1]), false);
                     structure.declareTypes((yyvsp[0].TypeParameters)->parameters); }
#line 2933 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 112:
#line 521 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop(); }
#line 2939 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 113:
#line 522 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto par = new IR::ParameterList((yylsp[-3]), (yyvsp[-3].params));
                                          auto mt = new IR::Type_Method((yylsp[-7]), (yyvsp[-6].TypeParameters), nullptr, par);
                                          (yyval.Method) = new IR::Method((yylsp[-7]), IR::ID((yylsp[-7]), (yyvsp[-7].str)), mt, false); }
#line 2947 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 114:
#line 530 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = (yyvsp[0].TypePtr); }
#line 2953 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 115:
#line 531 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = (yyvsp[0].Type_Name); }
#line 2959 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 116:
#line 532 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = (yyvsp[0].TypePtr); }
#line 2965 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 117:
#line 533 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = (yyvsp[0].TypePtr); }
#line 2971 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 118:
#line 537 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Name) = new IR::Type_Name((yylsp[0]), new IR::Path(IR::ID((yylsp[0]), (yyvsp[0].str)))); }
#line 2977 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 119:
#line 538 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Name) = new IR::Type_Name((yylsp[-1]), new IR::Path((yyvsp[-1].PathPrefix), IR::ID((yylsp[0]), (yyvsp[0].str)))); }
#line 2983 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 120:
#line 542 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = new IR::Type_Stack((yylsp[-3])+(yylsp[0]), (yyvsp[-3].Type_Name), (yyvsp[-1].Expression)); }
#line 2989 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 121:
#line 547 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto id = IR::ID((yylsp[-3]), (yyvsp[-3].str));
                                         auto type = new IR::Type_Name((yylsp[-4]), new IR::Path((yyvsp[-4].PathPrefix), id));
                                         (yyval.TypePtr) = new IR::Type_Specialized((yylsp[-4]) + (yylsp[0]), type, (yyvsp[-1].types)); }
#line 2997 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 122:
#line 550 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto id = IR::ID((yylsp[-3]), (yyvsp[-3].str));
                                         auto type = new IR::Type_Name((yylsp[-3]), new IR::Path(id));
                                         (yyval.TypePtr) = new IR::Type_Specialized((yylsp[-3]) + (yylsp[0]), type, (yyvsp[-1].types)); }
#line 3005 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 123:
#line 556 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type_Boolean::get(); }
#line 3011 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 124:
#line 557 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type_Error::get(); }
#line 3017 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 125:
#line 558 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type::Bits::get((yylsp[0]), 1); }
#line 3023 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 126:
#line 559 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type::Bits::get((yylsp[-1]), (yyvsp[-1].Constant)->asInt(), false); }
#line 3029 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 127:
#line 560 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type::Bits::get((yylsp[-1]), (yyvsp[-1].Constant)->asInt(), true); }
#line 3035 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 128:
#line 561 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type::Varbits::get((yylsp[-1]), (yyvsp[-1].Constant)->asInt()); }
#line 3041 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 129:
#line 565 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = (yyvsp[0].TypePtr); }
#line 3047 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 130:
#line 566 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type_Void::get(); }
#line 3053 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 131:
#line 567 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = new IR::Type_Name((yylsp[0]), new IR::Path(*(yyvsp[0].id))); }
#line 3059 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 132:
#line 572 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypeParameters) = new IR::TypeParameters(); }
#line 3065 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 133:
#line 573 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypeParameters) = new IR::TypeParameters((yylsp[-2])+(yylsp[0]), (yyvsp[-1].nm)); }
#line 3071 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 134:
#line 577 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.nm) = new IR::IndexedVector<IR::Type_Var>();
                                         (yyval.nm)->push_back(new IR::Type_Var((yylsp[0]), *(yyvsp[0].id))); }
#line 3078 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 135:
#line 579 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { ((yyval.nm)=(yyvsp[-2].nm))->push_back(new IR::Type_Var((yylsp[0]), *(yyvsp[0].id))); }
#line 3084 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 136:
#line 583 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = (yyvsp[0].TypePtr); }
#line 3090 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 137:
#line 584 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.TypePtr) = IR::Type_Dontcare::get(); }
#line 3096 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 138:
#line 588 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.types) = new IR::Vector<IR::Type>(); (yyval.types)->push_back((yyvsp[0].TypePtr)); }
#line 3102 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 139:
#line 589 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.types) = (yyvsp[-2].types); (yyval.types)->push_back((yyvsp[0].TypePtr)); }
#line 3108 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 140:
#line 593 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = (yyvsp[0].Type_Declaration); }
#line 3114 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 141:
#line 594 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = (yyvsp[-1].Type_Declaration); }
#line 3120 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 142:
#line 595 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop(); (yyval.Type_Declaration) = (yyvsp[-1].Type_Parser); }
#line 3126 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 143:
#line 596 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop(); (yyval.Type_Declaration) = (yyvsp[-1].Type_Control); }
#line 3132 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 144:
#line 597 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.pop(); (yyval.Type_Declaration) = (yyvsp[-1].Type_Declaration); }
#line 3138 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 145:
#line 601 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = (yyvsp[0].Type_Declaration); }
#line 3144 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 146:
#line 602 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = (yyvsp[0].Type_Declaration); }
#line 3150 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 147:
#line 603 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = (yyvsp[0].Type_Declaration); }
#line 3156 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 148:
#line 604 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = (yyvsp[0].Type_Declaration); }
#line 3162 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 149:
#line 608 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id)); }
#line 3168 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 150:
#line 609 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = new IR::Type_Header((yylsp[-4]), *(yyvsp[-4].id), (yyvsp[-6].annos), (yyvsp[-1].structFields)); }
#line 3174 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 151:
#line 613 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id)); }
#line 3180 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 152:
#line 614 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = new IR::Type_Struct((yylsp[-4]), *(yyvsp[-4].id), (yyvsp[-6].annos), (yyvsp[-1].structFields)); }
#line 3186 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 153:
#line 618 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id)); }
#line 3192 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 154:
#line 619 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = new IR::Type_Union((yylsp[-4]), *(yyvsp[-4].id), (yyvsp[-6].annos), (yyvsp[-1].structFields)); }
#line 3198 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 155:
#line 623 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.structFields) = new IR::IndexedVector<IR::StructField>(); }
#line 3204 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 156:
#line 624 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.structFields) = (yyvsp[-1].structFields); (yyvsp[-1].structFields)->push_back((yyvsp[0].StructField)); }
#line 3210 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 157:
#line 628 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StructField) = new IR::StructField((yylsp[-1]), *(yyvsp[-1].id), (yyvsp[-3].annos), (yyvsp[-2].TypePtr)); }
#line 3216 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 158:
#line 629 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StructField) = new IR::StructField((yylsp[-1]), IR::ID((yylsp[-1]), (yyvsp[-1].str)), (yyvsp[-3].annos), (yyvsp[-2].TypePtr)); }
#line 3222 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 159:
#line 633 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id)); }
#line 3228 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 160:
#line 634 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Type_Declaration) = new IR::Type_Enum((yylsp[-6]) + (yylsp[-4]), *(yyvsp[-4].id), (yyvsp[-1].ids)); }
#line 3234 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 161:
#line 638 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = new IR::Declaration_Errors((yylsp[-3]) + (yylsp[0]), (yyvsp[-1].ids)); }
#line 3240 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 162:
#line 642 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Node) = new IR::Declaration_MatchKind((yylsp[-3]) + (yylsp[0]), (yyvsp[-1].ids)); }
#line 3246 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 163:
#line 646 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.ids) = new IR::IndexedVector<IR::Declaration_ID>();
                                          (yyval.ids)->push_back(new IR::Declaration_ID((yylsp[0]), *(yyvsp[0].id)));}
#line 3253 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 164:
#line 648 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.ids) = (yyvsp[-2].ids); (yyval.ids)->push_back(new IR::Declaration_ID((yylsp[0]), *(yyvsp[0].id))); }
#line 3259 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 165:
#line 652 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id));
          (yyval.Type_Declaration) = new IR::Type_Typedef((yylsp[0]), *(yyvsp[0].id), new IR::Annotations((yyvsp[-3].annoVec)), (yyvsp[-1].TypePtr)); }
#line 3266 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 166:
#line 654 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id));
          (yyval.Type_Declaration) = new IR::Type_Typedef((yylsp[0]), *(yyvsp[0].id), IR::Annotations::empty, (yyvsp[-1].TypePtr)); }
#line 3273 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 167:
#line 656 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id));
                        (yyval.Type_Declaration) = new IR::Type_Typedef((yylsp[0]), *(yyvsp[0].id), new IR::Annotations((yyvsp[-3].annoVec)), (yyvsp[-1].Type_Declaration)); }
#line 3280 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 168:
#line 658 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { structure.declareType(*(yyvsp[0].id));
                        (yyval.Type_Declaration) = new IR::Type_Typedef((yylsp[0]), *(yyvsp[0].id), IR::Annotations::empty, (yyvsp[-1].Type_Declaration)); }
#line 3287 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 169:
#line 666 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto mc = new IR::MethodCallExpression((yylsp[-4]) + (yylsp[-1]), (yyvsp[-4].Expression),
                                                               new IR::Vector<IR::Type>(), (yyvsp[-2].exprs));
                                         (yyval.Statement) = new IR::MethodCallStatement((yylsp[-4]) + (yylsp[-1]), mc); }
#line 3295 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 170:
#line 670 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto mc = new IR::MethodCallExpression((yylsp[-7]) + (yylsp[-1]),
                                                                                (yyvsp[-7].Expression), (yyvsp[-5].types), (yyvsp[-2].exprs));
                                         (yyval.Statement) = new IR::MethodCallStatement((yylsp[-7]) + (yylsp[-1]), mc); }
#line 3303 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 171:
#line 673 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = new IR::AssignmentStatement((yylsp[-2]), (yyvsp[-3].Expression), (yyvsp[-1].Expression)); }
#line 3309 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 172:
#line 677 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = new IR::EmptyStatement((yylsp[0])); }
#line 3315 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 173:
#line 681 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = new IR::ExitStatement((yylsp[-1])); }
#line 3321 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 174:
#line 685 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = new IR::ReturnStatement((yylsp[-1]), nullptr); }
#line 3327 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 175:
#line 686 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = new IR::ReturnStatement((yylsp[-2]), (yyvsp[-1].Expression)); }
#line 3333 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 176:
#line 691 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = new IR::IfStatement((yylsp[-4]), (yyvsp[-2].Expression), (yyvsp[0].Statement), nullptr); }
#line 3339 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 177:
#line 693 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = new IR::IfStatement((yylsp[-6]), (yyvsp[-4].Expression), (yyvsp[-2].Statement), (yyvsp[0].Statement)); }
#line 3345 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 178:
#line 697 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = (yyvsp[0].Statement); }
#line 3351 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 179:
#line 698 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = (yyvsp[0].Statement); }
#line 3357 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 180:
#line 699 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = (yyvsp[0].Statement); }
#line 3363 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 181:
#line 700 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = (yyvsp[0].BlockStatement); }
#line 3369 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 182:
#line 701 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = (yyvsp[0].Statement); }
#line 3375 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 183:
#line 702 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = (yyvsp[0].Statement); }
#line 3381 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 184:
#line 703 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Statement) = (yyvsp[0].Statement); }
#line 3387 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 185:
#line 707 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.BlockStatement) = new IR::BlockStatement((yylsp[-3]) + (yylsp[0]), (yyvsp[-3].annos), (yyvsp[-1].statOrDecls)); }
#line 3393 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 186:
#line 711 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.statOrDecls) = new IR::IndexedVector<IR::StatOrDecl>(); }
#line 3399 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 187:
#line 712 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.statOrDecls) = (yyvsp[-1].statOrDecls); (yyval.statOrDecls)->push_back((yyvsp[0].StatOrDecl)); }
#line 3405 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 188:
#line 716 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    {
            (yyval.Statement) = new IR::SwitchStatement((yylsp[-6]), (yyvsp[-4].Expression), std::move(*(yyvsp[-1].switchCases))); }
#line 3412 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 189:
#line 721 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.switchCases) = new IR::Vector<IR::SwitchCase>(); }
#line 3418 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 190:
#line 722 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.switchCases) = (yyvsp[-1].switchCases); (yyval.switchCases)->push_back((yyvsp[0].SwitchCase)); }
#line 3424 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 191:
#line 726 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.SwitchCase) = new IR::SwitchCase((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].BlockStatement)); }
#line 3430 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 192:
#line 727 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.SwitchCase) = new IR::SwitchCase((yylsp[-1]), (yyvsp[-1].Expression), nullptr); }
#line 3436 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 193:
#line 731 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::PathExpression(*(yyvsp[0].id)); }
#line 3442 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 194:
#line 732 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::DefaultExpression((yylsp[0])); }
#line 3448 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 195:
#line 736 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StatOrDecl) = (yyvsp[0].Declaration); }
#line 3454 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 196:
#line 737 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StatOrDecl) = (yyvsp[0].Declaration); }
#line 3460 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 197:
#line 738 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StatOrDecl) = (yyvsp[0].Statement); }
#line 3466 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 198:
#line 739 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.StatOrDecl) = (yyvsp[0].Declaration); }
#line 3472 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 199:
#line 746 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::P4Table((yylsp[-6]), *(yyvsp[-6].id), (yyvsp[-8].annos),
                                                     new IR::ParameterList((yylsp[-4]), (yyvsp[-4].params)),
                                                     new IR::TableProperties((yylsp[-1]), (yyvsp[-1].properties))); }
#line 3480 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 200:
#line 750 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::P4Table((yylsp[-3]), *(yyvsp[-3].id), (yyvsp[-5].annos),
                                                     new IR::ParameterList(),
                                                     new IR::TableProperties((yylsp[-1]), (yyvsp[-1].properties))); }
#line 3488 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 201:
#line 756 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.properties) = new IR::IndexedVector<IR::TableProperty>();
                                           (yyval.properties)->push_back((yyvsp[0].TableProperty)); }
#line 3495 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 202:
#line 758 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.properties) = (yyvsp[-1].properties); (yyval.properties)->push_back((yyvsp[0].TableProperty)); }
#line 3501 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 203:
#line 762 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto v = new IR::Key((yylsp[-1]), (yyvsp[-1].keyElements));
                                           auto id = IR::ID(
                                               (yylsp[-4]), IR::TableProperties::keyPropertyName);
                                           (yyval.TableProperty) = new IR::TableProperty(
                                               (yylsp[-4]) + (yylsp[0]), id, IR::Annotations::empty, v, false); }
#line 3511 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 204:
#line 767 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto v = new IR::ActionList((yylsp[-1]), (yyvsp[-1].actions));
                                           auto id = IR::ID(
                                               (yylsp[-4]), IR::TableProperties::actionsPropertyName);
                                           (yyval.TableProperty) = new IR::TableProperty(
                                               (yylsp[-4]) + (yylsp[0]), id, IR::Annotations::empty, v, false); }
#line 3521 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 205:
#line 773 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto v = new IR::ExpressionValue((yylsp[-1]), (yyvsp[-1].Expression));
                                            auto id = IR::ID((yylsp[-3]), (yyvsp[-3].str));
                                            (yyval.TableProperty) = new IR::TableProperty((yylsp[-3]), id, (yyvsp[-5].annos), v, true); }
#line 3529 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 206:
#line 777 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto v = new IR::ExpressionValue((yylsp[-1]), (yyvsp[-1].Expression));
                                           auto id = IR::ID((yylsp[-3]), (yyvsp[-3].str));
                                           (yyval.TableProperty) = new IR::TableProperty((yylsp[-3]), id, (yyvsp[-4].annos), v, false); }
#line 3537 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 207:
#line 783 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.keyElements) = new IR::Vector<IR::KeyElement>(); }
#line 3543 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 208:
#line 784 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.keyElements) = (yyvsp[-1].keyElements); (yyval.keyElements)->push_back((yyvsp[0].KeyElement)); }
#line 3549 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 209:
#line 789 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto expr = new IR::PathExpression(*(yyvsp[-2].id));
                                           (yyval.KeyElement) = new IR::KeyElement((yylsp[-2]), (yyvsp[-1].annos), (yyvsp[-4].Expression), expr); }
#line 3556 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 210:
#line 794 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.actions) = new IR::IndexedVector<IR::ActionListElement>();
                                           (yyval.actions)->push_back((yyvsp[-1].ActionListElement)); }
#line 3563 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 211:
#line 796 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.actions) = (yyvsp[-2].actions); (yyval.actions)->push_back((yyvsp[-1].ActionListElement)); }
#line 3569 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 212:
#line 800 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto expr = new IR::PathExpression(*(yyvsp[0].id));
                                           (yyval.ActionListElement) = new IR::ActionListElement((yylsp[0]), (yyvsp[-1].annos), expr);}
#line 3576 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 213:
#line 803 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto method = new IR::PathExpression(*(yyvsp[-3].id));
                                           auto mce = new IR::MethodCallExpression(
                                               (yylsp[-3])+(yylsp[-1]), method, new IR::Vector<IR::Type>(), (yyvsp[-1].exprs));
                                           (yyval.ActionListElement) = new IR::ActionListElement((yylsp[-3]), (yyvsp[-4].annos), mce); }
#line 3585 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 214:
#line 813 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto pl = new IR::ParameterList((yylsp[-4]), (yyvsp[-4].params));
                                           (yyval.Declaration) = new IR::P4Action(
                                               (yylsp[-6]), *(yyvsp[-6].id), (yyvsp[-8].annos), pl,
                                               new IR::BlockStatement((yylsp[-1]), IR::Annotations::empty, (yyvsp[-1].statOrDecls))); }
#line 3594 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 215:
#line 823 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { auto ann = new IR::Annotations((yylsp[-4]), (yyvsp[-4].annoVec));
                                       (yyval.Declaration) = new IR::Declaration_Variable((yylsp[-4])+(yylsp[-1]), *(yyvsp[-2].id), ann, (yyvsp[-3].TypePtr), (yyvsp[-1].Expression)); }
#line 3601 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 216:
#line 826 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::Declaration_Variable(
                                         (yylsp[-3])+(yylsp[0]), *(yyvsp[-2].id), IR::Annotations::empty, (yyvsp[-3].TypePtr), (yyvsp[-1].Expression));}
#line 3608 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 217:
#line 832 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::Declaration_Constant((yylsp[-6])+(yylsp[-1]), *(yyvsp[-3].id), (yyvsp[-6].annos), (yyvsp[-4].TypePtr), (yyvsp[-1].Expression)); }
#line 3614 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 218:
#line 836 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = nullptr; }
#line 3620 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 219:
#line 837 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 3626 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 220:
#line 841 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 3632 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 221:
#line 847 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Declaration) = new IR::Function((yylsp[-1])+(yylsp[0]), (yyvsp[-1].Method)->name, (yyvsp[-1].Method)->type, (yyvsp[0].BlockStatement)); }
#line 3638 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 222:
#line 851 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = new IR::Vector<IR::Expression>(); }
#line 3644 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 223:
#line 852 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = (yyvsp[0].exprs); }
#line 3650 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 224:
#line 856 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = new IR::Vector<IR::Expression>();
                                           (yyval.exprs)->push_back((yyvsp[0].Expression)); }
#line 3657 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 225:
#line 858 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = (yyvsp[-2].exprs); (yyval.exprs)->push_back((yyvsp[0].Expression)); }
#line 3663 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 226:
#line 862 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 3669 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 227:
#line 866 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = new IR::Vector<IR::Expression>();
                                           (yyval.exprs)->push_back((yyvsp[0].Expression)); }
#line 3676 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 228:
#line 868 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.exprs) = (yyvsp[-2].exprs); (yyval.exprs)->push_back((yyvsp[0].Expression)); }
#line 3682 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 230:
#line 873 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.id) = new IR::ID((yylsp[0]), (yyvsp[0].str)); }
#line 3688 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 231:
#line 877 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::PathExpression(*(yyvsp[0].id)); }
#line 3694 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 232:
#line 878 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::This((yylsp[0])); }
#line 3700 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 233:
#line 879 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::PathExpression(new IR::Path((yyvsp[-1].PathPrefix), *(yyvsp[0].id))); }
#line 3706 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 234:
#line 880 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Member((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), *(yyvsp[0].id)); }
#line 3712 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 235:
#line 881 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::ArrayIndex((yylsp[-3]) + (yylsp[-1]), (yyvsp[-3].Expression), (yyvsp[-1].Expression)); }
#line 3718 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 236:
#line 882 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Slice((yylsp[-5]) + (yylsp[0]), (yyvsp[-5].Expression), (yyvsp[-3].Expression), (yyvsp[-1].Expression)); }
#line 3724 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 237:
#line 886 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Constant); }
#line 3730 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 238:
#line 887 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::StringLiteral((yylsp[0]), (yyvsp[0].str)); }
#line 3736 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 239:
#line 888 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::BoolLiteral((yylsp[0]), true); }
#line 3742 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 240:
#line 889 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::BoolLiteral((yylsp[0]), false); }
#line 3748 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 241:
#line 890 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::This((yylsp[0])); }
#line 3754 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 242:
#line 891 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::PathExpression(*(yyvsp[0].id)); }
#line 3760 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 243:
#line 892 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::PathExpression(new IR::Path((yyvsp[-1].PathPrefix), *(yyvsp[0].id))); }
#line 3766 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 244:
#line 893 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::ArrayIndex((yylsp[-3]) + (yylsp[0]), (yyvsp[-3].Expression), (yyvsp[-1].Expression)); }
#line 3772 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 245:
#line 894 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Slice((yylsp[-5]) + (yylsp[0]), (yyvsp[-5].Expression), (yyvsp[-3].Expression), (yyvsp[-1].Expression)); }
#line 3778 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 246:
#line 895 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::ListExpression((yylsp[-2]) + (yylsp[0]), (yyvsp[-1].exprs)); }
#line 3784 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 247:
#line 896 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[-1].Expression); }
#line 3790 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 248:
#line 897 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::LNot((yylsp[-1]) + (yylsp[0]), (yyvsp[0].Expression)); }
#line 3796 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 249:
#line 898 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Cmpl((yylsp[-1]) + (yylsp[0]), (yyvsp[0].Expression)); }
#line 3802 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 250:
#line 899 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Neg((yylsp[-1]) + (yylsp[0]), (yyvsp[0].Expression)); }
#line 3808 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 251:
#line 900 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = (yyvsp[0].Expression); }
#line 3814 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 252:
#line 901 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Member((yylsp[-2]) + (yylsp[0]),
                                                               new IR::TypeNameExpression((yylsp[-2]), (yyvsp[-2].Type_Name)),
                                                               *(yyvsp[0].id)); }
#line 3822 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 253:
#line 904 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Member((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), *(yyvsp[0].id)); }
#line 3828 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 254:
#line 905 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Mul((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3834 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 255:
#line 906 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Div((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3840 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 256:
#line 907 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Mod((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3846 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 257:
#line 908 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Add((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3852 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 258:
#line 909 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Sub((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3858 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 259:
#line 910 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Shl((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3864 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 260:
#line 911 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { checkShift((yylsp[-2]), (yylsp[-1])); (yyval.Expression) = new IR::Shr((yylsp[-3]) + (yylsp[0]), (yyvsp[-3].Expression), (yyvsp[0].Expression)); }
#line 3870 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 261:
#line 912 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Leq((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3876 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 262:
#line 913 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Geq((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3882 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 263:
#line 914 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Lss((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3888 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 264:
#line 915 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Grt((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3894 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 265:
#line 916 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Neq((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3900 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 266:
#line 917 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Equ((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3906 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 267:
#line 918 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::BAnd((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3912 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 268:
#line 919 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::BXor((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3918 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 269:
#line 920 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::BOr((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3924 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 270:
#line 921 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Concat((yylsp[-2]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3930 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 271:
#line 922 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::LAnd((yylsp[-2]) + (yylsp[-1]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3936 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 272:
#line 923 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::LOr((yylsp[-2]) + (yylsp[-1]) + (yylsp[0]), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3942 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 273:
#line 924 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Mux((yylsp[-4]) + (yylsp[0]), (yyvsp[-4].Expression), (yyvsp[-2].Expression), (yyvsp[0].Expression)); }
#line 3948 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 274:
#line 926 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::MethodCallExpression((yylsp[-6]) + (yylsp[-3]), (yyvsp[-6].Expression), (yyvsp[-4].types), (yyvsp[-1].exprs)); }
#line 3954 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 275:
#line 929 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::MethodCallExpression((yylsp[-3]) + (yylsp[0]), (yyvsp[-3].Expression),
                                                                  new IR::Vector<IR::Type>(), (yyvsp[-1].exprs)); }
#line 3961 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 276:
#line 931 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::ConstructorCallExpression((yylsp[-3]) + (yylsp[0]),
                                                                                  (yyvsp[-3].TypePtr), (yyvsp[-1].exprs)); }
#line 3968 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;

  case 277:
#line 933 "../frontends/p4/p4-parse.ypp" /* yacc.c:1646  */
    { (yyval.Expression) = new IR::Cast((yylsp[-3]) + (yylsp[0]), (yyvsp[-2].TypePtr), (yyvsp[0].Expression)); }
#line 3974 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
    break;


#line 3978 "../frontends/p4/p4-parse.cpp" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 938 "../frontends/p4/p4-parse.ypp" /* yacc.c:1906  */



#include "p4-lex.c"
}  // end anonymous namespace

static bool parsing = false;

void yyerror(const char *fmt, ...) {
    if (!strcmp(fmt, "syntax error, unexpected IDENTIFIER")) {
        ErrorReporter::instance.parser_error("syntax error, unexpected IDENTIFIER \"%s\"",
                                             yylval.str.c_str());
        return;
    }
    va_list args;
    va_start(args, fmt);
    ErrorReporter::instance.parser_error(fmt, args);
    va_end(args);
}

const IR::P4Program *parse_P4_16_file(const char *name, FILE *in) {
    extern int verbose;
    if (verbose)
        std::cout << "Parsing P4-16 program " << name << std::endl;

    int errors = 0;
#ifdef YYDEBUG
    if (const char *p = getenv("YYDEBUG"))
        yydebug = atoi(p);
    structure.setDebug(yydebug != 0);
#endif
    declarations = new IR::IndexedVector<IR::Node>();
    parsing = true;
    yyrestart(in);
    errors |= yyparse();
    parsing = false;
    if (errors) {
        return nullptr;
    } else {
        structure.endParse();
    }
    return new IR::P4Program(declarations->srcInfo, declarations);
}

// check that right shift >>
// has the two tokens at consecutive positions
// (not separated by anything else)
void checkShift(Util::SourceInfo l, Util::SourceInfo r)
{
    if (!l.isValid() || !r.isValid())
        BUG("Source position not available!");
    const Util::SourcePosition &f = l.getStart();
    const Util::SourcePosition &s = r.getStart();
    if (f.getLineNumber() != s.getLineNumber() ||
        f.getColumnNumber() != s.getColumnNumber() - 1)
        ::error("Syntax error at shift operator: %1%", l);
}
