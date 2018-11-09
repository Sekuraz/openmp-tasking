//
// Created by markus on 4/13/18.
//

#include <sstream>
#include <fstream>
#include <experimental/filesystem>

#include "clang/Lex/Lexer.h"

#include "RewriterVisitor.h"

using namespace std;
using namespace clang;
namespace fs = std::experimental::filesystem;


bool RewriterVisitor::VisitOMPTaskwaitDirective(clang::OMPTaskwaitDirective *taskwait) {
    TheRewriter.InsertTextBefore(taskwait->getLocStart(), "//");
    TheRewriter.InsertTextAfter(taskwait->getLocEnd(), "\ntaskwait();");

    needsHeader = true;
    return true;
}

bool RewriterVisitor::VisitFunctionDecl(clang::FunctionDecl *f) {
    if (f->isMain() && f->hasBody()) {
        // We assume that there is more than one statement in the body
        auto body = cast<CompoundStmt>(f->getBody());

        TheRewriter.RemoveText(SourceRange(body->getLocStart(), body->getLocEnd()));
        TheRewriter.InsertTextAfter(body->getLocStart(), "{ do_tasking(argc, argv); }");


        stringstream out;
        out << "void __main__1(int argc, char *argv[]) {" << endl;
        out << getSourceCode(*body);
        out << "}" << endl;
        out << "void __main__(int argc, char *argv[]) {" << endl;
        out << "    __main__1(argc, argv);" << endl;
        out << "    current_task->worker->handle_finish_task(current_task);" << endl;
        out << "}" << endl;

        TheRewriter.InsertTextBefore(f->getLocStart(), out.str());

        this->needsHeader = true;
    }

    return true;
}