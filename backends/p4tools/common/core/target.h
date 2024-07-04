#ifndef BACKENDS_P4TOOLS_COMMON_CORE_TARGET_H_
#define BACKENDS_P4TOOLS_COMMON_CORE_TARGET_H_

#include <cstdlib>
#include <map>
#include <optional>
#include <string>

#include "ir/ir.h"
#include "lib/compile_context.h"
#include "lib/exceptions.h"

namespace P4::P4Tools {

/// Encapsulates the details of a target device and architecture for a single tool. Implementations
/// are singletons and must provide a default constructor. Instances are automatically registered
/// into a central registry. The active target is set by calling @init. The suite of instances for
/// all supported tools for the active target can be obtained by calling @get.
class Target {
 public:
    /// Specifies a target device and architecture by their names in lower case.
    struct Spec {
        std::string deviceName;
        std::string archName;

        /// Names provided to this constructor are converted to lower case.
        Spec(std::string_view deviceName, std::string_view archName);

        /// Lexicographic ordering on (deviceName, archName).
        bool operator<(const Spec &) const;
    };

    /// Initializes the global target device and architecture to @deviceName and @archName.
    ///
    /// @returns true on success. If initialization fails, false is returned, and nothing is
    /// changed.
    static bool init(std::string_view deviceName, std::string_view archName);

    /// Initializes the global target device to @deviceName without changing the architecture. If
    /// no architecture was previously selected, then the first architecture registered for the
    /// device is chosen.
    ///
    /// @returns true on success. If initialization fails, false is returned, and nothing is
    /// changed.
    static bool setDevice(std::string_view deviceName);

    /// Initializes the global target architecture to @archName without changing the device. If no
    /// device was previously selected, then the first device registered for the architecture is
    /// chosen.
    ///
    /// @returns true on success. If initialization fails, false is returned, and nothing is
    /// changed.
    static bool setArch(std::string_view archName);

    /// The name of the tool supported by this instance.
    std::string toolName;

    /// The device and architecture supported by this instance.
    Spec spec;

    // A virtual destructor makes this class polymorphic, so that the dynamic cast in get() can
    // work.
    virtual ~Target() = default;

    /// @returns the default value for uninitialized variables for this particular target. This can
    /// be a taint variable or simply 0 (bits) or false (booleans).
    /// If @param forceTaint is active, this function always returns a taint variable.
    /// Can be overridden by sub-targets.
    virtual const IR::Expression *createTargetUninitialized(const IR::Type *type,
                                                            bool forceTaint) const;

    /// Initializes the global target device and architecture to @deviceName and @archName.
    /// Returns 0 on success. If initialization fails, returns -1.
    static std::optional<ICompileContext *> initializeTarget(std::string_view toolName,
                                                             const std::vector<const char *> &args);
    static std::optional<ICompileContext *> initializeTarget(std::string_view toolName,
                                                             std::string_view target,
                                                             std::string_view arch);

 protected:
    /// Creates and registers a new Target instance for the given @toolName, @deviceName, and
    /// @archName.
    Target(std::string_view toolName, const std::string &deviceName, const std::string &archName);

    /// @returns a new compilation context for the compiler.
    [[nodiscard]] virtual ICompileContext *makeContext() const = 0;

    /// @returns the target instance for the given tool and active target, as selected by @init.
    //
    // Implemented here because of limitations of templates.
    template <class TargetImpl>
    static const TargetImpl &get(std::string_view toolName) {
        if (curTarget == std::nullopt) {
            FATAL_ERROR(
                "Target not initialized. Please provide a target using the --target option.");
        }

        const auto &instances = registry.at(*curTarget);
        auto instance = instances.find(toolName);
        BUG_CHECK(instance != instances.end(),
                  "Architecture %1% on device %2% not supported for %3%", curTarget->archName,
                  curTarget->deviceName, toolName);

        const auto *casted = dynamic_cast<const TargetImpl *>(instance->second);
        BUG_CHECK(casted, "%1%/%2% implementation for %3% has wrong type", curTarget->deviceName,
                  curTarget->archName, toolName);
        return *casted;
    }

 private:
    /// The active target, if any.
    static std::optional<Spec> curTarget;

    /// Maps supported target specs to Target implementations for each supported tool.
    static std::map<Spec, std::map<std::string, const Target *, std::less<>>> registry;

    /// Maps the name of the first architecture registered for each device name.
    static std::map<std::string, std::string, std::less<>> defaultArchByDevice;

    /// Maps the name of the first device registered for each architecture name.
    static std::map<std::string, std::string, std::less<>> defaultDeviceByArch;
};

}  // namespace P4::P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_CORE_TARGET_H_ */
