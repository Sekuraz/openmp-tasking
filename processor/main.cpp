#include <algorithm>
#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sstream>


#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#include "tasking.h"


using namespace clang;
using namespace std;



// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
    bool needsHeader;

private:
    Rewriter &TheRewriter;
    set<string> handled_vars;

public:
    MyASTVisitor(Rewriter &R)
        : TheRewriter(R), needsHeader(false)
    {}

    bool VisitOMPTaskDirective(OMPTaskDirective* task) {
        auto *code = task->getCapturedStmt(task->getDirectiveKind());

        this->needsHeader = true;
        this->handled_vars.clear();

        TheRewriter.InsertTextBefore(task->getLocStart(), "//");
        TheRewriter.InsertTextAfterToken(task->getLocEnd(), "\nTask t(1);\n");


        if (task->hasClausesOfKind<OMPIfClause>()) {
            auto clause = task->getSingleClause<OMPIfClause>();
            auto cond = clause->getCondition();

            auto source = this->getSourceCode(*cond);

            TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t.if_clause = (" + source + ");\n");
        }
        if (task->hasClausesOfKind<OMPFinalClause>()) {
            auto clause = task->getSingleClause<OMPFinalClause>();
            auto cond = clause->getCondition();

            auto source = this->getSourceCode(*cond);

            TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t.final = (" + source + ");\n");
        }
        if (task->hasClausesOfKind<OMPUntiedClause>()) {
            TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t.untied = true;\n");
        }
        if (task->hasClausesOfKind<OMPDefaultClause>()) {
            auto clause = task->getSingleClause<OMPDefaultClause>();
            auto kind = clause->getDefaultKind();

            if (kind != OMPC_DEFAULT_shared) {
                TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t.shared_by_default = false;\n");
            }
        }
        if (task->hasClausesOfKind<OMPMergeableClause>()) {
            TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t.mergeable = true;\n");
        }
        if (task->hasClausesOfKind<OMPPrivateClause>()) {
            auto clause = task->getSingleClause<OMPPrivateClause>();
            for (auto &&pc : clause->private_copies()) {
                if (isa<DeclRefExpr>(pc)) {
                    auto expr = cast<DeclRefExpr>(pc);
                    this->insertVarCapture(*expr->getDecl(), task->getLocEnd(), at_private);
                } else {
                    llvm::errs() << "don't know how to handle a non DeclRef in a private clause, exiting.";
                    llvm::errs().flush();
                    exit(1);
                }
            }
        }
        if (task->hasClausesOfKind<OMPFirstprivateClause>()) {
            auto clause = task->getSingleClause<OMPFirstprivateClause>();
            for (auto &&pc : clause->private_copies()) {
                if (isa<DeclRefExpr>(pc)) {
                    auto expr = cast<DeclRefExpr>(pc);
                    this->insertVarCapture(*expr->getDecl(), task->getLocEnd(), at_firstprivate);
                } else {
                    llvm::errs() << "don't know how to handle a non DeclRef in a firstprivate clause, exiting.";
                    llvm::errs().flush();
                    exit(1);
                }
            }
        }
        if (task->hasClausesOfKind<OMPSharedClause>()) {
            auto clause = task->getSingleClause<OMPSharedClause>();
            for (auto &&pc : clause->varlists()) {
                if (isa<DeclRefExpr>(pc)) {
                    auto expr = cast<DeclRefExpr>(pc);
                    this->insertVarCapture(*expr->getDecl(), task->getLocEnd(), at_shared);
                } else {
                    llvm::errs() << "don't know how to handle a non DeclRef in a shared clause, exiting.";
                    llvm::errs().flush();
                    exit(1);
                }
            }
        }
        if (task->hasClausesOfKind<OMPDependClause>()) {
            for (auto &&clause : task->getClausesOfKind<OMPDependClause>()) {
                for (auto &&pc : clause->varlists()) {
                    if (isa<DeclRefExpr>(pc)) {
                        auto expr = cast<DeclRefExpr>(pc);
                        auto name = expr->getDecl()->getName();

                        string dep_type;

                        switch (clause->getDependencyKind()) {
                            case OMPC_DEPEND_in:
                                dep_type = "in";
                                break;
                            case OMPC_DEPEND_out:
                                dep_type = "out";
                                break;
                            case OMPC_DEPEND_inout:
                                dep_type = "inout";
                                break;
                            default:
                                llvm::errs()
                                        << "don't know how to handle a non in/out/inout in depend clause, exiting.";
                                llvm::errs().flush();
                                exit(1);
                        }

                        string c = "t." + dep_type + ".push_back(\"" + name.str() + "\");\n";
                        TheRewriter.InsertTextAfterToken(task->getLocEnd(), c);
                    } else {
                        llvm::errs() << "don't know how to handle a non DeclRef in depend clause, exiting.";
                        llvm::errs().flush();
                        exit(1);
                    }
                }
            }
        }
        if (task->hasClausesOfKind<OMPPriorityClause>()) {
            auto clause = task->getSingleClause<OMPPriorityClause>();

            auto priority = clause->getPriority();

            auto source = this->getSourceCode(*priority);
            TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t.priority = (" + source + ");\n");
        }

        for (auto var : code->captures()) {
            this->insertVarCapture(*var.getCapturedVar(), task->getLocEnd(), at_default);
        }

        auto source = this->getSourceCode(*code);

        llvm::outs() << source << "\n";


        task->dump();

        return true; // Is true correct?
    }

    string getSourceCode(const Stmt& stmt) {
        auto sr = stmt.getSourceRange();
        sr.setEnd(Lexer::getLocForEndOfToken(sr.getEnd(), 0, TheRewriter.getSourceMgr(), TheRewriter.getLangOpts()));

        auto source = Lexer::getSourceText(CharSourceRange::getCharRange(sr), TheRewriter.getSourceMgr(),
                                           TheRewriter.getLangOpts());

        return source.str();
    }

    void insertVarCapture(const NamedDecl& var, const SourceLocation& destination, const access_type at) {
        string at_string;

        switch(at) {
            case at_private:
                at_string = "at_private";
                break;
            case at_firstprivate:
                at_string = "at_firstprivate";
                break;
            case at_shared:
                at_string = "at_shared";
                break;
            default:
                at_string = "at_default";
        }

        auto name = var.getName();

        if (this->handled_vars.find(name.str()) == this->handled_vars.end()) {
            string c = "{\n\tVar " + name.str() + "_var = {\"" + name.str() + "\", &" + name.str() + ", " + at_string + "};";
            c += "\n\tt.vars.push_back(" + name.str() + "_var);\n}\n";
            TheRewriter.InsertTextAfterToken(destination, c);
            this->handled_vars.insert(name.str());
        }
    }
};


// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(Rewriter &R)
        : Visitor(R)
    {}

    // Override the method that gets called for each parsed top-level
    // declaration.
    virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
        for (auto && b: DR) {
            // Traverse the declaration using our AST visitor.
            Visitor.TraverseDecl(b);
        }
        this->needsHeader = Visitor.needsHeader;
        return true;
    }

    bool needsHeader;

private:
    MyASTVisitor Visitor;
};


int main(int argc, char *argv[]) {
    if (argc > 2) {
        llvm::errs() << "Usage: rewritersample <filename>\n";
        llvm::errs() << "Default: filename = '/tmp/t.cpp'\n";
        return 1;
    }

    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    CompilerInstance TheCompInst;
    TheCompInst.createDiagnostics(0, false);

    TheCompInst.getInvocation().getLangOpts()->OpenMP = 1;

    // Initialize target info with the default triple for our platform.
    auto TO = std::make_shared<TargetOptions>();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
    TheCompInst.setTarget(TI);

    TheCompInst.createFileManager();
    FileManager &FileMgr = TheCompInst.getFileManager();
    TheCompInst.createSourceManager(FileMgr);
    SourceManager &SourceMgr = TheCompInst.getSourceManager();
    TheCompInst.createPreprocessor(TU_Complete);
    TheCompInst.createASTContext();

    // A Rewriter helps us manage the code rewriting task.
    Rewriter TheRewriter;
    TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

    // Set the main file handled by the source manager to the input file.
    const FileEntry *FileIn;

    if (argc == 2) {
        FileIn = FileMgr.getFile(argv[1]);

    } else {
        FileIn = FileMgr.getFile("/tmp/t.cpp");
    }

    FileID file = SourceMgr.getOrCreateFileID(FileIn, clang::SrcMgr::CharacteristicKind::C_User);
    SourceMgr.setMainFileID(file);
    TheCompInst.getDiagnosticClient().BeginSourceFile(TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());

    // Create an AST consumer instance which is going to get called by
    // ParseAST.
    MyASTConsumer TheConsumer(TheRewriter);

    // Parse the file to AST, registering our consumer as the AST consumer.
    ParseAST(TheCompInst.getPreprocessor(), &TheConsumer, TheCompInst.getASTContext());

    // At this point the rewriter's buffer should be full with the rewritten
    // file contents.

    llvm::errs().flush();
    llvm::outs().flush();

    if (TheConsumer.needsHeader) {
        ifstream in("processor/tasking.h", std::ios::in );
        if (in) {
            stringstream ss;
            ss << in.rdbuf();
            llvm::outs() << ss.str();
            in.close();
        }
    }

    const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
    llvm::outs() << string(RewriteBuf->begin(), RewriteBuf->end());

    return 0;
}
