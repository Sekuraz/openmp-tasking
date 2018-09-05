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