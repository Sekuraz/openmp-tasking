//
// Created by markus on 4/13/18.
//

#ifndef PROJECT_REWRITERVISITOR_H
#define PROJECT_REWRITERVISITOR_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>

class RewriterVisitor : public clang::RecursiveASTVisitor<RewriterVisitor>
{
public:
    bool needsHeader;
    clang::Rewriter &TheRewriter;

public:
    explicit RewriterVisitor(clang::Rewriter &R) : needsHeader(false), TheRewriter(R) {}

    bool VisitOMPTaskwaitDirective(clang::OMPTaskwaitDirective *taskwait);
};


#endif //PROJECT_REWRITERVISITOR_H
