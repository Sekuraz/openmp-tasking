//
// Created by markus on 4/13/18.
//

#ifndef PROJECT_REWRITERVISITOR_H
#define PROJECT_REWRITERVISITOR_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>

/**
 * Rewrite some OpenMP specific function calls
 * They might be within a task so they should be rewritten first
 */
class RewriterVisitor : public clang::RecursiveASTVisitor<RewriterVisitor>
{
public:
    /**
     * Whether or not the generated code uses the tasking header.
     * This is set to true when any rewrites happened and it is used in main
     */
    bool needsHeader;
    /**
     * The rewriter instance which is used in order to rewrite code.
     */
    clang::Rewriter &TheRewriter;

public:
    explicit RewriterVisitor(clang::Rewriter &R) : needsHeader(false), TheRewriter(R) {}

    /**
     * Visit each OpenMP taskwait directive and rewrite it to use the taskwait function of the tasking header
     * There might be a need to wait for tasks on other nodes
     * @param taskwait The taskwait directive object
     * @return Whether or not to recurse into the function (always true)
     */
    bool VisitOMPTaskwaitDirective(clang::OMPTaskwaitDirective *taskwait);
};


#endif //PROJECT_REWRITERVISITOR_H
