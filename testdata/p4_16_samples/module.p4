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

package s();

parser Filter(out bool filter);

package switch0(Filter f);

parser f(out bool x)
{ state start { transition accept; } }

parser f2(out bool x, out bool y)
{ state start { transition accept; } }

parser Extractor<T>(out T dt);

parser Extractor2<T1, T2>(out T1 data1, out T2 data2);

package switch1<S>(Extractor<S> e);
package switch2(Extractor<bool> e);
package switch3(Extractor2<bool, bool> e);
package switch4<S>(Extractor2<bool, S> e);

switch0(f()) main1;
switch1<bool>(f()) main3;
switch2(f()) main4;
switch3(f2()) main5;
switch4<bool>(f2()) main6;
switch1(f()) main2;
switch4(f2()) main7;
