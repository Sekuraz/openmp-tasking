//
// Created by markus on 04.11.18.
//

#ifndef PROJECT_VISITORBASE_H
#define PROJECT_VISITORBASE_H


#include <clang/Rewrite/Core/Rewriter.h>
#include "clang/AST/RecursiveASTVisitor.h"

class VisitorBase
{
public:
    /**
     * The rewriter instance which is used in order to rewrite code.
     */
   clang::Rewriter &TheRewriter;

    /**
     * The file name of the out stream
     */
    std::string FileName;

    /**
     * Construct the VisitorBase
     * @param R The Rewriter for the current compilation unit
     */
    explicit VisitorBase(clang::Rewriter &R);

   /**
     * Get the code associated with the given statement.
     * Doing this with the clang Rewriter.getRewrittenText (the content might have been changed during the rewriter run)
     * might lead to missing ';'
     * @param stmt The statement which source should be extracted
     * @return THe string containing the source
     */
    std::string getSourceCode(const clang::Stmt& stmt);

    /**
     * Whether or not the generated code uses the tasking header.
     * This is set to true when any rewrites happened and it is used in main
     */
    bool needsHeader;
};


#endif //PROJECT_VISITORBASE_H
