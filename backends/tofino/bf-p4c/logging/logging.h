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
