//
// Created by markus on 04.11.18.
//

#include "ExtractorVisitor.h"
#include "VisitorBase.h"
#include "clang/Lex/Lexer.h"


using namespace std;
using namespace clang;

VisitorBase::VisitorBase(clang::Rewriter &R) : TheRewriter(R) {}

string VisitorBase::getSourceCode(const clang::Stmt& stmt) {
    auto sr = stmt.getSourceRange();
    sr.setEnd(Lexer::getLocForEndOfToken(sr.getEnd(), 0, TheRewriter.getSourceMgr(), TheRewriter.getLangOpts()));

    auto source = TheRewriter.getRewrittenText(sr);

    return source;
}