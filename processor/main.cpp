#include <experimental/filesystem>

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
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#include "ExtractorVisitor.h"

using namespace clang;
using namespace std;
namespace fs = std::experimental::filesystem;



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
        if (Visitor.needsHeader) {
            Visitor.TheRewriter.InsertTextBefore((*DR.begin())->getLocStart(),
                                                 "#include \"" + fs::current_path().string() + "/processor/tasking.h\"\n");
        }
        return true;
    }

private:
    ExtractorVisitor Visitor;
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

    if (argc == 2 && !hasInvocation) {
        FileIn = FileMgr.getFile(argv[1]);
    } else {
        if (TheCompInst.getInvocation().getFrontendOpts().Inputs.empty()) {
            FileIn = FileMgr.getFile("test.cpp");
        }
        else {
            FileIn = FileMgr.getFile(TheCompInst.getInvocation().getFrontendOpts().Inputs[0].getFile());
        }
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

    const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());

    llvm::outs() << string(RewriteBuf->begin(), RewriteBuf->end());

    return 0;
}
