/*
* Copyright 2020, MNK Labs & Consulting
* http://mnkcg.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Compile: g++ range-to-tcam.cpp -o range_to_masks -lgmp
*/

#include "replaceParserRange.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include "../lib/gmputil.h"

bool debug = false;

using namespace std;

struct tcam_entry {
    big_int value;
    big_int mask;
};

int trailing_zeros(big_int n, int width)
{
    int zeros = 0;

    while (n > 0 && (n & 1) == 0) {
        zeros += 1;
        n >>= 1;
    }

    return (zeros < width) ? zeros : width;
}

vector<tcam_entry> range_to_tcam_entries(int width, big_int min, big_int max)
{
    vector<tcam_entry> entries;
    big_int remaining_range_size = max - min + 1;

    while (remaining_range_size > 0) {
        big_int range_size = ((big_int) 1) << trailing_zeros(min, width);

        while (range_size > remaining_range_size)
            range_size >>= 1;

        tcam_entry entry;

        entry.value = min;
        entry.mask = ~(range_size - 1) & ((((big_int) 1) << width) - 1);

        entries.push_back(entry);
        remaining_range_size -= range_size;
        min += range_size;
    }

    return entries;
}
