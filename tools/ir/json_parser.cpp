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

#include "ir/json_parser.h"

#include <iostream>

int JsonObject::get_id() const {
    if (find("Node_ID") == end())
        return -1;
    else
        return *(find("Node_ID")->second->to<JsonNumber>());
}

std::string JsonObject::get_type() const {
    if (find("Node_Type") == end())
        return "";
    else
        return *(dynamic_cast<JsonString*>(find("Node_Type")->second));
}

std::string JsonObject::get_filename() const {
    if (find("filename") == end())
        return "";
    else
        return *(dynamic_cast<JsonString*>(find("filename")->second));
}

std::string JsonObject::get_sourceFragment() const {
    if (find("source_fragment") == end())
        return "";
    else
        return *(dynamic_cast<JsonString*>(find("source_fragment")->second));
}

int JsonObject::get_line() const {
    if (find("line") == end())
        return -1;
    else
        return *(find("line")->second->to<JsonNumber>());
}

int JsonObject::get_column() const {
    if (find("column") == end())
        return -1;
    else
        return *(find("column")->second->to<JsonNumber>());
}

JsonObject JsonObject::get_sourceJson() const {
    if (find("Source_Info") == end()) {
        JsonObject obj;
        obj.setSrcInfo(false);
        return obj;
    } else {
        return *(dynamic_cast<JsonObject*>(find("Source_Info")->second));
    }
}

// Hack to make << operator work multi-threaded
static thread_local int level = 0;

std::string getIndent(int l) {
    std::stringstream ss;
    for (int i = 0; i < l*4; i++) ss << " ";
    return ss.str();
}

std::ostream& operator<<(std::ostream &out, JsonData* json) {
    if (dynamic_cast<JsonObject*>(json)) {
        auto obj = dynamic_cast<ordered_map<std::string, JsonData*>*>(json);
        out << "{";
        if (obj->size() > 0) {
            level++;
            out << std::endl;
            for (auto &e : *obj)
                out << getIndent(level) << e.first << " : " << e.second << "," << std::endl;
            out << getIndent(--level);
        }
        out << "}";
    } else if (dynamic_cast<JsonVector*>(json)) {
        std::vector<JsonData*>* vec = dynamic_cast<std::vector<JsonData*>*>(json);
        out << "[";
        if (vec->size() > 0) {
            level++;
            out << std::endl;
            for (auto &e : *vec) {
                out << getIndent(level) << e << "," << std::endl;
            }
            out << getIndent(--level);
        }
        out << "]";
    } else if (dynamic_cast<JsonString*>(json)) {
        JsonString* s = dynamic_cast<JsonString*>(json);
        out << "\"" << s->c_str() << "\"";

    } else if (dynamic_cast<JsonNumber*>(json)) {
        JsonNumber* num = dynamic_cast<JsonNumber*>(json);
        out << num->val;

    } else if (dynamic_cast<JsonBoolean*>(json)) {
        JsonBoolean *b = dynamic_cast<JsonBoolean*>(json);
        out << (b->val ? "true" : "false");
    } else if (dynamic_cast<JsonNull*>(json)) {
        out << "null";
    }
    return out;
}


std::istream& operator>>(std::istream &in, JsonData*& json) {
    while (in) {
        char ch;
        in >> ch;
        switch (ch) {
        case '{': {
            ordered_map<std::string, JsonData*> obj;
            do {
                in >> std::ws >> ch;
                if (ch == '}')
                    break;
                in.unget();

                JsonData *key, *val;
                in >> key >> std::ws >> ch >> std::ws >> val;
                obj[*(dynamic_cast<std::string*>(key))] = val;

                in >> std::ws >> ch;
            } while (in && ch != '}');

            json = new JsonObject(obj);
            return in;
        }
        case '[': {
            std::vector<JsonData*> vec;
            do {
                in >> std::ws >> ch;
                if (ch == ']')
                    break;
                in.unget();

                JsonData *elem;
                in >> elem;
                vec.push_back(elem);

                in >> std::ws >> ch;
            } while (in && ch != ']');

            json = new JsonVector(vec);
            return in;
        }
        case '"': {
            std::string s;
            getline(in, s, '"');
            while (!s.empty() && s.back() == '\\') {
                int bscount = 0;  // odd number of '\' chars mean the quote is escaped
                for (auto t = s.rbegin(); t != s.rend() && *t == '\\'; ++t) bscount++;
                if ((bscount & 1) == 0) break;
                s += '"';
                std::string more;
                getline(in, more, '"');
                s += more; }
            json = new JsonString(s);
            return in;
        }
        case '-': case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': case '8': case '9': {
            mpz_class num;
            in.unget();
            in >> num;
            json = new JsonNumber(num);
            return in;
        }
        case 't': case 'T':
            in.ignore(3);
            json = new JsonBoolean(true);
            return in;
        case 'f': case 'F':
            in.ignore(4);
            json = new JsonBoolean(false);
            return in;
        case 'n': case 'N':
            in.ignore(3);
            json = new JsonNull();
            return in;
        default:
            return in;
        }
    }
    return in;
}
