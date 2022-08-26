#ifndef TESTGEN_TARGETS_BMV2_BACKEND_STF_STF_H_
#define TESTGEN_TARGETS_BMV2_BACKEND_STF_STF_H_

#include <cstddef>
#include <fstream>
#include <map>
#include <vector>

#include <boost/optional/optional.hpp>

#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/testgen/backend/tf.h"
#include "backends/p4tools/testgen/lib/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/// Encapsulates the STF commands and assembles the arguments to be printed to the STF File.
class STFTest {
 public:
    /// Base command class for all STF commands to inherit from.
    class Command {
     public:
        virtual cstring getCommandName() const = 0;

        /// The default implementation prints the command name. When subclasses override from
        /// this base class, they should call the corresponding subclasses' implementation.
        virtual void print(std::ofstream&) const;
    };

    /// Adds an entry to a table.
    /// "add <table> <addr> {<key>:<value>} <action>({<name>:<value>}) [= <handle>]"
    class Add : public Command {
     private:
        /// Table to be programmed.
        cstring table;

        /// For a ternary table, <addr> is the priority.
        /// For an exact match table <addr> is the way to use.
        int addr;

        /// Each element is an API name paired with a match rule.
        const std::map<cstring, const FieldMatch>* matches;

        /// Action to program.
        cstring action;

        /// Action args.
        const std::vector<ActionArg>* args;

     public:
        Add(cstring table, const std::map<cstring, const FieldMatch>* matches, cstring action,
            const std::vector<ActionArg>* args, int addr);

        cstring getCommandName() const override;

        /// Assemble the "add" command.
        void print(std::ofstream&) const override;
    };

    /// Sets the default (miss) action to be executed for a table.
    /// "setdefault <table> <action>({<name>:<value>})"
    class SetDefault : public Command {
     private:
        /// Table whose default action needs to be set.
        cstring table;

        /// Default action.
        cstring action;

        /// Action args.
        const std::vector<const ActionArg>* args;

     public:
        SetDefault(cstring table, cstring action, const std::vector<const ActionArg>* args);

        cstring getCommandName() const override;

        /// Assemble the "setdefault" command.
        void print(std::ofstream&) const override;
    };

    /// Base Stats class for the stats checkers to inherit from.
    class CheckStats : public Command {
     private:
        int pipe;
        cstring table;
        int index;
        const IR::Constant* value;

     public:
        CheckStats(int pipe, cstring table, int index, const IR::Constant* value);

        /// Assemble the check stats command.
        void print(std::ofstream&) const override;
    };

    /// To read a counter (statistics) table and print or check its value.
    /// "check_counter [<pipe>:]<table>(<index>) <field> = <value>"
    class CheckCounter : public CheckStats {
     public:
        CheckCounter(int pipe, cstring table, int index, const IR::Constant* value);

        cstring getCommandName() const override;
    };

    /// To read a register (stateful) table and print or check its value.
    /// "check_register [<pipe>:]<table>(<index>) <field> = <value>"
    class CheckRegister : public CheckStats {
     public:
        CheckRegister(int pipe, cstring table, int index, const IR::Constant* value);

        cstring getCommandName() const override;
    };

    /// Base class to emit an Ingress/Egress packet.
    class Packet : public Command {
     private:
        int port;
        const IR::Constant* data;

        /// If a bit is set in the @mask, the corresponding bit in @data is ignored.
        const IR::Constant* mask;

     public:
        Packet(int port, const IR::Constant* data, const IR::Constant* mask);

        void print(std::ofstream&) const override;
    };

    /// Injects a raw packet.
    /// "packet <port> <packet data>"
    class Ingress : public Packet {
     public:
        Ingress(int port, const IR::Constant* data);

        cstring getCommandName() const override;

        void print(std::ofstream&) const override;
    };

    /// Expects an output packet on a specific port.
    /// "expect <port> <packet data>"
    class Egress : public Packet {
     public:
        Egress(int port, const IR::Constant* data, const IR::Constant* mask);

        cstring getCommandName() const override;

        void print(std::ofstream&) const override;
    };

    /// Wait for packet processing to complete.
    /// "wait" // No parameters
    class Wait : public Command {
     public:
        Wait() {}

        cstring getCommandName() const override;

        void print(std::ofstream&) const override;
    };

    /// Adds a comment.
    /// # <comment>
    class Comment : public Command {
     private:
        // Comment message
        cstring message;

     public:
        explicit Comment(cstring message);

        cstring getCommandName() const override;

        void print(std::ofstream&) const override;
    };
};

/// Extracts information from the @testSpec to assemble STF commands.
class STF : public TF {
 private:
    static void add(std::ofstream& out, const TestSpec* testSpec);
    static void packet(std::ofstream& out, const TestSpec* testSpec);
    static void expect(std::ofstream& out, const TestSpec* testSpec);
    static void wait(std::ofstream& out);
    static void comment(std::ofstream& out, cstring message);
    static void trace(std::ofstream& out, const TestSpec* testSpec);

 public:
    STF(cstring testName, boost::optional<unsigned int> seed);
    void outputTest(const TestSpec* spec, cstring selectedBranches, size_t testIdx,
                    float currentCoverage) override;
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_BACKEND_STF_STF_H_ */
