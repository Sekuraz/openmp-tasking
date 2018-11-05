//
// Created by markus on 04.11.18.
//

#include <experimental/filesystem>

#include "clang/Lex/Lexer.h"

#include "VisitorBase.h"


using namespace std;
using namespace clang;
namespace fs = std::experimental::filesystem;


VisitorBase::VisitorBase(clang::Rewriter &R, string filename)
    : TheRewriter(R),
      FileName(filename)
{}

string VisitorBase::getSourceCode(const clang::Stmt& stmt) {
    auto sr = stmt.getSourceRange();
    sr.setEnd(Lexer::getLocForEndOfToken(sr.getEnd(), 0, TheRewriter.getSourceMgr(), TheRewriter.getLangOpts()));

    auto source = TheRewriter.getRewrittenText(sr);

    return source;
}