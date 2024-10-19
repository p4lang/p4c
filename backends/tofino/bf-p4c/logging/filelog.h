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

#ifndef _BACKENDS_TOFINO_BF_P4C_LOGGING_FILELOG_H_
#define _BACKENDS_TOFINO_BF_P4C_LOGGING_FILELOG_H_

#include <sys/stat.h>

#include <fstream>
#include <streambuf>
#include <string>
#include <utility>

#include "bf-p4c/logging/manifest.h"
#include "lib/cstring.h"

namespace Logging {

/// \enum Mode specifies how the log file is created and/or reused.
enum Mode {
    /// Appends to a log. Creates a new log if it doesn't already exist.
    APPEND,

    /// Creates a new log. Overwrites if the log already exists.
    CREATE,

    /// Creates if this is the first time writing to the log; otherwise, appends.
    AUTO
};

/// A FileLog is used to redirect the logging output of a visitor pass to a file.

class FileLog {
 private:
    std::basic_streambuf<char> *clog_buff = nullptr;
    std::ofstream *log = nullptr;

    static const cstring &name2type(cstring logName);
    static std::set<cstring> filesWritten;

 public:
    explicit FileLog(int pipe, cstring logName, Mode mode = AUTO) {
        bool append = true;
        switch (mode) {
            case APPEND:
                append = true;
                break;
            case CREATE:
                append = false;
                break;
            case AUTO:
                append = filesWritten.count(logName);
                break;
        }

        filesWritten.insert(logName);

        auto logDir = BFNContext::get().getOutputDirectory("logs"_cs, pipe);
        if (logDir) {
            // Assuming cstring has a method to convert to std::string
            std::filesystem::path fullPath =
                std::filesystem::path(logDir.string_view()) / logName.string_view();

            // Open the file stream with the full path
            LOG1("Open logfile " << fullPath.string() << " append: " << append);
            Manifest::getManifest().addLog(pipe, name2type(logName), logName);
            clog_buff = std::clog.rdbuf();
            auto flags = std::ios_base::out;
            if (append) flags |= std::ios_base::app;
            log = new std::ofstream(fullPath, flags);
            std::clog.rdbuf(log->rdbuf());
        }
    }

    /// Closes the log using close
    ~FileLog() { close(); }

    /// Untie the file and close the filestream.
    void close() {
        if (clog_buff != nullptr) {
            std::clog.rdbuf(clog_buff);
        }
        if (log) {
            log->flush();
            log->close();
            delete log;
            log = nullptr;
        }
    }

    /// Closes a FileLog and cleans up.
    static void close(FileLog *&log) {
        if (!log) return;
        delete log;
        log = nullptr;
    }
};

}  // end namespace Logging

#endif /* _BACKENDS_TOFINO_BF_P4C_LOGGING_FILELOG_H_ */
