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

#ifndef P4C_LIB_SOURCE_FILE_H_
#define P4C_LIB_SOURCE_FILE_H_

#include <vector>

#include "gtest/gtest_prod.h"
#include "cstring.h"
#include "stringref.h"
#include "map.h"

namespace Test { class UtilSourceFile; }

class IHasDbPrint {
 public:
    virtual void dbprint(std::ostream& out) const = 0;
    void print() const;  // useful in the debugger
    virtual ~IHasDbPrint() {}
};

namespace Util {
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
    SourcePosition()
            : lineNumber(0),
              columnNumber(0) {}

    SourcePosition(unsigned lineNumber, unsigned columnNumber);

    SourcePosition(const SourcePosition& other)
            : lineNumber(other.lineNumber),
              columnNumber(other.columnNumber) {}

    inline bool operator==(const SourcePosition& rhs) const {
        return columnNumber == rhs.columnNumber &&
                lineNumber == rhs.columnNumber;
    }
    inline bool operator!=(const SourcePosition& rhs) const
    {return !this->operator==(rhs);}

    inline bool operator< (const SourcePosition& rhs) const {
        return (lineNumber < rhs.lineNumber) ||
                (lineNumber == rhs.lineNumber &&
                 columnNumber < rhs.columnNumber);
    }
    inline bool operator> (const SourcePosition& rhs) const
    {return rhs.operator< (*this);}
    inline bool operator<=(const SourcePosition& rhs) const
    {return !this->operator> (rhs);}
    inline bool operator>=(const SourcePosition& rhs) const
    {return !this->operator< (rhs);}

    /// Move one column back.  This never moves one line back.
    SourcePosition& operator--() {
        if (columnNumber > 0)
            columnNumber--;
        return *this;
    }
    SourcePosition operator--(int) {
        SourcePosition tmp(*this);
        this->operator--();
        return tmp;
    }

    inline const SourcePosition& min(const SourcePosition& rhs) const {
        if (this->operator<(rhs))
            return *this;
        return rhs;
    }

    inline const SourcePosition& max(const SourcePosition& rhs) const {
        if (this->operator>(rhs))
            return *this;
        return rhs;
    }

    cstring toString() const;

    bool isValid() const {
        return lineNumber != 0;
    }

    unsigned getLineNumber() const {
        return lineNumber;
    }

    unsigned getColumnNumber() const {
        return columnNumber;
    }

 private:
    // Input sources where this character position is interpreted.
    unsigned      lineNumber;
    unsigned      columnNumber;
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
    /// Creates an "invalid" SourceInfo
    SourceInfo()
        : sources(nullptr), start(SourcePosition()), end(SourcePosition()) {}

    /// Creates a SourceInfo for a 'point' in the source, or invalid
    SourceInfo(const InputSources* sources, SourcePosition point)
        : sources(sources), start(point), end(point) {}

    SourceInfo(const InputSources* sources, SourcePosition start,
               SourcePosition end);

    SourceInfo(const SourceInfo& other) = default;
    ~SourceInfo() = default;
    SourceInfo& operator=(const SourceInfo& other) = default;

    /**
        A SourceInfo that spans both this and rhs.
        However, if this or rhs is invalid, it is not taken into account */
    const SourceInfo operator+(const SourceInfo& rhs) const {
        if (!this->isValid())
            return rhs;
        if (!rhs.isValid())
            return *this;
        SourcePosition s = start.min(rhs.start);
        SourcePosition e = end.max(rhs.end);
        return SourceInfo(sources, s, e);
    }
    SourceInfo &operator+=(const SourceInfo& rhs) {
        if (!isValid()) {
            *this = rhs;
        } else if (rhs.isValid()) {
            start = start.min(rhs.start);
            end = end.max(rhs.end);
        }
        return *this;
    }

    bool operator==(const SourceInfo &rhs) const
    { return start == rhs.start && end == rhs.end; }

    cstring toDebugString() const;

    void dbprint(std::ostream& out) const
    { out << this->toDebugString(); }

    cstring toSourceFragment() const;
    cstring toBriefSourceFragment() const;
    cstring toPositionString() const;
    cstring toSourcePositionData(unsigned *outLineNumber,
                                 unsigned *outColumnNumber) const;
    SourceFileLine toPosition() const;

    bool isValid() const
    { return this->start.isValid(); }
    explicit operator bool() const { return isValid(); }

    cstring getSourceFile() const;

    const SourcePosition& getStart() const
    { return this->start; }

    const SourcePosition& getEnd() const
    { return this->end; }

    /**
       True if this comes 'before' this source position.
       'invalid' source positions come first.
       This is true if the start of other is strictly before
       the start of this. */
    bool operator< (const SourceInfo& rhs) const {
        if (!rhs.isValid()) return false;
        if (!isValid()) return true;
        return this->start < rhs.start;
    }
    inline bool operator> (const SourceInfo& rhs) const
    { return rhs.operator< (*this); }
    inline bool operator<=(const SourceInfo& rhs) const
    { return !this->operator> (rhs); }
    inline bool operator>=(const SourceInfo& rhs) const
    { return !this->operator< (rhs); }

 private:
    const InputSources* sources;
    SourcePosition start;
    SourcePosition end;
};

class IHasSourceInfo {
 public:
    virtual SourceInfo getSourceInfo() const = 0;
    virtual cstring toString() const = 0;
    virtual ~IHasSourceInfo() {}
};

/** A line in a source file */
struct SourceFileLine {
    /// an empty filename indicates stdin
    cstring   fileName;
    unsigned  sourceLine;

    SourceFileLine(cstring file, unsigned line) :
            fileName(file),
            sourceLine(line) {}

    cstring toString() const;
};

class Comment final : IHasDbPrint {
 private:
    SourceInfo srcInfo;
    bool singleLine;
    cstring body;

 public:
    Comment(SourceInfo srcInfo, bool singleLine, cstring body):
            srcInfo(srcInfo), singleLine(singleLine), body(body) {}
    cstring toString() const {
        cstring result;
        if (singleLine)
            result = "//";
        else
            result = "/*";
        result += body;
        if (!singleLine)
            result += "*/";
        return result;
    }
    void dbprint(std::ostream& out) const {
        out << toString();
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
    FRIEND_TEST(UtilSourceFile, InputSources);

 public:
    InputSources();

    cstring getLine(unsigned lineNumber) const;
    /// Original source line that produced the line with the specified number
    SourceFileLine getSourceLine(unsigned line) const;

    unsigned lineCount() const;
    SourcePosition getCurrentPosition() const;
    unsigned getCurrentLineNumber() const;

    /// Prevents further changes; currently not used.
    void seal();

    /// Append this text; it is either a newline or a text with no newlines.
    void appendText(const char* text);

    /**
        Map the next line in the file to the line with number 'originalSourceLine'
        from file 'file'. */
    void mapLine(cstring file, unsigned originalSourceLineNo);

    /**
       The following return a nice (multi-line, newline-terminated)
       string describing a position in the sources, e.g.:
       int<32> variable;
               ^^^^^^^^ */
    cstring getSourceFragment(const SourcePosition &position) const;
    cstring getSourceFragment(const SourceInfo &position) const;
    cstring getBriefSourceFragment(const SourceInfo &position) const;

    cstring toDebugString() const;
    void addComment(SourceInfo srcInfo, bool singleLine, cstring body);

 private:
    /// Append this text to the last line; must not contain newlines
    void appendToLastLine(StringRef text);
    /// Append a newline and start a new line
    void appendNewline(StringRef newline);

    /// Input program that is being currently compiled; there can be only one.
    bool sealed;

    std::map<unsigned, SourceFileLine> line_file_map;

    /// Each line also stores the end-of-line character(s)
    std::vector<cstring> contents;
    /// The commends found in the file.
    std::vector<Comment*> comments;
};

}  // namespace Util

inline void dbprint(const IHasDbPrint* o)
{ o->dbprint(std::cout); }

#endif /* P4C_LIB_SOURCE_FILE_H_ */
