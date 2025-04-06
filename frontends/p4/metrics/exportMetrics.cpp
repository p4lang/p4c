#include <iostream>
#include <fstream>
#include <unordered_set>
#include "exportMetrics.h"

namespace P4 {

bool ExportMetricsPass::preorder(const IR::P4Program* /*program*/) {

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << " for writing metrics.\n";
        return false;
    }

    for (const auto &metric : selectedMetrics) {
        if (metric == "cyclomatic") {
            file << "\nCyclomatic Complexity:\n";
            for (const auto &[func, cc] : metrics.cyclomaticComplexity) {
                file <<"  "<<func << ": " << cc << "\n";
            }
        } 
        else if (metric == "halstead") {
            file << "\nHalstead Metrics:\n";
            file << "  Unique Operators: " << metrics.halsteadMetrics.uniqueOperators << "\n";
            file << "  Unique Operands: " << metrics.halsteadMetrics.uniqueOperands << "\n";
            file << "  Total Operators: " << metrics.halsteadMetrics.totalOperators << "\n";
            file << "  Total Operands: " << metrics.halsteadMetrics.totalOperands << "\n";

            file << "  Vocabulary: " << metrics.halsteadMetrics.vocabulary << "\n";
            file << "  Length: " << metrics.halsteadMetrics.length << "\n";
            file << "  Difficulty: " << metrics.halsteadMetrics.difficulty << "\n";
            file << "  Volume: " << metrics.halsteadMetrics.volume << "\n";
            file << "  Effort: " << metrics.halsteadMetrics.effort << "\n";
            file << "  Estimated Bugs: " << metrics.halsteadMetrics.deliveredBugs << "\n";
        } 
        else if (metric == "unused-code") {
            file << "\nUnused Code Instances:\n";
            
            file << "  Program Structure:\n";
            file << "\tActions: " << metrics.unusedCodeInstances.actions << "\n";
            file << "\tFunctions: " << metrics.unusedCodeInstances.functions << "\n";
            file << "\tParser states: " << metrics.unusedCodeInstances.states << "\n";

            file << "  Declarations:\n";
            file << "\tVariables: " << metrics.unusedCodeInstances.variables << "\n";
            file << "\tEnums: " << metrics.unusedCodeInstances.enums << "\n";

            file << "  Control Flow:\n";
            file << "\tBlocks: " << metrics.unusedCodeInstances.blocks << "\n";
            file << "\tConditionals: " << metrics.unusedCodeInstances.conditionals << "\n";
            file << "\tSwitches: " << metrics.unusedCodeInstances.switches << "\n";

            file << "  Other:\n";
            file << "\tParameters: " << metrics.unusedCodeInstances.parameters << "\n";
            file << "\tReturns: " << metrics.unusedCodeInstances.returns << "\n";
        }
        else if (metric == "nesting-depth") {
            file << "\nNesting Depth Metrics:\n";
            file << "  Average: " << metrics.nestingDepth.avgNestingDepth << "\n";
            file << "  Global Max: " << metrics.nestingDepth.maxNestingDepth << "\n";
            file << "  Individual blocks: \n";

            auto iterator = metrics.nestingDepth.blockNestingDepth.begin();
            while (iterator != metrics.nestingDepth.blockNestingDepth.end()){
                const auto& [blockName, depth] = *iterator;
                auto depthValue = metrics.nestingDepth.blockNestingDepth[blockName];
                file<<"\t"<<blockName<<": "<<depthValue<<"\n";
                iterator++;
            }
        } 
        else if (metric == "header-general") {
            file << "\nHeader Metrics:\n";
            file << "  Total Headers: " << metrics.headerMetrics.numHeaders << "\n";
            file << "  Avg Fields Per Header: " << metrics.headerMetrics.avgFieldsNum << "\n";
            file << "  Avg Field Size: " << metrics.headerMetrics.avgFieldSize << "\n";
            file << "  Per-header metrics:\n";

            auto iterator = metrics.headerMetrics.fieldsNum.begin();
            while (iterator != metrics.headerMetrics.fieldsNum.end()){
                const auto& [headerName, numFields] = *iterator;
                auto sizeFields = metrics.headerMetrics.fieldSizeSum[headerName];
                file<<"\t"<<headerName<<":\n";
                file<<"\t  Fields: "<<numFields<<"\n";
                file<<"\t  Fields size sum: "<<sizeFields<<"\n";
                iterator++;
            }
        } 
        else if (metric == "header-manipulation") {
            file << "\nHeader Manipulation Metrics:\n";
            file << "  Total Operations: " << metrics.headerManipulationMetrics.totalManipulations.numOperations << "\n";
            file << "  Total Size: " << metrics.headerManipulationMetrics.totalManipulations.totalSize << "\n";
            file << "  Per-packet metrics:\n";

            auto iterator = metrics.headerManipulationMetrics.perPacketManipulations.begin();
            while (iterator != metrics.headerManipulationMetrics.perPacketManipulations.end()){
                const auto& [packetName, operations] = *iterator;
                file<<"\t"<<packetName<<":\n";
                file<<"\t  Operations: "<<operations.numOperations<<"\n";
                file<<"\t  Size: "<<operations.totalSize<<"\n";
                iterator++;
            }
        }
        else if (metric == "header-modification") {
            file << "\nHeader Modification Metrics:\n";
            file << "  Total Operations: " << metrics.headerModificationMetrics.totalModifications.numOperations << "\n";
            file << "  Total Size: " << metrics.headerModificationMetrics.totalModifications.totalSize << "\n";
            file << "  Per-packet metrics:\n";

            auto iterator = metrics.headerModificationMetrics.perPacketModifications.begin();
            while (iterator != metrics.headerModificationMetrics.perPacketModifications.end()){
                const auto& [packetName, operations] = *iterator;
                file<<"\t"<<packetName<<":\n";
                file<<"\t  Operations: "<<operations.numOperations<<"\n";
                file<<"\t  Size: "<<operations.totalSize<<"\n";
                iterator++;
            }
        }
        else if (metric == "match-action") {
            file << "\nMatch-Action Table Metrics:\n";
            file << "  Number of Tables: " << metrics.matchActionTableMetrics.numTables << "\n";
            file << "  Total Keys: " << metrics.matchActionTableMetrics.totalKeys << "\n";
            file << "  Total Key Size: " << metrics.matchActionTableMetrics.totalKeySizeSum << "\n";
            file << "  Avg Key Size: " << metrics.matchActionTableMetrics.avgKeySize << "\n";
            file << "  Avg Keys Per Table: " << metrics.matchActionTableMetrics.avgKeysPerTable << "\n";
            file << "  Max Keys Per Table: " << metrics.matchActionTableMetrics.maxKeysPerTable << "\n";
            file << "  Total Actions: " << metrics.matchActionTableMetrics.totalActions << "\n";
            file << "  Avg Actions Per Table: " << metrics.matchActionTableMetrics.avgActionsPerTable << "\n";
            file << "  Max Actions Per Table: " << metrics.matchActionTableMetrics.maxActionsPerTable << "\n";
            file << "  Per-table Metrics:\n";

            auto iterator = metrics.matchActionTableMetrics.keysNum.begin();
            while (iterator != metrics.matchActionTableMetrics.keysNum.end()){
                const auto& [tableName, numKeys] = *iterator;
                auto numActions = metrics.matchActionTableMetrics.actionsNum[tableName];
                auto keySizeSum = metrics.matchActionTableMetrics.keySizeSum[tableName];
                file<<"  "<<tableName<<":\n";
                file<<"\t Actions: "<<numActions<<"\n";
                file<<"\t Keys: "<<numKeys<<"\n";
                file<<"\t Key size sum: "<<keySizeSum<<"\n";
                iterator++;
            }
        } 
        else if (metric == "parser") {
            file << "\nParser Metrics:\n";
            file << "  States: " << metrics.parserMetrics.totalStates << "\n";
            file << "  Per-parser complexities:\n";

            auto iterator = metrics.parserMetrics.StateComplexity.begin();
            while (iterator != metrics.parserMetrics.StateComplexity.end()){
                const auto& [stateName, complexity] = *iterator;
                auto CCValue = metrics.parserMetrics.StateComplexity[stateName];
                file<<"\t"<<stateName<<": "<<CCValue<<"\n";
                iterator++;
            }
        } 
        else if (metric == "extern") {
            file << "\nExtern Metrics:\n";
            file << "  Extern Functions: " << metrics.externMetrics.externFunctions << "\n";
            file << "  Function Calls: " << metrics.externMetrics.externFunctionUses << "\n";
            file << "  Extern Structures: " << metrics.externMetrics.externStructures << "\n";
            file << "  Number of Structure Uses: " << metrics.externMetrics.externStructUses << "\n";
        }
        else if (metric == "inlined") {
            file << "\nNumber of Inlined Actions: " << metrics.inlinedActionsNum << "\n";
        } 
    }

    file.close();

    return false;
}

}  // namespace P4