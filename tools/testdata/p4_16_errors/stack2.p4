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
header h {}

control p()
{
    apply {
        h[5] stack1;
        h[3] stack2;

        stack1 = stack2;  // assignment between different stacks
        stack1.lastIndex = 3;  // not an l-value
        stack2.size = 4;  // not an l-value
    }
}
