/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BACKENDS_TOFINO_BF_P4C_LOGGING_RAPIDJSON_ADAPTER_H_
#define _BACKENDS_TOFINO_BF_P4C_LOGGING_RAPIDJSON_ADAPTER_H_

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

namespace Logging {

/// Plain writer does not indent with whitespaces
using PlainWriter = rapidjson::Writer<rapidjson::StringBuffer>;
using PrettyWriter = rapidjson::PrettyWriter<rapidjson::StringBuffer>;

/**
 *  Since rapidjson doesn't use virtual inheritance for PrettyWriter,
 *  we need this adapter class to be able to pick between pretty and
 *  plain json writers.
 */
class Writer {
 public:
    virtual void Key(const char *s) = 0;
    virtual void Bool(bool b) = 0;
    virtual void Int(int n) = 0;
    virtual void Double(double d) = 0;
    virtual void String(const char *s) = 0;
    virtual void StartArray() = 0;
    virtual void EndArray() = 0;
    virtual void StartObject() = 0;
    virtual void EndObject() = 0;

    virtual ~Writer() {}
};

class PrettyWriterAdapter : public Writer {
    PrettyWriter w;

 public:
    virtual void Key(const char *s) { w.Key(s); }
    virtual void Bool(bool b) { w.Bool(b); }
    virtual void Int(int n) { w.Int(n); }
    virtual void Double(double d) { w.Double(d); }
    virtual void String(const char *s) { w.String(s); }
    virtual void StartArray() { w.StartArray(); }
    virtual void EndArray() { w.EndArray(); }
    virtual void StartObject() { w.StartObject(); }
    virtual void EndObject() { w.EndObject(); }

    explicit PrettyWriterAdapter(rapidjson::StringBuffer &sb) : w(sb) {}
};

class PlainWriterAdapter : public Writer {
    PlainWriter w;

 public:
    virtual void Key(const char *s) { w.Key(s); }
    virtual void Bool(bool b) { w.Bool(b); }
    virtual void Int(int n) { w.Int(n); }
    virtual void Double(double d) { w.Double(d); }
    virtual void String(const char *s) { w.String(s); }
    virtual void StartArray() { w.StartArray(); }
    virtual void EndArray() { w.EndArray(); }
    virtual void StartObject() { w.StartObject(); }
    virtual void EndObject() { w.EndObject(); }

    explicit PlainWriterAdapter(rapidjson::StringBuffer &sb) : w(sb) {}
};

}  // namespace Logging

#endif /* _BACKENDS_TOFINO_BF_P4C_LOGGING_RAPIDJSON_ADAPTER_H_ */
