/*
Collects basic Halstead metrics (unique and total operators and operands)
of the compiled program, and calculates derived Halstead metrics 
(vocabulary, length, difficulty, volume, effort, delivered bugs) at
the end of traversal. The collection process is based on rules, 
which were adapted from the rules for collecting Halstead metrics in C:

- Identifier and control block declarations are not considered.
- Constructor calls are considered operators.
- All variables, constants, and literals are considered operands.
- Local variables and constants with identical names in different blocks are considered distinct operands.
- Structure fields are considered as global operands, and are only counted when used.
- Control flow statements (if, select, transition, etc.) are considered operators.
- Function calls, method calls and action invocations are considered operators.
- Built-in functions (extract, clear, get, etc.) are considered operators.
- Square brackets and dots are considered operators.
- Unary and binary versions of operators such as + and - are counted separately.
- State names in transition statements are considered operands (since they are similar to GOTO statements in C)
*/

#ifndef FRONTENDS_P4_HALSTEAD_METRICS_H_
#define FRONTENDS_P4_HALSTEAD_METRICS_H_

#include <cmath>
#include <unordered_set>
#include <vector>
#include <string>

#include "../ir/ir.h"
#include "../lib/log.h"
#include "metricsStructure.h"

namespace P4 {

class HalsteadMetricsPass : public Inspector {
 private:
    HalsteadMetrics &metrics;
    std::unordered_set<std::string> uniqueUnaryOperators;
    std::unordered_set<std::string> uniqueBinaryOperators;
    std::unordered_multiset<std::string> uniqueOperands;
    std::unordered_set<std::string> structFields; // Field names of the defined structures.
    std::unordered_set<std::string> uniqueFields; // Structure fields that were used in the program.
    std::vector<std::unordered_set<std::string>> scopedOperands; // Operands separated by scopes. 
    const std::unordered_set<std::string> reservedKeywords = {
      "extract",       
      "emit",          
      "isValid",      
      "setValid",      
      "setInvalid",    
      "push_front",    
      "pop_front",     
      "next",          
      "last",          
      "apply",         
      "hit",           
      "miss",          
      "action_run",              
      "accept",        
      "mark_to_drop",  
      "read",          
      "write",         
      "count",         
      "execute",       
      "clear",         
      "update",        
      "get",          
      "verify",        
      "clone",         
      "resubmit",      
      "recirculate",   
      "transition",    
      "size",          
      "max_length",    
      "length",        
    };
    const std::unordered_set<std::string> matchTypes = {
      "exact", 
      "lpm", 
      "ternary", 
      "range", 
      "optional"
    };

    //Helper methods.

    void addOperand(const std::string& operand);
    void addUnaryOperator(const std::string& op);
    void addBinaryOperator(const std::string& op);

 public:
    explicit HalsteadMetricsPass(Metrics &metricsRef)
      : metrics(metricsRef.halsteadMetrics) { setName("HalsteadMetricsPass"); }

    // Scope handling.

    bool preorder(const IR::P4Control* /*control*/) override;
    void postorder(const IR::P4Control* /*control*/) override;
    bool preorder(const IR::P4Parser* /*parser*/) override;
    void postorder(const IR::P4Parser* /*parser*/) override;
    bool preorder(const IR::Function* /*function*/) override;
    void postorder(const IR::Function* /*function*/) override;

   // Operand and operator data collection. 

    void postorder(const IR::Type_Header* headerType) override;
    void postorder(const IR::Type_Struct* structType) override;
    bool preorder(const IR::PathExpression *pathExpr) override;
    bool preorder(const IR::Member *member) override;
    void postorder(const IR::Constant *constant) override;
    void postorder(const IR::ConstructorCallExpression *ctorCall) override;
    bool preorder(const IR::MethodCallExpression *methodCall) override;
    void postorder(const IR::AssignmentStatement* /*stmt*/) override;
    void postorder(const IR::IfStatement *stmt) override;
    void postorder(const IR::SwitchStatement* /*stmt*/) override;
    void postorder(const IR::SwitchCase* /*case*/) override;
    void postorder(const IR::ForStatement* /*stmt*/) override;  
    void postorder(const IR::ForInStatement* /*stmt*/) override;
    void postorder(const IR::ReturnStatement* /*stmt*/) override;
    void postorder(const IR::ExitStatement* /*stmt*/) override;
    bool preorder(const IR::Operation_Unary *op) override;
    void postorder(const IR::Operation_Binary *op) override;
    void postorder(const IR::ParserState* state) override;
    void postorder(const IR::SelectExpression* /*selectExpr*/) override;
    bool preorder(const IR::SelectCase* selectCase) override;
    void postorder(const IR::P4Table *table) override;

    /// Calculate metrics at the end of traversal.
    void postorder(const IR::P4Program* /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HALSTEAD_METRICS_H_ */
