/*
Copyright 2013-present Barefoot Networks, Inc. 

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

header my_header {
    bit    g1;
    bit<2> g2;
    bit<3> g3;
}

typedef bit<3> Field;

header your_header
{
   Field field;
}

header_union your_union
{
    your_header h1;
}

struct str
{
     your_header hdr;
     your_union  unn;
     bit<32>     dt;
}

control p()
{
    apply {
        your_header[5] stack;
    }
}

typedef your_header[5] your_stack;
