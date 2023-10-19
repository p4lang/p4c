#include "error_message.h"

std::string ErrorMessage::getPrefix() const {
    std::string p = prefix;
    if (type == MessageType::Error) {
        if (p.empty())
            p = "error: ";
        else
            p = "[--Werror=" + p + "] error: ";
    } else if (type == MessageType::Warning) {
        if (p.empty())
            p = "warning: ";
        else
            p = "[--Wwarn=" + p + "] warning: ";
    } else if (type == MessageType::Info) {
        if (p.empty())
            p = "info: ";
        else
            p = "[--Winfo=" + p + "] info: ";
    }
    return p;
}

std::string ErrorMessage::toString() const {
    std::string result = "";
    std::string mainFragment = "";
    if (!locations.empty() && locations.front().isValid()) {
        result += locations.front().toPositionString() + ": ";
        mainFragment = locations.front().toSourceFragment();
    }

    result += getPrefix() + message + "\n" + mainFragment;

    for (unsigned i = 1; i < locations.size(); i++) {
        if (locations[i].isValid())
            result += locations[i].toPositionString() + "\n" + locations[i].toSourceFragment();
    }

    result += suffix;

    return result;
}

std::string ParserErrorMessage::toString() const {
    return std::string(location.toPositionString().c_str()) + ":" + message + "\n" +
           location.toSourceFragment().c_str();
}
