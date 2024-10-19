/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_COMMON_TABLE_PRINTER_H_
#define BF_P4C_COMMON_TABLE_PRINTER_H_

#include <iomanip>

#include "lib/exceptions.h"
#include "lib/log.h"

/**
 *  Usage:
 *
 *  TablePrinter tp(std::cout, {"name", "size", "price"});
 *
 *  tp.addRow({"americano", "small", "$2.25"});
 *  tp.addRow({"latte", "medium", "$2.65"});
 *  tp.addRow({"espresso", "single", "$2"});
 *  tp.addRow({"mocha", "dark", "$5"});
 *
 *  tp.print();
 */

class TablePrinter {
 public:
    enum Align { LEFT, CENTER, RIGHT };

    TablePrinter(std::ostream &s, std::vector<std::string> headers, Align align = RIGHT)
        : _s(s), _headers(headers), _align(align) {
        for (auto &h : headers) _colWidth.push_back(h.length());
    }

    bool empty() { return _data.empty(); }

    void addRow(std::vector<std::string> row) {
        BUG_CHECK(row.size() == _headers.size(), "row size does not match header");

        _data.push_back(row);

        for (unsigned i = 0; i < row.size(); i++) {
            if (row[i].length() > _colWidth[i]) _colWidth[i] = row[i].length();
        }
    }

    void addSep(unsigned start_col = 0) { _seps[_data.size()] = start_col; }

    void addBlank() {
        unsigned cols = _colWidth.size();
        if (cols) {
            std::vector<std::string> blank(cols, "");
            addRow(blank);
        }
    }

    void print() const {
        printSep();
        printRow(_headers);
        printSep();

        for (unsigned i = 0; i < _data.size(); i++) {
            printRow(_data.at(i));
            if (_seps.count(i + 1) && i + 1 != _data.size()) printSep(_seps.at(i + 1));
        }

        printSep();
    }

 private:
    unsigned getTableWidth() const {
        unsigned rv = 0;
        for (auto cw : _colWidth) rv += cw + cellPad;
        return rv + _headers.size() + 1;
    }

    void printSep(unsigned start_col = 0) const {
        for (unsigned i = 0; i < _colWidth.size(); i++) {
            _s << (i < start_col ? "|" : "+")
               << std::string(_colWidth.at(i) + cellPad, i >= start_col ? '-' : ' ');
        }

        _s << "+" << std::endl;
    }

    void printCell(unsigned col, std::string data) const {
        unsigned width = _colWidth.at(col);
        if (_align == LEFT) _s << std::left;
        _s << std::setw(width + cellPad);
        if (_align == CENTER) data += std::string((width + cellPad - data.length() + 1) / 2, ' ');
        _s << data;
    }

    void printRow(const std::vector<std::string> &row) const {
        for (unsigned i = 0; i < row.size(); i++) {
            _s << "|";

            printCell(i, row.at(i));

            if (i == row.size() - 1) _s << "|";
        }
        _s << std::endl;
    }

    std::ostream &_s;

    std::vector<std::vector<std::string>> _data;
    std::map<unsigned, unsigned> _seps;  // maps line numbers to starting column

    std::vector<std::string> _headers;
    std::vector<unsigned> _colWidth;

    Align _align = RIGHT;
    const unsigned cellPad = 2;
};

#endif /* BF_P4C_COMMON_TABLE_PRINTER_H_ */
