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

#ifndef _BACKENDS_TOFINO_BF_P4C_LOGGING_LOGGING_H_
#define _BACKENDS_TOFINO_BF_P4C_LOGGING_LOGGING_H_

#include <string.h>
#include <time.h>

#include <cstdarg>
#include <fstream>

#include "bf-p4c/common/run_id.h"
#include "rapidjson_adapter.h"

namespace Logging {
/// Define the levels of logging. All messages above the set level for logger are logged.
enum LogLevel_t { LOG, DEBUG, INFO, WARNING, ERROR, CRITICAL };

/// Base class for logging to a file
class Logger : public rapidjson::Document {
 private:
    LogLevel_t _level;
    std::ofstream _logFile;

 public:
    explicit Logger(const char *filename, LogLevel_t level = INFO)
        : rapidjson::Document(rapidjson::Type::kObjectType), _level(level) {
        _logFile.open(filename, std::ofstream::out);
    }
    ~Logger() {
        _logFile.flush();
        _logFile.close();
    }

    void setLevel(LogLevel_t level) { _level = level; }
    virtual void serialize(Writer &) const = 0;
    virtual void log() {
        rapidjson::StringBuffer sb;
        PrettyWriterAdapter writerAdapter(sb);
        serialize(writerAdapter);
        _logFile << sb.GetString();
        _logFile.flush();
    }

    static const std::string buildDate(void) {
        const time_t now = time(NULL);
        struct tm tmp;
        if (localtime_r(&now, &tmp) == NULL) {
            throw std::runtime_error("Error calling localtime_r: " + std::string(strerror(errno)));
        }

        char bdate[1024];
        strftime(bdate, 1024, "%c", &tmp);
        return bdate;
    }
};
}  // end namespace Logging

#endif /* _BACKENDS_TOFINO_BF_P4C_LOGGING_LOGGING_H_ */
