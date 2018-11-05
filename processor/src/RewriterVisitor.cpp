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

        stringstream main_code;
        istringstream main_code_stream(getSourceCode(*body));
        string line;
        while (getline(main_code_stream, line)) {
           if (line.find("return") != string::npos) {
               line = "return;"; // no support for return values
           }
           main_code << line << endl;
        }

        TheRewriter.RemoveText(SourceRange(body->getLocStart(), body->getLocEnd()));
        TheRewriter.InsertTextAfter(body->getLocStart(), "{ setup_tasking(argc, argv); }");


        stringstream out;
        out << "void __main_0(int argc, char *argv[]) {" << endl;
        out << main_code.str();
        out << "}" << endl;

        TheRewriter.InsertTextBefore(f->getLocStart(), out.str());

        this->needsHeader = true;
    }

    return true;
}