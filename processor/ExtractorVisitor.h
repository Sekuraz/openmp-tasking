//
// Created by markus on 4/11/18.
//

#ifndef PROCESSOR_EXTRACTORVISITOR_H
#define PROCESSOR_EXTRACTORVISITOR_H

#include <string>
#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"


// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class ExtractorVisitor : public clang::RecursiveASTVisitor<ExtractorVisitor>
{
public:
    bool needsHeader;
    clang::Rewriter &TheRewriter;

private:
    std::vector<std::string> handled_vars;
    std::vector<std::string> generated_ids;
    std::hash<std::string> hasher;
    std::stringstream out;
    std::string FileName;

public:
    explicit ExtractorVisitor(clang::Rewriter &R);

    ~ExtractorVisitor();

    bool VisitFunctionDecl(clang::FunctionDecl *f);

    bool VisitOMPTaskDirective(clang::OMPTaskDirective* task);

    template<typename ClauseType>
    void extractConditionClause(const clang::OMPTaskDirective& task, const std::string& property_name);

    template<typename ClauseType>
    void extractAccessClause(const clang::OMPTaskDirective& task, const std::string& access_name);

    std::string getSourceCode(const clang::Stmt& stmt);

    void insertVarCapture(const clang::ValueDecl& var, const clang::SourceLocation& destination,
                          const std::string& access_name);

    size_t getVarSize(const clang::ValueDecl& var);
};

#endif //PROCESSOR_EXTRACTORVISITOR_H
