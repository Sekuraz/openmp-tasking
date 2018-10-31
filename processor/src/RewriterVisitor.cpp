//
// Created by markus on 4/13/18.
//

#include "RewriterVisitor.h"

bool RewriterVisitor::VisitOMPTaskwaitDirective(clang::OMPTaskwaitDirective *taskwait) {
    TheRewriter.InsertTextBefore(taskwait->getLocStart(), "//");
    TheRewriter.InsertTextAfter(taskwait->getLocEnd(), "\ntaskwait();");

    needsHeader = true;
    return true;
}

bool RewriterVisitor::VisitFunctionDecl(FunctionDecl *f) {
    if (f->isMain() && f->hasBody()) {
        // We assume that there is more than one statement in the body
        auto body = cast<CompoundStmt>(f->getBody());
        TheRewriter.InsertTextBefore(body->body_front()->getLocStart(), "setup_tasking();\n");
        if (isa<ReturnStmt>(body->body_back())) {
            TheRewriter.InsertTextBefore(body->body_back()->getLocStart(), "teardown_tasking();\n");
        }
        else {
            llvm::errs() << "There might be a problem with the teardown mechanic in '" << this->FileName;
            llvm::errs() << "'. No return statement found at the end of main().";

            auto end = Lexer::getLocForEndOfToken(body->body_back()->getLocEnd(), 0, TheRewriter.getSourceMgr(),
                                                  TheRewriter.getLangOpts());

            TheRewriter.InsertTextAfter(end, "teardown_tasking();\n");
        }

        this->needsHeader = true;
    }

    return true;
}