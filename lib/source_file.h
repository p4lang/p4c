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

/* -*-c++-*- */

/* Source-level information for a P4 program */

#ifndef LIB_SOURCE_FILE_H_
#define LIB_SOURCE_FILE_H_

#include <map>
#include <sstream>
#include <string_view>
#include <vector>

#include "cstring.h"
#include "stringify.h"

// GTest
#ifdef P4C_GTEST_ENABLED
#include "gtest/gtest_prod.h"
#endif

namespace P4::Test {
class UtilSourceFile;
}

namespace P4::Util {
using namespace P4::literals;

struct SourceFileLine;
/**
A character position within some InputSources: a pair of
line/column positions.  Can only be interpreted in the context
of some InputSources.

In files line numbering starts at 1, so 0 is reserved for
"invalid" source positions.  As a consequence, invalid source
position are the "smallest", which is a reasonable choice.
*/
class SourcePosition final {
 public:
    /// Creates an invalid source position
    SourcePosition() = default;

    SourcePosition(unsigned lineNumber, unsigned columnNumber);

    SourcePosition(const SourcePosition &other) = default;
    SourcePosition &operator=(const SourcePosition &) = default;

    inline bool operator==(const SourcePosition &rhs) const {
        return columnNumber == rhs.columnNumber && lineNumber == rhs.lineNumber;
    }
    inline bool operator!=(const SourcePosition &rhs) const { return !this->operator==(rhs); }

    inline bool operator<(const SourcePosition &rhs) const {
        return (lineNumber < rhs.lineNumber) ||
               (lineNumber == rhs.lineNumber && columnNumber < rhs.columnNumber);
    }
    inline bool operator>(const SourcePosition &rhs) const { return rhs.operator<(*this); }
    inline bool operator<=(const SourcePosition &rhs) const { return !this->operator>(rhs); }
    inline bool operator>=(const SourcePosition &rhs) const { return !this->operator<(rhs); }

    /// Move one column back.  This never moves one line back.
    SourcePosition &operator--() {
        if (columnNumber > 0) columnNumber--;
        return *this;
    }
    SourcePosition operator--(int) {
        SourcePosition tmp(*this);
        this->operator--();
        return tmp;
    }

    inline const SourcePosition &min(const SourcePosition &rhs) const {
        if (this->operator<(rhs)) return *this;
        return rhs;
    }

    inline const SourcePosition &max(const SourcePosition &rhs) const {
        if (this->operator>(rhs)) return *this;
        return rhs;
    }

    cstring toString() const;

    bool isValid() const { return lineNumber != 0; }

    unsigned getLineNumber() const { return lineNumber; }

    unsigned getColumnNumber() const { return columnNumber; }

 private:
    // Input sources where this character position is interpreted.
    unsigned lineNumber = 0;
    unsigned columnNumber = 0;
};

class InputSources;

/**
Information about the source position of a language element -
a range of position within an InputSources.   Can only be
interpreted relative to some InputSources.

For a program element, the start is inclusive and the end is
exclusive (the first position after the language element).

SourceInfo can also be "invalid"
*/
class SourceInfo final {
 public:
    cstring filename = ""_cs;
    int line = -1;
    int column = -1;
    cstring srcBrief = ""_cs;
    SourceInfo(cstring filename, int line, int column, cstring srcBrief) {
        this->filename = filename;
        this->line = line;
        this->column = column;
        this->srcBrief = srcBrief;
    }
    /// Creates an "invalid" SourceInfo
    SourceInfo() = default;

    /// Creates a SourceInfo for a 'point' in the source, or invalid
    SourceInfo(const InputSources *sources, SourcePosition point)
        : sources(sources), start(point), end(point) {}

    SourceInfo(const InputSources *sources, SourcePosition start, SourcePosition end);

    SourceInfo(const SourceInfo &other) = default;
    SourceInfo &operator=(const SourceInfo &other) = default;
    ~SourceInfo() = default;

    /**
        A SourceInfo that spans both this and rhs.
        However, if this or rhs is invalid, it is not taken into account */
    SourceInfo operator+(const SourceInfo &rhs) const {
        if (!this->isValid()) return rhs;
        if (!rhs.isValid()) return *this;
        SourcePosition s = start.min(rhs.start);
        SourcePosition e = end.max(rhs.end);
        return SourceInfo(sources, s, e);
    }
    SourceInfo &operator+=(const SourceInfo &rhs) {
        if (!isValid()) {
            *this = rhs;
        } else if (rhs.isValid()) {
            start = start.min(rhs.start);
            end = end.max(rhs.end);
        }
        return *this;
    }

    bool operator==(const SourceInfo &rhs) const { return start == rhs.start && end == rhs.end; }

    cstring toString() const;

    void dbprint(std::ostream &out) const { out << this->toString(); }

    /**
        Create a string with a line of source, optionally with carets on the following line
        marking the spot of this SourceInfo.  If trimWidth is >= 10, trim the line to be
        at most trimWidth characters, as too-long lines are unreadable.  trimWidth = -1
        defaults to 0 (disable) if useMarker is false or the COLUMNS envvar or 100 if
        useMarker is true */
    cstring toSourceFragment(int trimWidth = -1, bool useMarker = true) const;
    cstring toSourceFragment(bool useMarker) const { return toSourceFragment(-1, useMarker); }
    cstring toBriefSourceFragment() const;
    cstring toPositionString() const;
    cstring toSourcePositionData(unsigned *outLineNumber, unsigned *outColumnNumber) const;
    SourceFileLine toPosition() const;

    bool isValid() const { return this->start.isValid(); }
    explicit operator bool() const { return isValid(); }

    cstring getSourceFile() const;
    cstring getLineNum() const;

    const SourcePosition &getStart() const { return this->start; }

    const SourcePosition &getEnd() const { return this->end; }

    /**
       True if this comes 'before' this source position.
       'invalid' source positions come first.
       This is true if the start of other is strictly before
       the start of this. */
    bool operator<(const SourceInfo &rhs) const {
        if (!rhs.isValid()) return false;
        if (!isValid()) return true;
        return this->start < rhs.start;
    }
    inline bool operator>(const SourceInfo &rhs) const { return rhs.operator<(*this); }
    inline bool operator<=(const SourceInfo &rhs) const { return !this->operator>(rhs); }
    inline bool operator>=(const SourceInfo &rhs) const { return !this->operator<(rhs); }

    friend std::ostream &operator<<(std::ostream &os, const SourceInfo &info);

 private:
    const InputSources *sources = nullptr;
    SourcePosition start = SourcePosition();
    SourcePosition end = SourcePosition();
};

class IHasSourceInfo {
 public:
    virtual SourceInfo getSourceInfo() const = 0;
    virtual cstring toString() const = 0;
    virtual ~IHasSourceInfo() {}
};

/// SFINAE helper to check if given class has a `getSourceInfo` method.
template <class, class = void>
struct has_SourceInfo : std::false_type {};

template <class T>
struct has_SourceInfo<T, std::void_t<decltype(std::declval<T>().getSourceInfo())>>
    : std::true_type {};

template <class T>
inline constexpr bool has_SourceInfo_v = has_SourceInfo<T>::value;

/** A line in a source file */
struct SourceFileLine {
    /// an empty filename indicates stdin
    cstring fileName;
    unsigned sourceLine;

    SourceFileLine(std::string_view file, unsigned line) : fileName(file), sourceLine(line) {}

    cstring toString() const;
};

class Comment final : IHasDbPrint {
 private:
    SourceInfo srcInfo;
    bool singleLine;
    cstring body;

 public:
    Comment(SourceInfo srcInfo, bool singleLine, cstring body)
        : srcInfo(srcInfo), singleLine(singleLine), body(body) {}
    cstring toString() const {
        std::stringstream str;
        dbprint(str);
        return str.str();
    }
    void dbprint(std::ostream &out) const override {
        if (singleLine)
            out << "//";
        else
            out << "/*";
        out << body;
        if (!singleLine) out << "*/";
    }
};

/**
  Information about all the input sources that comprise a P4 program that is being compiled.
  The inputSources can be seen as a simple file produced by the preprocessor,
  but also as a set of fragments of input files which were stitched by the preprocessor.

  The mutable part of the API is tailored for interaction with the lexer.
  After the lexer is done this object can be "sealed" and never changes again.

  This class implements a singleton pattern: there is a single instance of this class.
*/
class InputSources final {
#ifdef P4C_GTEST_ENABLED
    FRIEND_TEST(UtilSourceFile, InputSources);
#endif

 public:
    InputSources();

    std::string_view getLine(unsigned lineNumber) const;
    /// Original source line that produced the line with the specified number
    SourceFileLine getSourceLine(unsigned line) const;

    unsigned lineCount() const;
    SourcePosition getCurrentPosition() const;
    unsigned getCurrentLineNumber() const;

    /// Prevents further changes; currently not used.
    void seal();

    /// Append this text; it is either a newline or a text with no newlines.
    void appendText(const char *text);

    /**
        Map the next line in the file to the line with number 'originalSourceLine'
        from file 'file'. */
    void mapLine(std::string_view file, unsigned originalSourceLineNo);

    /**
       The following return a nice (multi-line, newline-terminated)
       string describing a position in the sources, e.g.:
       int<32> variable;
               ^^^^^^^^ */
    cstring getSourceFragment(const SourcePosition &position, int trimWidth, bool useMarker) const;
    cstring getSourceFragment(const SourceInfo &position, int trimWidth, bool useMarker) const;
    cstring getBriefSourceFragment(const SourceInfo &position) const;

    cstring toDebugString() const;
    void addComment(SourceInfo srcInfo, bool singleLine, cstring body);

 private:
    /// Append this text to the last line; must not contain newlines
    void appendToLastLine(std::string_view text);
    /// Append a newline and start a new line
    void appendNewline(std::string_view newline);

    /// Input program that is being currently compiled; there can be only one.
    bool sealed;

    std::map<unsigned, SourceFileLine> line_file_map;

    /// Each line also stores the end-of-line character(s)
    std::vector<std::string> contents;
    /// The commends found in the file.
    std::vector<Comment *> comments;
};

}  // namespace P4::Util

namespace P4 {

void dbprint(const IHasDbPrint *o);

}  // namespace P4

#endif /* LIB_SOURCE_FILE_H_ */
