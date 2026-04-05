#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_CONTROL_PLANE_PROTOBUF_UTILS_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_CONTROL_PLANE_PROTOBUF_UTILS_H_

#include <fcntl.h>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include <filesystem>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/rtsmith/core/util.h"
#include "lib/big_int.h"
#include "lib/error.h"

namespace P4::P4Tools::RtSmith::Protobuf {

/// Helper function, which converts a Protobuf byte string into a big integer
/// (boost cpp_int).
inline big_int stringToBigInt(const std::string &valueString) {
    big_int value;
    boost::multiprecision::import_bits(
        value, reinterpret_cast<const u_char *>(valueString.data()),
        reinterpret_cast<const u_char *>(valueString.data()) + valueString.size());
    return value;
}

/// Deserialize a .proto file into a P4Runtime-compliant Protobuf object.
template <class T>
[[nodiscard]] static std::optional<T> deserializeObjectFromFile(
    const std::filesystem::path &inputFile) {
    T protoObject;

    // Parse the input file into the Protobuf object.
    int fd = open(inputFile.c_str(),
                  O_RDONLY);  // NOLINT, we are forced to use open here.
    RETURN_IF_FALSE_WITH_MESSAGE(fd > 0, std::nullopt,
                                 error("Failed to open file %1%", inputFile.c_str()));
    google::protobuf::io::ZeroCopyInputStream *input =
        new google::protobuf::io::FileInputStream(fd);

    RETURN_IF_FALSE_WITH_MESSAGE(google::protobuf::TextFormat::Parse(input, &protoObject),
                                 std::nullopt,
                                 error("Failed to parse configuration \"%1%\" for file %2%",
                                       protoObject.ShortDebugString(), inputFile.c_str()));

    printFeature("p4rtsmith_protobuf", 4, "Parsed configuration: %1%", protoObject.DebugString());
    // Close the open file.
    close(fd);
    return protoObject;
}

}  // namespace P4::P4Tools::RtSmith::Protobuf

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_CONTROL_PLANE_PROTOBUF_UTILS_H_ */
