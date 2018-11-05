//
// Created by markus on 4/13/18.
//

#ifndef PROJECT_REWRITERVISITOR_H
#define PROJECT_REWRITERVISITOR_H

#include <clang/Rewrite/Core/Rewriter.h>

#include "VisitorBase.h"

/**
 * Rewrite some OpenMP specific function calls
 * They might be within a task so they should be rewritten first
 */
class RewriterVisitor : public VisitorBase, public clang::RecursiveASTVisitor<RewriterVisitor>
{
public:
    /**
     * Construct the RewriterVisitor
     * @param R The Rewriter for the current compilation unit
     * @param main_file the name of the input file
     */
    explicit RewriterVisitor(clang::Rewriter &R, std::string main_file) : VisitorBase(R, main_file) {}

    /**
     * Visit each OpenMP taskwait directive and rewrite it to use the taskwait function of the tasking header
     * There might be a need to wait for tasks on other nodes
     * @param taskwait The taskwait directive object
     * @return Whether or not to recurse into the function (always true)
     */
    bool VisitOMPTaskwaitDirective(clang::OMPTaskwaitDirective *taskwait);

    /**
     * Visit each function in turn to rewrite the main function
     * @param f The current function
     * @return Whether or not to recurse into the function (always true)
     */
    bool VisitFunctionDecl(clang::FunctionDecl *f);
};


#endif //PROJECT_REWRITERVISITOR_H
