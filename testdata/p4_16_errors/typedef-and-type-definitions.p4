/*
Copyright 2019 Cisco Systems, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <core.p4>


header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f2;
}

header_union hu1_t {
    h1_t h1;
    h2_t h2;
}

struct s1_t {
    bit<8> f2;
}

enum enum1_t {
    A,
    B
}

enum bit<7> serializable_enum1_t {
    C = 3,
    D = 8
}

// Naming convention used here:

// D is abbreviation for `typedef`
// T is abbrevation for `type`

// Since types and typedefs can be defined in terms of each other, the
// names I use here contain sequences of Ds and Ts to indicate the
// order in which they have been "stacked", e.g. EthDT_t is a `type`
// (the T is last) defined on type of a `typedef` (the D just before
// the T).


//////////////////////////////////////////////////////////////////////
// typdef
//////////////////////////////////////////////////////////////////////

// void, match_kind cause syntax error that causes compiler to
// stop immediately without looking for further errors.

// syntax error, unexpected VOID, expecting ENUM or HEADER or HEADER_UNION or STRUCT
//typedef void       voidD_t;
//syntax error, unexpected MATCH_KIND, expecting ENUM or HEADER or HEADER_UNION or STRUCT
//typedef match_kind matchD_t;

// The compiler allows all of the typedef definitions below, without
// error.


typedef int        intD_t;
typedef bit<8>     bD_t;
typedef int<8>     iD_t;
typedef varbit<8>  vD_t;
typedef error      errorD_t;
typedef bool       boolD_t;
typedef enum1_t    enum1D_t;
typedef serializable_enum1_t serializable_enum1D_t;
typedef h1_t       H1D_t;
typedef header h2a_t { bit<16> f2; bit<8> f3; } H2aD_t;
typedef h1_t[3]    h1StackD_t;
typedef hu1_t      hu1D_t;
typedef s1_t       s1D_t;
typedef struct PointA_t { int<32> x; int<32> y; } PointAD_t;
typedef tuple<bit<8>, bit<17> > Tuple1D_t;
typedef tuple<bit<8>, varbit<17> > Tuple2D_t;


//////////////////////////////////////////////////////////////////////
// type
//////////////////////////////////////////////////////////////////////

// void, match_kind cause syntax error that causes compiler to
// stop immediately without looking for further errors.

// syntax error, unexpected VOID, expecting ENUM or HEADER or HEADER_UNION or STRUCT
//type    void       voidT_t;
// syntax error, unexpected MATCH_KIND, expecting ENUM or HEADER or HEADER_UNION or STRUCT
//type    match_kind matchT_t;


// Every line marked '// error' below causes the compiler to issue an
// error message of this form:

// error: <type_name>: 'type' can only be applied to base types


type    int        intT_t;
type    bit<8>     bT_t;
type    int<8>     iT_t;
type    varbit<8>  vT_t;  // error
type    error      errorT_t;  // error
type    bool       boolT_t;
type    enum1_t    enum1T_t;  // error
type    serializable_enum1_t serializable_enum1T_t;  // error
type    h1_t       H1T_t;  // error
type    header h2b_t { bit<16> f2; bit<8> f3; } H2bT_t;  // error
type    h1_t[3]    h1StackT_t;  // error
type    hu1_t      hu1T_t;  // error
type    s1_t       s1T_t;  // error
type    struct PointB_t { int<32> x; int<32> y; } PointBD_t;  // error
type    tuple<bit<8>, bit<17> > Tuple1T_t;  // error
type    tuple<bit<8>, varbit<17> > Tuple2T_t;  // error


// typedef on top of another typedef or type are allowed
typedef bD_t bDD_t;
typedef bT_t bTD_t;

// type on top of another typedef or type are allowed, at least if the
// base type is allowed directly in a type definition.

type    bD_t bDT_t;
type    bT_t bTT_t;

// Every line marked '// error' below causes the compiler to issue an
// error message of this form:

// error: <type_name>: 'type' can only be applied to base types

type    iD_t iDT_t;
type    vD_t vDT_t;  // error
type    errorD_t errorDT_t;  // error
type    boolD_t boolDT_t;
type    enum1D_t enum1DT_t;  // error
type    serializable_enum1D_t serializable_enum1DT_t;  // error
type    H1D_t H1DT_t;  // error
type    H2aD_t H2aDT_t;  // error
type    h1StackD_t h1StackDT_t;  // error
type    hu1D_t hu1DT_t;  // error
type    s1D_t s1DT_t;  // error
type    PointAD_t PointADT_t;  // error
type    Tuple1D_t Tuple1DT_t;  // error
type    Tuple2D_t Tuple2DT_t;  // error
