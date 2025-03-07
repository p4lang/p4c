#include <iostream>
#include <fstream>
#include <unordered_set>
#include "metrics.h"

namespace P4 {

// Code metric collection options
std::unordered_set<std::string> codeMetrics = {};

std::unordered_set<std::string> validMetrics = {
    "cyclomatic",
    "halstead",
    "unused-code",
    "duplicit-code",
    "nesting-depth",
    "header-general",
    "header-manipulation",
    "header-modification",
    "match-action",
    "parser",
    "inlined",
    "extern"
};

std::unordered_map<std::string, int> cyclomaticComplexity;
int duplicateCodeInstances = 0;
int unusedCodeInstances = 0;
NestingDepth nestingDepth;
HalsteadMetrics halsteadMetrics;
HeaderMetrics headerMetrics;
HeaderManipulationMetrics headerManipulationMetrics;
HeaderModificationMetrics headerModificationMetrics;
MatchActionTableMetrics matchActionTableMetrics;
ParserMetrics parserMetrics;
int inlinedActionsNum = 0;
int externalObjectsNum = 0;

bool ExportMetricsPass::preorder(const IR::P4Program *program) {
    std::cout<<"Exporting metrics"<<std::endl;

    for (const auto& metric : codeMetrics) {
            std::cout << metric << "\n";
    } 
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << " for writing metrics.\n";
        return false;
    }

    file << "Collected P4 Metrics:\n";

    for (const auto &metric : codeMetrics) {
        if (metric == "cyclomatic") {
            file << "\nCyclomatic Complexity:\n";
            for (const auto &[func, cc] : cyclomaticComplexity) {
                file << "  Function: " << func << " -> CC: " << cc << "\n";
            }
        } 
        else if (metric == "halstead") {
            file << "\nHalstead Metrics:\n";
            file << "  Unique Operators: " << halsteadMetrics.uniqueOperators << "\n";
            file << "  Unique Operands: " << halsteadMetrics.uniqueOperands << "\n";
            file << "  Total Operators: " << halsteadMetrics.totalOperators << "\n";
            file << "  Total Operands: " << halsteadMetrics.totalOperands << "\n";
            file << "  Vocabulary: " << halsteadMetrics.vocabulary << "\n";
            file << "  Length: " << halsteadMetrics.length << "\n";
            file << "  Difficulty: " << halsteadMetrics.difficulty << "\n";
            file << "  Volume: " << halsteadMetrics.volume << "\n";
            file << "  Effort: " << halsteadMetrics.effort << "\n";
            file << "  Estimated Bugs: " << halsteadMetrics.deliveredBugs << "\n";
        } 
        else if (metric == "unreachable-code") {
            file << "\nUnreachable Code Instances: " << unusedCodeInstances << "\n";
        } 
        else if (metric == "duplicit-code") {
            file << "\nDuplicate Code Instances: " << duplicateCodeInstances << "\n";
        } 
        else if (metric == "nesting-depth") {
            file << "\nNesting Depth Metrics:\n";
            file << "  Avg Depth: " << nestingDepth.avgNestingDepth << "\n";
            file << "  Max Depth: " << nestingDepth.maxNestingDepth << "\n";
        } 
        else if (metric == "header-general") {
            file << "\nHeader Metrics:\n";
            file << "  Total Headers: " << headerMetrics.numHeaders << "\n";
            file << "  Avg Fields Per Header: " << headerMetrics.avgFieldsNum << "\n";
            file << "  Avg Field Size: " << headerMetrics.avgFieldSize << "\n";
        } 
        else if (metric == "header-manipulation") {
            file << "\nHeader Manipulation Metrics:\n";
            file << "  Total Operations: " << headerManipulationMetrics.totalManipulations.numOperations << "\n";
            file << "  Total Size: " << headerManipulationMetrics.totalManipulations.totalSize << "\n";
        } 
        else if (metric == "header-modification") {
            file << "\nHeader Modification Metrics:\n";
            file << "  Total Operations: " << headerModificationMetrics.totalModifications.numOperations << "\n";
            file << "  Total Size: " << headerModificationMetrics.totalModifications.totalSize << "\n";
        } 
        else if (metric == "match-action") {
            file << "\nMatch-Action Table Metrics:\n";
            file << "  Number of Tables: " << matchActionTableMetrics.numTables << "\n";
            file << "  Total Keys: " << matchActionTableMetrics.totalKeys << "\n";
            file << "  Total Actions: " << matchActionTableMetrics.totalActions << "\n";
        } 
        else if (metric == "parser") {
            file << "\nParser Metrics:\n";
            file << "  Total States: " << parserMetrics.totalStates << "\n";
        } 
        else if (metric == "inlined") {
            file << "\nInlined Actions: " << inlinedActionsNum << "\n";
        } 
        else if (metric == "extern") {
            file << "\nExternal Objects: " << externalObjectsNum << "\n";
        }
    }

    file.close();
    std::cout << "Metrics exported to " << filename << " successfully.\n";

    return false;
}

}  // namespace P4