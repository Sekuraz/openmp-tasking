#include <algorithm>
#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <sys/stat.h>


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
    string headerName;
    ofstream out;

private:
    Rewriter &TheRewriter;
    vector<string> handled_vars;
    hash<string> hasher;


public:
    MyASTVisitor(Rewriter &R)
        : TheRewriter(R), needsHeader(false)
    {
        stringstream name;
        name << "/tmp/tasking_functions/";
        mkdir(name.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        name << R.getSourceMgr().getFileEntryForID(R.getSourceMgr().getMainFileID())->getName().str();
        name << ".hpp";
        headerName = name.str();
        out = ofstream(headerName);
        out << "#include <cstdarg>" << endl << endl;
    }

    ~MyASTVisitor() {
        out.close();
    }

    bool VisitOMPTaskDirective(OMPTaskDirective* task) {
        auto *code = task->getCapturedStmt(task->getDirectiveKind());

        this->needsHeader = true;
        this->handled_vars.clear();

        TheRewriter.InsertTextBefore(task->getLocStart(), "//");
        TheRewriter.InsertTextAfterToken(task->getLocEnd(), "\nTask t(1);\n");

        auto hash = hasher(task->getLocStart().printToString(TheRewriter.getSourceMgr()));
        out << "void x_" << hash << " (int pseudo, ...) {" << endl;
        out << "va_list args;" << endl << "va_start(args, pseudo);" << endl;

        this->extractConditionClause<OMPIfClause>(*task, "if_clause");
        this->extractConditionClause<OMPFinalClause>(*task, "final");

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

        this->extractAccessClause<OMPPrivateClause>(*task, "private");
        this->extractAccessClause<OMPFirstprivateClause>(*task, "firstprivate");
        this->extractAccessClause<OMPSharedClause>(*task, "shared");

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
            auto cond = clause->getPriority();

            auto source = this->getSourceCode(*cond);

            TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t.priority = (" + source + ");\n");
        }

        for (auto var : code->captures()) {
            this->insertVarCapture(*var.getCapturedVar(), task->getLocEnd(), "default");
        }

        auto source = this->getSourceCode(*code);

        if (source.back() != '}') { // single line
            source = "{ " + source + "; }";
        }

        out << source;
        out << endl << "}" << endl;

        //task->dump();

        return true; // Is true correct?
    }

    template<typename ClauseType>
    void extractConditionClause(const OMPTaskDirective& task, const string& property_name) {
        if (task.hasClausesOfKind<ClauseType>()) {

            auto clause = task.getSingleClause<ClauseType>();
            auto cond = clause->getCondition();

            auto source = this->getSourceCode(*cond);

            TheRewriter.InsertTextAfterToken(task.getLocEnd(), "t." + property_name + " = (" + source + ");\n");
        }
    }

    template<typename ClauseType>
    void extractAccessClause(const OMPTaskDirective& task, const string& access_name) {
         if (task.hasClausesOfKind<ClauseType>()) {

            auto clause = task.getSingleClause<ClauseType>();
            for (auto &&pc : clause->varlists()) {
                if (isa<DeclRefExpr>(pc)) {
                    auto expr = cast<DeclRefExpr>(pc);
                    this->insertVarCapture(*expr->getDecl(), task.getLocEnd(), access_name);
                } else {
                    llvm::errs() << "don't know how to handle a non DeclRef in a " + access_name + " clause, exiting.";
                    llvm::errs().flush();
                    exit(1);
                }
            }
        }
    }

    string getSourceCode(const Stmt& stmt) {
        auto sr = stmt.getSourceRange();
        sr.setEnd(Lexer::getLocForEndOfToken(sr.getEnd(), 0, TheRewriter.getSourceMgr(), TheRewriter.getLangOpts()));

        auto source = Lexer::getSourceText(CharSourceRange::getCharRange(sr), TheRewriter.getSourceMgr(),
                                           TheRewriter.getLangOpts());

        return source.str();
    }

    void insertVarCapture(const ValueDecl& var, const SourceLocation& destination, const string& access_name) {

        string name = var.getName().str(), type = var.getType().getAsString();

        if (find(this->handled_vars.begin(), this->handled_vars.end(), name) == this->handled_vars.end()) {
            string c = "{\n\tVar " + name + "_var = {\"" + name + "\", &" + name + ", at_" + access_name + "};";
            c += "\n\tt.vars.push_back(" + name + "_var);\n}\n";
            TheRewriter.InsertTextAfterToken(destination, c);

            out << "void * " << name << "_pointer = va_arg(args, void *);" << endl;
            out << type << " " << name << " = *((" << type << "*) " << name << "_pointer);" << endl;
            this->handled_vars.push_back(name);
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


int main(int argc, const char *argv[]) {
    /*
    if (argc > 2) {
        llvm::errs() << "Usage: rewritersample <filename>\n";
        llvm::errs() << "Default: filename = '/tmp/t.cpp'\n";
        return 1;
    }
     */

    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    CompilerInstance TheCompInst;
    TheCompInst.createDiagnostics();

    bool hasInvocation = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            hasInvocation = true;
            auto invocation = make_shared<CompilerInvocation>();
            CompilerInvocation::CreateFromArgs(*invocation, argv + i + 1, argv + argc, TheCompInst.getDiagnostics());
            TheCompInst.setInvocation(invocation);
            break;
        }
    }

    if (!hasInvocation) {
        TheCompInst.getInvocation().getLangOpts()->OpenMP = 1;
    }

    TheCompInst.getInvocation().getLangOpts()->CPlusPlus = 1;


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
        FileIn = FileMgr.getFile("test.cpp");
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
