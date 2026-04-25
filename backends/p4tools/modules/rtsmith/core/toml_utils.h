#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_TOML_UTILS_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_TOML_UTILS_H_

#include <toml++/toml.hpp>

#include "backends/p4tools/modules/rtsmith/core/config.h"

namespace P4::P4Tools::RtSmith {

class TOMLUtils {
 public:
    /// @brief Override the default fuzzer configurations through the TOML file.
    /// @param fuzzConfig The fuzzer configurations.
    /// @param path The path to the TOML file.
    static void overrideFuzzerConfigs(FuzzerConfig &fuzzerConfig, std::filesystem::path path);

    /// @brief Override the default fuzzer configurations through the string representation of the
    /// configurations of format TOML.
    /// @param fuzzConfig The fuzzer configurations.
    /// @param configInString The string representation of the configurations.
    static void overrideFuzzerConfigsInString(FuzzerConfig &fuzzerConfig,
                                              std::string configInString);

    /// @brief Get the TOML node from the TOML result and cast it to a specific type of value.
    template <typename T>
    static std::optional<T> getAndCastTOMLNode(const toml::parse_result &tomlConfig,
                                               const std::string &key) {
        auto node = tomlConfig[key];
        if (!node) {
            return std::nullopt;
        }
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, uint64_t> ||
                      std::is_same_v<T, size_t>) {
            return castTOMLNode<T>(node);
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            if (auto nodeValuePtr = node.as_array()) {
                std::vector<std::string> result;
                for (const auto &element : *nodeValuePtr) {
                    if (element.is_string()) {
                        // Get the value of the string out of the `std::optional` encapsulations
                        // and push it to the `result` vector.
                        result.push_back(castTOMLNode<std::string>(element).value());
                    } else {
                        return std::nullopt;
                    }
                }
                return std::make_optional(result);
            }
        }
        return std::nullopt;
    }

    /// @brief Cast the TOML node to a specific type of value (encapsulated in
    /// `std::optional`).
    template <typename T>
    static std::optional<T> castTOMLNode(const toml::v3::node_view<const toml::v3::node> &node) {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, uint64_t> ||
                      std::is_same_v<T, size_t>) {
            if (auto nodeValuePtr = node.as_integer()) {
                return std::make_optional(nodeValuePtr->get());
            }
        }
        return std::nullopt;
    }

    /// @brief Cast the TOML node to a specific type of value (encapsulated in
    /// `std::optional`).
    /// This is an overloaded function of the `castTOMLNode` function for the
    /// `toml::v3::node`-type parameter.
    template <typename T>
    static std::optional<T> castTOMLNode(const toml::v3::node &node) {
        if constexpr (std::is_same_v<T, std::string>) {
            if (auto nodeValuePtr = node.as_string()) {
                return std::make_optional(nodeValuePtr->get());
            }
        }
        return std::nullopt;
    }
};

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_TOML_UTILS_H_ */
