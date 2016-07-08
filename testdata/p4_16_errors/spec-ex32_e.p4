/*
Copyright 2016 VMware, Inc.

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
header Hdr {
    varbit<256> data0;

    @length(data0)      // illegal: expression is not uint<32>
    varbit<12> data1;

    @length(size2)     // illegal: cannot use size2, defined after data2
    varbit<256> data2;

    int<32> size2;
}
