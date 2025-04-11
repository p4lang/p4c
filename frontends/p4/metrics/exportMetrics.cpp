#include "exportMetrics.h"

namespace P4 {

bool ExportMetricsPass::preorder(const IR::P4Program* /*program*/) {
    // Open text file
    std::ofstream textFile(filename + ".txt");
    if (!textFile.is_open()) {
        std::cerr << "Error: Unable to open file " << filename + ".txt" << "\n";
        return false;
    }

    // Open JSON file
    std::string jsonFilename = filename + ".json";
    std::ofstream jsonFile(jsonFilename);
    if (!jsonFile.is_open()) {
        std::cerr << "Error: Unable to open JSON file " << jsonFilename << "\n";
        return false;
    }

    Util::JsonObject* root = new Util::JsonObject();

    // Cyclomatic complexity.
    if (selectedMetrics.count("cyclomatic")) {
        textFile << "\nCyclomatic Complexity:\n";
        auto* ccJson = new Util::JsonObject();
        for (const auto& [func, cc] : metrics.cyclomaticComplexity) {
            textFile << "  " << func << ": " << cc << "\n";
            ccJson->emplace(toCString(func), new Util::JsonValue(cc));
        }
        root->emplace("cyclomatic_complexity", ccJson);
    }

    // Halstead metrics.
    if (selectedMetrics.count("halstead")) {
        textFile << "\nHalstead Metrics:\n"
                << "  Unique Operators: " << metrics.halsteadMetrics.uniqueOperators << "\n"
                << "  Unique Operands: " << metrics.halsteadMetrics.uniqueOperands << "\n"
                << "  Total Operators: " << metrics.halsteadMetrics.totalOperators << "\n"
                << "  Total Operands: " << metrics.halsteadMetrics.totalOperands << "\n"
                << "  Vocabulary: " << metrics.halsteadMetrics.vocabulary << "\n"
                << "  Length: " << metrics.halsteadMetrics.length << "\n"
                << "  Difficulty: " << metrics.halsteadMetrics.difficulty << "\n"
                << "  Volume: " << metrics.halsteadMetrics.volume << "\n"
                << "  Effort: " << metrics.halsteadMetrics.effort << "\n"
                << "  Estimated Bugs: " << metrics.halsteadMetrics.deliveredBugs << "\n";

        auto* halsteadJson = new Util::JsonObject();
        halsteadJson->emplace("unique_operators", new Util::JsonValue(metrics.halsteadMetrics.uniqueOperators))
                   ->emplace("unique_operands", new Util::JsonValue(metrics.halsteadMetrics.uniqueOperands))
                   ->emplace("total_operators", new Util::JsonValue(metrics.halsteadMetrics.totalOperators))
                   ->emplace("total_operands", new Util::JsonValue(metrics.halsteadMetrics.totalOperands))
                   ->emplace("vocabulary", new Util::JsonValue(metrics.halsteadMetrics.vocabulary))
                   ->emplace("length", new Util::JsonValue(metrics.halsteadMetrics.length))
                   ->emplace("difficulty", new Util::JsonValue(metrics.halsteadMetrics.difficulty))
                   ->emplace("volume", new Util::JsonValue(metrics.halsteadMetrics.volume))
                   ->emplace("effort", new Util::JsonValue(metrics.halsteadMetrics.effort))
                   ->emplace("estimated_bugs", new Util::JsonValue(metrics.halsteadMetrics.deliveredBugs));
        root->emplace("halstead", halsteadJson);
    }

    // Unused code metrics.
    if (selectedMetrics.count("unused-code")) {
        textFile << "\nUnused Code Instances:\n"
                << "  Program Structure:\n"
                << "\tActions: " << metrics.unusedCodeInstances.actions << "\n"
                << "\tFunctions: " << metrics.unusedCodeInstances.functions << "\n"
                << "\tParser states: " << metrics.unusedCodeInstances.states << "\n"
                << "  Declarations:\n"
                << "\tVariables: " << metrics.unusedCodeInstances.variables << "\n"
                << "\tEnums: " << metrics.unusedCodeInstances.enums << "\n"
                << "  Control Flow:\n"
                << "\tBlocks: " << metrics.unusedCodeInstances.blocks << "\n"
                << "\tConditionals: " << metrics.unusedCodeInstances.conditionals << "\n"
                << "\tSwitches: " << metrics.unusedCodeInstances.switches << "\n"
                << "  Other:\n"
                << "\tParameters: " << metrics.unusedCodeInstances.parameters << "\n"
                << "\tReturns: " << metrics.unusedCodeInstances.returns << "\n";

        auto* unusedJson = new Util::JsonObject();
        unusedJson->emplace("actions", new Util::JsonValue(metrics.unusedCodeInstances.actions))
                   ->emplace("functions", new Util::JsonValue(metrics.unusedCodeInstances.functions))
                   ->emplace("states", new Util::JsonValue(metrics.unusedCodeInstances.states))
                   ->emplace("variables", new Util::JsonValue(metrics.unusedCodeInstances.variables))
                   ->emplace("enums", new Util::JsonValue(metrics.unusedCodeInstances.enums))
                   ->emplace("blocks", new Util::JsonValue(metrics.unusedCodeInstances.blocks))
                   ->emplace("conditionals", new Util::JsonValue(metrics.unusedCodeInstances.conditionals))
                   ->emplace("switches", new Util::JsonValue(metrics.unusedCodeInstances.switches))
                   ->emplace("parameters", new Util::JsonValue(metrics.unusedCodeInstances.parameters))
                   ->emplace("returns", new Util::JsonValue(metrics.unusedCodeInstances.returns));
        root->emplace("unused_code", unusedJson);
    }

    // Nesting depth metrics.
    if (selectedMetrics.count("nesting-depth")) {
        textFile << "\nNesting Depth Metrics:\n"
                << "  Average: " << metrics.nestingDepth.avgNestingDepth << "\n"
                << "  Global Max: " << metrics.nestingDepth.maxNestingDepth << "\n"
                << "  Individual blocks: \n";

        auto* nestingJson = new Util::JsonObject();
        auto* blockDepths = new Util::JsonObject();
        for (const auto& [blockName, depth] : metrics.nestingDepth.blockNestingDepth) {
            textFile << "\t" << blockName << ": " << depth << "\n";
            blockDepths->emplace(toCString(blockName), new Util::JsonValue(depth));
        }
        nestingJson->emplace("average", new Util::JsonValue(metrics.nestingDepth.avgNestingDepth))
                  ->emplace("global_max", new Util::JsonValue(metrics.nestingDepth.maxNestingDepth))
                  ->emplace("individual_blocks", blockDepths);
        root->emplace("nesting_depth", nestingJson);
    }

    // Header general metrics.
    if (selectedMetrics.count("header-general")) {
        textFile << "\nHeader Metrics:\n"
                << "  Total Headers: " << metrics.headerMetrics.numHeaders << "\n"
                << "  Avg Fields Per Header: " << metrics.headerMetrics.avgFieldsNum << "\n"
                << "  Avg Field Size: " << metrics.headerMetrics.avgFieldSize << "\n"
                << "  Per-header metrics:\n";

        auto* headerJson = new Util::JsonObject();
        auto* perHeader = new Util::JsonObject();
        for (const auto& [header, fields] : metrics.headerMetrics.fieldsNum) {
            textFile << "\t" << header << ":\n"
                    << "\t  Fields: " << fields << "\n"
                    << "\t  Fields size sum: " << metrics.headerMetrics.fieldSizeSum.at(header) << "\n";
            
            auto* headerData = new Util::JsonObject();
            headerData->emplace("fields", new Util::JsonValue(fields))
                       ->emplace("field_size_sum", new Util::JsonValue(metrics.headerMetrics.fieldSizeSum.at(header)));
            perHeader->emplace(toCString(header), headerData);
        }
        headerJson->emplace("total_headers", new Util::JsonValue(metrics.headerMetrics.numHeaders))
                 ->emplace("avg_fields_per_header", new Util::JsonValue(metrics.headerMetrics.avgFieldsNum))
                 ->emplace("avg_field_size", new Util::JsonValue(metrics.headerMetrics.avgFieldSize))
                 ->emplace("per_header_metrics", perHeader);
        root->emplace("header_metrics", headerJson);
    }

    // Header manipulation metrics.
    if (selectedMetrics.count("header-manipulation")) {
        textFile << "\nHeader Manipulation Metrics:\n"
                << "  Total Operations: " << metrics.headerManipulationMetrics.total.numOperations << "\n"
                << "  Total Size: " << metrics.headerManipulationMetrics.total.totalSize << "\n"
                << "  Per-packet metrics:\n";

        auto* manipulationJson = new Util::JsonObject();
        auto* perPacketManip = new Util::JsonObject();
        for (const auto& [packet, ops] : metrics.headerManipulationMetrics.perPacket) {
            textFile << "\t" << packet << ":\n"
                    << "\t  Operations: " << ops.numOperations << "\n"
                    << "\t  Size: " << ops.totalSize << "\n";
            
            auto* packetOps = new Util::JsonObject();
            packetOps->emplace("operations", new Util::JsonValue(ops.numOperations))
                      ->emplace("size", new Util::JsonValue(ops.totalSize));
            perPacketManip->emplace(toCString(packet), packetOps);
        }
        manipulationJson->emplace("total_operations", 
                                 new Util::JsonValue(metrics.headerManipulationMetrics.total.numOperations))
                        ->emplace("total_size", 
                                 new Util::JsonValue(metrics.headerManipulationMetrics.total.totalSize))
                        ->emplace("per_packet", perPacketManip);
        root->emplace("header_manipulation", manipulationJson);
    }

    // Header modification metrics.
    if (selectedMetrics.count("header-modification")) {
        textFile << "\nHeader Modification Metrics:\n"
                << "  Total Operations: " << metrics.headerModificationMetrics.total.numOperations << "\n"
                << "  Total Size: " << metrics.headerModificationMetrics.total.totalSize << "\n"
                << "  Per-packet metrics:\n";

        auto* modificationJson = new Util::JsonObject();
        auto* perPacketMod = new Util::JsonObject();
        for (const auto& [packet, ops] : metrics.headerModificationMetrics.perPacket) {
            textFile << "\t" << packet << ":\n"
                    << "\t  Operations: " << ops.numOperations << "\n"
                    << "\t  Size: " << ops.totalSize << "\n";
            
            auto* packetMod = new Util::JsonObject();
            packetMod->emplace("operations", new Util::JsonValue(ops.numOperations))
                     ->emplace("size", new Util::JsonValue(ops.totalSize));
            perPacketMod->emplace(toCString(packet), packetMod);
        }
        modificationJson->emplace("total_operations", 
                                new Util::JsonValue(metrics.headerModificationMetrics.total.numOperations))
                       ->emplace("total_size", 
                                new Util::JsonValue(metrics.headerModificationMetrics.total.totalSize))
                       ->emplace("per_packet", perPacketMod);
        root->emplace("header_modification", modificationJson);
    }


    // Match-action table metrics.
    if (selectedMetrics.count("match-action")) {
        textFile << "\nMatch-Action Table Metrics:\n"
                << "  Number of Tables: " << metrics.matchActionTableMetrics.numTables << "\n"
                << "  Total Keys: " << metrics.matchActionTableMetrics.totalKeys << "\n"
                << "  Total Key Size: " << metrics.matchActionTableMetrics.totalKeySizeSum << "\n"
                << "  Avg Key Size: " << metrics.matchActionTableMetrics.avgKeySize << "\n"
                << "  Avg Keys Per Table: " << metrics.matchActionTableMetrics.avgKeysPerTable << "\n"
                << "  Max Keys Per Table: " << metrics.matchActionTableMetrics.maxKeysPerTable << "\n"
                << "  Total Actions: " << metrics.matchActionTableMetrics.totalActions << "\n"
                << "  Avg Actions Per Table: " << metrics.matchActionTableMetrics.avgActionsPerTable << "\n"
                << "  Max Actions Per Table: " << metrics.matchActionTableMetrics.maxActionsPerTable << "\n"
                << "  Per-table Metrics:\n";

        auto* tableJson = new Util::JsonObject();
        auto* perTable = new Util::JsonObject();
        for (const auto& [table, keys] : metrics.matchActionTableMetrics.keysNum) {
            textFile << "  " << table << ":\n"
                    << "\t Actions: " << metrics.matchActionTableMetrics.actionsNum.at(table) << "\n"
                    << "\t Keys: " << keys << "\n"
                    << "\t Key size sum: " << metrics.matchActionTableMetrics.keySizeSum.at(table) << "\n";
            
            auto* tableData = new Util::JsonObject();
            tableData->emplace("actions", new Util::JsonValue(metrics.matchActionTableMetrics.actionsNum.at(table)))
                     ->emplace("keys", new Util::JsonValue(keys))
                     ->emplace("key_size_sum", new Util::JsonValue(metrics.matchActionTableMetrics.keySizeSum.at(table)));
            perTable->emplace(toCString(table), tableData);
        }
        tableJson->emplace("num_tables", new Util::JsonValue(metrics.matchActionTableMetrics.numTables))
                ->emplace("total_keys", new Util::JsonValue(metrics.matchActionTableMetrics.totalKeys))
                ->emplace("total_key_size", new Util::JsonValue(metrics.matchActionTableMetrics.totalKeySizeSum))
                ->emplace("avg_key_size", new Util::JsonValue(metrics.matchActionTableMetrics.avgKeySize))
                ->emplace("avg_keys_per_table", new Util::JsonValue(metrics.matchActionTableMetrics.avgKeysPerTable))
                ->emplace("max_keys_per_table", new Util::JsonValue(metrics.matchActionTableMetrics.maxKeysPerTable))
                ->emplace("total_actions", new Util::JsonValue(metrics.matchActionTableMetrics.totalActions))
                ->emplace("avg_actions_per_table", new Util::JsonValue(metrics.matchActionTableMetrics.avgActionsPerTable))
                ->emplace("max_actions_per_table", new Util::JsonValue(metrics.matchActionTableMetrics.maxActionsPerTable))
                ->emplace("per_table_metrics", perTable);
        root->emplace("match_action_tables", tableJson);
    }

    // Parser metrics.
    if (selectedMetrics.count("parser")) {
        textFile << "\nParser Metrics:\n"
                << "  States: " << metrics.parserMetrics.totalStates << "\n"
                << "  Per-parser complexities:\n";

        auto* parserJson = new Util::JsonObject();
        auto* stateComplexities = new Util::JsonObject();
        for (const auto& [state, complexity] : metrics.parserMetrics.StateComplexity) {
            textFile << "\t" << state << ": " << complexity << "\n";
            stateComplexities->emplace(toCString(state), new Util::JsonValue(complexity));
        }
        parserJson->emplace("total_states", new Util::JsonValue(metrics.parserMetrics.totalStates))
                 ->emplace("state_complexities", stateComplexities);
        root->emplace("parser", parserJson);
    }

    // Extern metrics.
    if (selectedMetrics.count("extern")) {
        textFile << "\nExtern Metrics:\n"
                << "  Extern Functions: " << metrics.externMetrics.externFunctions << "\n"
                << "  Function Calls: " << metrics.externMetrics.externFunctionUses << "\n"
                << "  Extern Structures: " << metrics.externMetrics.externStructures << "\n"
                << "  Structure Uses: " << metrics.externMetrics.externStructUses << "\n";

        auto* externJson = new Util::JsonObject();
        externJson->emplace("functions", new Util::JsonValue(metrics.externMetrics.externFunctions))
                  ->emplace("function_calls", new Util::JsonValue(metrics.externMetrics.externFunctionUses))
                  ->emplace("structures", new Util::JsonValue(metrics.externMetrics.externStructures))
                  ->emplace("structure_uses", new Util::JsonValue(metrics.externMetrics.externStructUses));
        root->emplace("extern", externJson);
    }

    // Inlined actions.
    if (selectedMetrics.count("inlined")) {
        textFile << "\nNumber of Inlined Actions: " << metrics.inlinedActionsNum << "\n";
        root->emplace("inlined_actions", new Util::JsonValue(metrics.inlinedActionsNum));
    }

    jsonFile << root->toString();
    textFile.close();
    jsonFile.close();

    return false;
}

}  // namespace P4