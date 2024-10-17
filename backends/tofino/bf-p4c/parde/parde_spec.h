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

#ifndef EXTENSIONS_BF_P4C_PARDE_PARDE_SPEC_H_
#define EXTENSIONS_BF_P4C_PARDE_PARDE_SPEC_H_

#include <map>
#include <vector>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/parde/match_register.h"

using namespace P4::literals;

/**
 * \defgroup parde Parser & deparser
 * \brief Content related to parser and deparser
 *
 * # General information
 *
 * There are some basic visitors defined for parde:
 * - PardeInspector
 * - PardeModifier
 * - PardeTransform
 * - ParserInspector
 * - ParserModifier
 * - ParserTransform
 * - DeparserInspector
 * - DeparserModifier
 * - DeparserTransform
 *
 * The assembler representations for parde:
 * - Deparser
 * - BaseAsmParser
 *
 * The parde can also be affected at the beginning of \ref midend by architecture translation,
 * see \ref ArchTranslation for more information.
 *
 * # Parser
 *
 * In frontend and midend, the IR::P4Parser %IR node is used to represent a parser.
 * During architecture translation, i.e. within the BFN::ArchTranslation midend pass,
 * the Tofino parser becomes represented with the IR::BFN::TnaParser %IR node.
 *
 * In backend, after transformation within the BFN::BackendConverter post-midend pass,
 * the Tofino parser is represented with the IR::BFN::Parser %IR node.
 * Within the LowerParser pass, the parser is transformed (lowered) into the representation
 * that is closer to how HW works. For Tofino 1/2, IR::BFN::LoweredParser is produced,
 *
 * In assembler, the Tofino 1/2 parser is represented with the Parser class,
 * Specific differences of Tofino versions are implemented using template specialization.
 *
 * ## Frontend passes
 *
 * Frontend passes operate on IR::P4Parser nodes representing parsers and IR::ParserState nodes
 * representing parser states.
 *
 * Frontend passes related to parser:
 * - P4V1::Converter - Converts P4-14 program to an equivalent P4-16 program in v1model
 *   architecture.
 * - P4::ValidateParsedProgram - Performs simple semantic checks on the program.
 * - P4::EvaluatorPass - Creates blocks for all high-level constructs.
 * - P4::CreateBuiltins - Creates accept and reject states.
 * - P4::InstantiateDirectCalls - Replaces direct invocations of parsers with an instantiation
 *   followed by an invocation.
 * - P4::CheckNamedArgs - Checks that parsers cannot have optional parameters.
 * - P4::BindTypeVariables - Inserts explicit type specializations where they are missing.
 * - P4::SpecializeGenericFunctions - Specializes generic functions by substituting type
 *   parameters.
 * - P4::SpecializeGenericTypes - Specializes generic types by substituting type
 *   parameters.
 * - P4::RemoveParserIfs
 *   - P4::RemoveParserControlFlow
 *     - P4::DoRemoveParserControlFlow - Converts if statements in parsers into transitions.
 *     - P4::SimplifyControlFlow - Simplifies control flow.
 * - P4::SimplifyParsers - Removes unreachable states and collapses simple chains of states.
 * - P4::ResetHeaders - Explicitly invalidates uninitialized header variables.
 * - P4::UniqueNames - Gives unique names to various declarations.
 * - P4::MoveDeclarations - Moves local declarations to the "top".
 * - P4::MoveInitializers - Moves initializers into a new start state.
 * - P4::SideEffectOrdering
 *   - P4::DoSimplifyExpressions - Converts expressions so that each expression contains
 *     at most one side effect.
 * - P4::SimplifyDefUse
 *   - P4::DoSimplifyDefUse
 *     - P4::ProcessDefUse
 *       - P4::ComputeWriteSet - Computes the write set for each expression and statement.
 * - P4::SpecializeAll - Specializes parsers by substituting type arguments and constructor
 *   parameters.
 * - P4::RemoveDontcareArgs - Replaces don't care arguments with an unused temporary.
 * - P4::FunctionsInliner - Inlines functions.
 * - P4::MoveConstructors - Converts constructor call expressions that appear within
 *   the bodies of IR::P4Parser blocks into IR::Declaration_Instance.
 * - P4::RemoveRedundantParsers - Removes redundant parsers.
 * - P4::Inline - Performs inlining of parsers.
 * - P4::RemoveAllUnusedDeclarations - Removes unused declarations.
 * - P4::HierarchicalNames - Adjusts the \@name annotations on objects to reflect their position
 *   in the hierarchy.
 * - P4::ComputeParserCG - Builds a CallGraph of IR::ParserState nodes.
 *
 * ## Midend passes
 *
 * After midend pass BFN::ArchTranslation, the parser becomes represented with
 * the IR::BFN::TnaParser node which inherits from IR::P4Parser and contains some additional
 * useful metadata.
 * The parser states are still represented with IR::ParserState nodes.
 *
 * Generic midend passes related to parser (p4c):
 * - P4::CopyStructures
 *   - P4::RemoveAliases - Analyzes assignments between structures and introduces additional
 *     copy operations.
 * - P4::EliminateTuples - Converts tuples into structs.
 * - P4::ExpandLookahead - Expands lookahead calls.
 * - P4::LocalCopyPropagation - Does local copy propagation and dead code elimination.
 *
 * Tofino-specific midend passes related to parser (bf-p4c):
 *
 * - BFN::CopyHeaders - Converts header assignments into field assignments.
 * - ComputeDefUse - Computes defuse info in the midend.
 * - BFN::CheckVarbitAccess - Checks that varbit accesses are valid.
 * - BFN::DesugarVarbitExtract - Rewrites varbit usages to usages of multiple small headers.
 * - BFN::InitializeMirrorIOSelect - Adds mirror_io_select initialization to egress parser.
 * - BFN::ParserEnforceDepthReq - Enforces parser min/max depth requirements.
 * - P4ParserGraphs - Extends p4c's parser graph with various algorithms.
 *
 * ## Post-midend passes
 *
 * Post-midend passes convert midend representation of parser (IR::BFN::TnaParser) into
 * backend representation (IR::BFN::Parser).
 *
 * Post-midend passes related to parser:
 * - BridgedPacking and SubstitutePackedHeaders
 *   - RenameArchParams - Replaces the user-supplied parameter names with the corresponding
 *     parameter names defined in the architecture.
 *   - BFN::BackendConverter - Converts mid-end IR into back-end IR.
 *     - BFN::ExtractParser - Transforms midend parser IR::BFN::TnaParser into backend
 *       parser IR::BFN::Parser.
 *     - BFN::ProcessBackendPipe
 *       - BFN::ProcessParde - Resolves header stacks and adds shim for intrinsic metadata.
 *         - AddParserMetadata - Extends parsers to extract standard metadata.
 * - BridgedPacking
 *   - ExtractBridgeInfo
 *     - CollectBridgedFieldsUse - Finds usages (extract calls) of bridged headers.
 *
 * ## Backend passes
 *
 * In backend, the parser is represented with IR::BFN::Parser node and the parser states with
 * IR::BFN::ParserState nodes.
 * Within LowerParser pass, the parser is transformed to its lowered form represented with
 * IR::BFN::LoweredParser node and the parser states are represented with
 * IR::BFN::LoweredParserState nodes.
 * Both parser representations, before and after parser lowering, can be dumped to the dot files
 * using DumpParser pass, which is often called between the individual steps during parser
 * transformations so that these dumps can be used for debugging.
 *
 * Backend passes related to parser before parser lowering:
 * - FieldDefUse - Collects fields defuse info in backend.
 * - StackPushShims - Adds parser states to initialize the `$stkvalid` fields that are used
 *   to handle the `push_front` and `pop_front` primitives for header stacks.
 * - ParserCopyProp - Does parser copy propagation.
 * - RewriteParserMatchDefs - Looks for extracts into temporary local variables used
 *   in select statements.
 * - ResolveNegativeExtract - For extracts with negative source, i.e. source is in an earlier
 *   state, adjusts the state's shift amount so that the source is within current state's
 *   input buffer.
 * - InferPayloadOffset - Applies Tofino 2 feature of stopping the header length counter
 *   in any state.
 * - MergeParserStates - Merges a chain of states into a large state before parser lowering.
 * - CheckParserMultiWrite - Checks multiple writes to the same field on non-mutually exclusive
 *   paths.
 * - PHV_AnalysisPass
 *   - MutexOverlay
 *     - HeaderMutex
 *       - FindParserHeaderEncounterInfo - Based on dominators and post-dominators of parser
 *         states, determines which other headers have also surely been encountered if a given
 *         header has been encountered and which other headers have not been encountered if
 *         a given header has not been encountered.
 * - CheckFieldCorruption - Checks field corruption after PHV allocation is done.
 * - AdjustExtract - Adjusts extractions that extract from fields that are serialized
 *   from phv container.
 *
 * Backend passes responsible for parser lowering within LowerParser:
 * - PragmaNoInit - Adds no_init pragma to the specified fields.
 * - Parde::Lowered::LowerParserIR - Generates the lowered version of the parser IR
 *   (IR::BFN::LoweredParser) and swaps it in for the existing representation (IR::BFN::Parser).
 *   - Parde::Lowered::SplitGreedyParserStates - Checks constraints related to partial_hdr_err_proc
 *     and eventually splits the states.
 *   - Parde::Lowered::ResolveParserConstants - Resolves constants in parser.
 *   - ParserCopyProp - Does parser copy propagation.
 *   - SplitParserState - Splits parser states into multiple states to account for HW resource
 *     constraints of a single parser state.
 *   - Parde::Lowered::FindNegativeDeposits - Finds all of the states that do a checksum deposit
 *     but also do not extract/shift before doing it.
 *   - Parde::Lowered::RemoveNegativeDeposits - Updates the IR so that every checksum deposit
 *     can also shift by at least one.
 *   - AllocateParserMatchRegisters - Performs the parser match register allocation.
 *   - CollectParserInfo - Collects info about parser graph useful for graph algorithms.
 *   - Parde::Lowered::EliminateEmptyStates - Eliminates empty states.
 *   - AllocateParserChecksums - Allocates parser checksums.
 *   - Parde::Lowered::ComputeLoweredParserIR - Produces low-level, target-specific representation
 *     of the parser program.
 *   - Parde::Lowered::ReplaceParserIR - Replaces the IR::BFN::Parser node with a lowered
 *     representation produced by Parde::Lowered::ComputeLoweredParserIR pass.
 *   - Parde::Lowered::HoistCommonMatchOperations - Merges matches which can be merged after
 *     lowering but could not be before lowering.
 *   - Parde::Lowered::MergeLoweredParserStates - Merges states which can be merged after
 *     lowering but could not be before lowering.
 * - Parde::Lowered::LowerDeparserIR - Generates the lowered version of the deparser IR
 *   (for more info see sections bellow).
 * - Parde::Lowered::WarnTernaryMatchFields - Checks invalid containers participating
 *   in ternary matches.
 * - Parde::Lowered::ComputeInitZeroContainers - Computes containers that have fields relying
 *   on parser zero initialization.
 * - CollectLoweredParserInfo - Collects info about lowered parser graph useful
 *   for graph algorithms.
 * - Parde::Lowered::ComputeMultiWriteContainers - Collects all containers that are written
 *   more than once by the parser.
 * - Parde::Lowered::ComputeBufferRequirements - Computes the number of bytes which must be
 *   available for each parser match to avoid a stall.
 * - CharacterizeParser - Prints various info about parser to the log file.
 *
 * ## Parser write modes
 *
 * The parser supports several container extraction write modes. Tofino 1 provide a single
 * configuration bit per container that enables or disables multiple writes. Containers with
 * multi-writes disabled permit only one write per packet: any attempt to write multiple times
 * causes a parser error and aborts parsing. Containers with multi-write enabled are in bitwise-OR
 * mode: repeated writes to the container OR the new data with the existing container content.
 * Tofino 2 adds a second configuration bit per container that chooses between bitwise-OR and
 * clear-on-write (new data replaces existing data) when mutli-writes are enabled.
 *
 * Bitwise-or and clear-on-write only make sense in the context of multiple writes. If a container
 * only allows a single write, then there is never any existing content to OR/clear prior to a
 * write. (All containers are zeroed at the start-of-packet.)
 *
 * The compiler calculates write modes to determine how to configure each container. The three write
 * modes are:
 *
 *  - SINGLE_WRITE - Allow only a single write to the container for any given packet. The parser
 *    will produce a runtime error and will abort parsing if an attempt is made to write to this
 *    container multiple times. The P4 code can match on the multi-write error in case the user
 *    wishes to handle the error (e.g., forwarding the packet to CPU), otherwise it is dropped by
 *    the ingress parser/forwarded by the egress parser.
 *
 *  - BITWISE_OR - Allow multiple writes to the container, using bitwise-OR semantics (new data is
 *    ORed with existing data).
 *
 *  - CLEAR_ON_WRITE - Allow multiple writes to the container, using clear-on-write semantics
 *    (new data replaces existing data in the container). This is not supported on Tofino 1.
 *
 * Containers should be marked as SINGLE_WRITE whenever possible because the hardware can detect and
 * flag attempts to write to the container multiple times. This could indicate a compiler error or a
 * packet with unexpected repeated headers/more repeated headers than supported by a header stack.
 *
 * Write modes apply to containers. The compiler associates a write mode with each extract (via the
 * @p write_mode attribute) rather than each container, so the compiler must verify that all
 * extracts to the same container have the same write mode (or can be downgraded to be compatible).
 *
 * ### Choosing write modes
 *
 * Most header field extracts should be marked as SINGLE_WRITE or CLEAR_ON_WRITE.  They should be
 * marked as SINGLE_WRITE if the field is extracted no more than once on every path through the
 * parse graph, otherwise they should be marked as CLEAR_ON_WRITE if multiple extractions can occur
 * on a single path and the later extraction replaces the earlier extraction.
 *
 * BITWISE_OR should typically only be used for metadata field extracts representing state, such as
 * validity or error flags, that are written by constants. This includes narrow multi-bit fields
 * (narrower than container size) that are written by constants. Setting state fields to BITWISE_OR
 * allows multiple of them to be packed together in a single container, with different fields (bit
 * ranges) within the container being written in different states.
 *
 * ### Write mode downgrading          {#write_mode_downgrading}
 *
 * SINGLE_WRITE fields can be downgraded to BITWISE_OR to improve container packing. If a
 * SINGLE_WRITE field is written at most once in any parser path, and only written by constants,
 * then it can be downgraded to BITWISE_OR.
 *
 * The downside of downgrading the write mode is that we lose the ability to check whether the
 * field is written multiple times by any given packet.
 *
 * ### Tofino 2 considerations
 *
 * All containers within the parser are 16b. %Parser containers are combined or split to form 32b
 * and 8b MAU containers. Different write modes _can_ be used for the two adjacent 16b parser
 * containers that form a 32b MAU container. A single write most applies to the 16b parser container
 * that is split to form two 8b MAU containers, so both 8b containers must have the same write mode.
 *
 * Tofino 2 extractors can write a full 16b to a parser container, or they can write 8b to either
 * the upper or lower half of a container. The container write mode is applied to any write to the
 * container. An 8b extract to a SINGLE_WRITE marks the whole container as having been written, so a
 * subsequent extract to the other 8b half will trigger a container multi-write error. Similarly, an
 * 8b extract to a CLEAR_ON_WRITE container clears the whole container before writing the new data:
 * an 8b extract to one half followed by an 8b extract to the other half of a CLEAR_ON_WRITE
 * container results in only the second write being present. Except for very rare circumstances,
 * containers should be set to BITWISE_OR if the upper and lower halves are expected to be written
 * independently for any packet.
 *
 * # Deparser
 *
 * The deparser reassembles packets prior to storage in TM (Tofino 1/2) and prior to transmission
 * via the MAC (all chips).
 *
 * ## Target constants
 *
 * Tofino1:
 * - Target::Tofino::DEPARSER_CHECKSUM_UNITS = 6
 * - Target::Tofino::DEPARSER_CONSTANTS = 0
 * - Target::Tofino::DEPARSER_MAX_POV_BYTES = 32
 * - Target::Tofino::DEPARSER_MAX_POV_PER_USE = 1
 * - Target::Tofino::DEPARSER_MAX_FD_ENTRIES = 192
 *
 * Tofino2:
 * - Target::JBay::DEPARSER_CHECKSUM_UNITS = 8
 * - Target::JBay::DEPARSER_CONSTANTS = 8
 * - Target::JBay::DEPARSER_MAX_POV_BYTES = 16
 * - Target::JBay::DEPARSER_MAX_POV_PER_USE = 1
 * - Target::JBay::DEPARSER_CHUNKS_PER_GROUP = 8
 * - Target::JBay::DEPARSER_CHUNK_SIZE = 8
 * - Target::JBay::DEPARSER_CHUNK_GROUPS = 16
 * - Target::JBay::DEPARSER_CLOTS_PER_GROUP = 4
 * - Target::JBay::DEPARSER_TOTAL_CHUNKS = DEPARSER_CHUNK_GROUPS * DEPARSER_CHUNKS_PER_GROUP = 128
 * - Target::JBay::DEPARSER_MAX_FD_ENTRIES = DEPARSER_TOTAL_CHUNKS = 128
 */
/**
 *
 * ## Midend passes
 *
 * Midend passes related to deparsing:
 *  - BFN::CheckHeaderAlignment (in BFN::PadFlexibleField) - Ensures that headers are byte aligned.
 *  - \ref DesugarVarbitExtract - Generates emit statements for variable-length headers.
 *  - BFN::ParserEnforceDepthReq - Adds emit statements for padding headers to ensure the minimum
 *                                 parse depth.
 *  - \ref SimplifyNestedIf - Simplifies nested if statements into simple if statements that the
 *                           deparser can process.
 *
 * ## Backend passes
 *
 * Backend passes related to deparsing:
 *  - AddDeparserMetadata - Adds deparser metadata parameters.
 *  - AddMetadataPOV - Adds POV bits for metadata used by the deparser (Tofino 2+). Tofino 1
 *                     uses the valid bit associated with each %PHV; Tofino 2+ use POV bits
 *                     instead.
 *  - BFN::AsmOutput - Outputs the deparser assembler. Uses DeparserAsmOutput and the passes it
 *                     invokes.
 *  - CollectClotInfo - Collects information for generating CLOTs.
 *  - \ref DeparserCopyOpt - Optimize copy assigned fields prior to deparsing.
 *  - \ref ExtractChecksum - Replaces EmitField with EmitChecksum emits in deparser. Also
 *                           creates tables and optimizes conditions and fields that the
 *                           checksum uses. Invoked from BFN::BackendConverter.
 *  - BFN::ExtractDeparser - Convert IR::BFN::TnaDeparser objects to IR::BFN::Deparser objects. The
 *                           pass generates emit and digest objects as part of this process.
 *  - GreedyClotAllocator - CLOT allocation. Enforces deparser CLOT rules during allocation.
 *  - InsertParserClotChecksums - Identifies CLOT fields used in deparser checksums to allow the
 *                                checksum to be calculated in the parser (Tofino 2).
 *  - LowerParser - Replaces high-level parser and deparser %IR that operate on fields with
 *                  low-level parser and deparser %IR that operate on %PHV containers.
 *  - \ref ResetInvalidatedChecksumHeaders - Reset fields that are used in deparser checksum
 *                                           operations and that are invalidated in the MAU
 *                                           (Tofino 1).
 */

class PardeSpec {
 public:
    /// @return the size in bytes of the ingress intrinsic metadata header
    /// WARNING this should match "ingress_intrinsic_metadata_t" in tofino.p4
    size_t byteIngressIntrinsicMetadataSize() const { return 8; }

    /// @return the size in bytes of the ingress static per-port metadata on
    /// this device. (This is the "phase 0" data.) On resubmitted packets, the
    /// same region is used for resubmit metadata.
    virtual size_t bytePhase0Size() const = 0;
    size_t bitPhase0Size() const { return bytePhase0Size() * 8; }

    /// @return the size in bits or bytes of the reserved resubmit tag on this device.
    size_t bitResubmitTagSize() const { return 8; }
    size_t byteResubmitTagSize() const { return 1; }

    /// @return the size in bits or bytes of the resubmit data on this device.
    size_t bitResubmitSize() const { return bitPhase0Size(); }
    size_t byteResubmitSize() const { return bytePhase0Size(); }

    /// @return the size in bytes of the padding between the ingress static
    /// per-port metadata and the beginning of the packet.
    virtual size_t byteIngressPrePacketPaddingSize() const = 0;
    size_t bitIngressPrePacketPaddingSize() const {
        return byteIngressPrePacketPaddingSize() * 8;
    }

    /// @return the total size of all ingress metadata on this device.
    size_t byteTotalIngressMetadataSize() const {
        return byteIngressIntrinsicMetadataSize() +
               bytePhase0Size() +
               byteIngressPrePacketPaddingSize();
    }

    /// The size of input buffer, in bytes.
    int byteInputBufferSize() const { return 32; }

    /// The region of the input buffer that contains special intrinsic
    /// metadata rather than packet data. In bytes.
    nw_byterange byteInputBufferMetadataRange() const {
        return StartLen(32, 32);
    }

    /// Specifies the available kinds of extractors, specified as sizes in bits,
    /// and the number of extractors of each kind available in each hardware parser
    /// state.
    virtual const std::map<unsigned, unsigned>& extractorSpec() const = 0;

    /// Specifies the available match registers
    virtual const std::vector<MatchRegister> matchRegisters() const = 0;

    /// Specifies the available scracth registers
    virtual const std::vector<MatchRegister> scratchRegisters() const = 0;

    // Region in the input buffer where the specified scratch register is located.
    virtual const nw_bitrange bitScratchRegisterRange(const MatchRegister &reg) const = 0;

    // Verifies if the provided range is a valid scratch register location.
    virtual bool byteScratchRegisterRangeValid(nw_byterange range) const = 0;

    /// Total parsers supported ingress/egress
    virtual int numParsers() const = 0;

    /// Total TCAM Rows supported ingress/egress
    virtual int numTcamRows() const = 0;

    /// The maximum number of CLOTs that can be generated in each parser state.
    virtual unsigned maxClotsPerState() const = 0;

    /// The maximum number of bytes a CLOT can hold.
    virtual unsigned byteMaxClotSize() const = 0;

    /// The number of CLOTs available for allocation in each gress.
    virtual unsigned numClotsPerGress() const = 0;

    /// The maximum number of CLOTs that can be live for each packet in each gress.
    virtual unsigned maxClotsLivePerGress() const = 0;

    /// The minimum number of bytes required between consecutive CLOTs.
    virtual unsigned byteInterClotGap() const = 0;

    /// The minimum offset a CLOT can have, in bits.
    virtual unsigned bitMinClotPos(gress_t) const = 0;

    /// The maximum offset+length a CLOT can have, in bits.
    virtual unsigned bitMaxClotPos() const = 0;

    /// The minimum parse depth for the given gress
    virtual size_t minParseDepth(gress_t) const { return 0; }

    /// The maximum parse depth for the given gress
    virtual size_t maxParseDepth(gress_t) const { return SIZE_MAX; }

    /// Do all extractors support single-byte extracts?
    virtual bool parserAllExtractorsSupportSingleByte() const = 0;

    // Number of deparser consant bytes available
    virtual unsigned numDeparserConstantBytes() const = 0;

    // Number deparser checksum units each gress can support
    virtual unsigned numDeparserChecksumUnits() const = 0;

    // Number of deparser checksum units that can invert its output
    virtual unsigned numDeparserInvertChecksumUnits() const = 0;

    // Deparser chunk size in bytes
    virtual unsigned deparserChunkSize() const = 0;

    // Deparser group size in chunks
    virtual unsigned deparserChunkGroupSize() const = 0;

    // Number of chunk groups in deparser
    virtual unsigned numDeparserChunkGroups() const = 0;

    // Number of clots in a single chunk group in deparser
    virtual unsigned numClotsPerDeparserGroup() const = 0;

    /// Clock frequency
    virtual double clkFreq() const = 0;

    /// Max line rate per-port (Gbps)
    virtual unsigned lineRate() const = 0;

    /// Unused
    virtual const std::vector<std::string>& mdpValidVecFields() const = 0;

    /// Unused
    virtual const std::unordered_set<std::string>& mdpValidVecFieldsSet() const = 0;
};

class TofinoPardeSpec : public PardeSpec {
 public:
    size_t bytePhase0Size() const override { return 8; }
    size_t byteIngressPrePacketPaddingSize() const override { return 0; }

    const std::map<unsigned, unsigned>& extractorSpec() const override {
        static const std::map<unsigned, unsigned> extractorSpec = {
            {8,  4},
            {16, 4},
            {32, 4}
        };
        return extractorSpec;
    }

    const std::vector<MatchRegister> matchRegisters() const override {
        static std::vector<MatchRegister> spec;

        if (spec.empty()) {
            spec = { MatchRegister("half"_cs),
                     MatchRegister("byte0"_cs),
                     MatchRegister("byte1"_cs) };
        }

        return spec;
    }

    const std::vector<MatchRegister> scratchRegisters() const override {
        return { };
    }

    const nw_bitrange bitScratchRegisterRange(const MatchRegister &reg) const override {
        // Bug check while silencing unused variable warning.
        BUG("Scratch registers not available in Tofino. Register: %1%", reg.name);
        return nw_bitrange();
    }

    bool byteScratchRegisterRangeValid(nw_byterange) const override {
        return false;
    }

    int numParsers() const override { return 18; }
    int numTcamRows() const override { return 256; }

    unsigned maxClotsPerState() const override { BUG("No CLOTs in Tofino"); }
    unsigned byteMaxClotSize() const override { BUG("No CLOTs in Tofino"); }
    unsigned numClotsPerGress() const override { return 0; }

    unsigned maxClotsLivePerGress() const override {
        BUG("No CLOTs in Tofino");
    }

    unsigned byteInterClotGap() const override { BUG("No CLOTs in Tofino"); }

    unsigned bitMinClotPos(gress_t) const override { BUG("No CLOTs in Tofino"); }
    unsigned bitMaxClotPos() const override { BUG("No CLOTs in Tofino"); }

    size_t minParseDepth(gress_t gress) const override { return gress == EGRESS ? 65 : 0; }

    size_t maxParseDepth(gress_t gress) const override { return gress == EGRESS ? 160 : SIZE_MAX; }

    bool parserAllExtractorsSupportSingleByte() const override { return false; }

    unsigned numDeparserConstantBytes() const override { return 0; }
    unsigned numDeparserChecksumUnits() const override { return 6; }
    unsigned numDeparserInvertChecksumUnits() const override { return 0; }
    unsigned deparserChunkSize() const { return 0; }
    unsigned deparserChunkGroupSize() const { return 0; }
    unsigned numDeparserChunkGroups() const { return 0; }
    unsigned numClotsPerDeparserGroup() const { return 0; }

    double clkFreq() const override { return 1.22; }
    unsigned lineRate() const override { return 100; }

    const std::vector<std::string>& mdpValidVecFields() const override {
        static std::vector<std::string> vldVecFields;
        return vldVecFields;
    }

    const std::unordered_set<std::string>& mdpValidVecFieldsSet() const override {
        static std::unordered_set<std::string> vldVecSet;
        return vldVecSet;
    }
};

class JBayPardeSpec : public PardeSpec {
 public:
    size_t bytePhase0Size() const override { return 16; }
    size_t byteIngressPrePacketPaddingSize() const override { return 8; }

    const std::map<unsigned, unsigned>& extractorSpec() const override {
        static const std::map<unsigned, unsigned> extractorSpec = {
            {16, 20}
        };
        return extractorSpec;
    }

    const std::vector<MatchRegister> matchRegisters() const override {
        static std::vector<MatchRegister> spec;

        if (spec.empty()) {
            spec = { MatchRegister("byte0"_cs),
                     MatchRegister("byte1"_cs),
                     MatchRegister("byte2"_cs),
                     MatchRegister("byte3"_cs) };
        }

        return spec;
    }

    const std::vector<MatchRegister> scratchRegisters() const override {
        static std::vector<MatchRegister> spec;

        if (spec.empty()) {
            matchRegisters();  // make sure the match registers are created first

            spec = { MatchRegister("save_byte0"_cs),
                     MatchRegister("save_byte1"_cs),
                     MatchRegister("save_byte2"_cs),
                     MatchRegister("save_byte3"_cs) };
        }

        return spec;
    }

    const nw_bitrange bitScratchRegisterRange(const MatchRegister &reg) const override {
        if (reg.name == "save_byte0")
            return nw_bitrange(504,511);
        else if (reg.name == "save_byte1")
            return nw_bitrange(496,503);
        else if (reg.name == "save_byte2")
            return nw_bitrange(488,495);
        else if (reg.name == "save_byte3")
            return nw_bitrange(480,487);
        else
            BUG("Invalid scratch register %1%", reg.name);
        return nw_bitrange();
    }

    bool byteScratchRegisterRangeValid(nw_byterange range) const override {
        return (range.lo >= 60) && (range.hi <= 63);
    }

    // TBD
    int numParsers() const override { return 36; }
    int numTcamRows() const override { return 256; }

    unsigned maxClotsPerState() const override { return 2; }

    // Cap max size to 56 as a workaround of TF2LAB-44
    unsigned byteMaxClotSize() const override {
        return BackendOptions().tof2lab44_workaround ? 56 : 64; }

    unsigned numClotsPerGress() const override { return 64; }
    unsigned maxClotsLivePerGress() const override { return 16; }
    unsigned byteInterClotGap() const override { return 3; }
    unsigned bitMinClotPos(gress_t gress) const override {
        return gress == EGRESS ? 28 * 8 : byteTotalIngressMetadataSize() * 8;
    }
    unsigned bitMaxClotPos() const override { return 384 * 8; /* 384 bytes */ }
    bool parserAllExtractorsSupportSingleByte() const override { return true; }

    unsigned numDeparserConstantBytes() const override { return 8; }
    unsigned numDeparserChecksumUnits() const override { return 8; }
    unsigned numDeparserInvertChecksumUnits() const override { return 4; }
    unsigned deparserChunkSize() const { return 8; }
    unsigned deparserChunkGroupSize() const { return 8; }
    unsigned numDeparserChunkGroups() const { return 16; }
    unsigned numClotsPerDeparserGroup() const { return 4; }

    // FIXME: adjust to true clock rate
    double clkFreq() const override { return 1.35; }
    unsigned lineRate() const override { return 400; }

    const std::vector<std::string>& mdpValidVecFields() const override {
        static std::vector<std::string> vldVecFields;
        return vldVecFields;
    }

    const std::unordered_set<std::string>& mdpValidVecFieldsSet() const override {
        static std::unordered_set<std::string> vldVecSet;
        return vldVecSet;
    }
};

class JBayA0PardeSpec : public JBayPardeSpec {
 public:
    unsigned numDeparserInvertChecksumUnits() const override { return 0; }
};



#endif /* EXTENSIONS_BF_P4C_PARDE_PARDE_SPEC_H_ */
