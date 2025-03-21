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
    Metrics &metrics;
    std::unordered_set<std::string> uniqueUnaryOperators;
    std::unordered_set<std::string> uniqueBinaryOperators;
    std::unordered_multiset<std::string> uniqueOperands;
    std::unordered_set<std::string> uniqueFields;
    std::unordered_set<std::string> structFields;
    std::vector<std::unordered_set<std::string>> scopedOperands;
    const std::unordered_set<std::string> specialMethods = {
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
      : metrics(metricsRef) { setName("HalsteadMetricsPass"); }

    // Scope enter/exit methods.

    bool preorder([[maybe_unused]] const IR::P4Control *control) override;
    void postorder([[maybe_unused]] const IR::P4Control *control) override;
    bool preorder([[maybe_unused]] const IR::P4Parser *parser) override;
    void postorder([[maybe_unused]] const IR::P4Parser *parser) override;
    bool preorder([[maybe_unused]] const IR::ActionFunction *action) override;
    void postorder([[maybe_unused]] const IR::ActionFunction *action) override;
    bool preorder([[maybe_unused]] const IR::Type_Header *headerType) override;
    bool preorder([[maybe_unused]] const IR::Type_Struct *structType) override;

   // Operand and operator data collection. 

    bool preorder(const IR::PathExpression *pathExpr) override;
    bool preorder(const IR::Member *member) override;
    bool preorder(const IR::Constant *constant) override;
    bool preorder(const IR::ConstructorCallExpression *ctorCall) override;
    bool preorder(const IR::MethodCallExpression *methodCall) override;
    bool preorder(const IR::AssignmentStatement *stmt) override;
    bool preorder(const IR::IfStatement *stmt) override;
    bool preorder(const IR::SwitchStatement *stmt) override;
    bool preorder([[maybe_unused]] const IR::ReturnStatement *stmt) override;
    bool preorder([[maybe_unused]] const IR::ExitStatement *stmt) override;
    bool preorder(const IR::Operation_Unary *op) override;
    bool preorder(const IR::Operation_Binary *op) override;
    bool preorder(const IR::SelectExpression* selectExpr) override;
    bool preorder(const IR::SelectCase* selectCase) override;
    bool preorder(const IR::P4Table *table) override;

    /// Calculate metrics at the end of traversal
    void postorder([[maybe_unused]] const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HALSTEAD_METRICS_H_ */
