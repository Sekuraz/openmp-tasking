#include <experimental/filesystem>
#include <iostream>
#include <fstream>
#include <cstdio>

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
#include "RewriterVisitor.h"
#include "VisitorBase.h"

using namespace clang;
using namespace std;
namespace fs = std::experimental::filesystem;

/**
 * This class handles traversing the compilation unit by calling the Visitors
 */
class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(Rewriter &R, string main_file)
        : rewriter(R, main_file), extractor(R, main_file)
    {}

    // Override the method that gets called for each parsed top-level
    // declaration.
    virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
        for (auto && b: DR) {
            // Traverse the declaration using our AST visitor.
            rewriter.TraverseDecl(b);
            extractor.TraverseDecl(b);

        }

        if ((rewriter.needsHeader || extractor.needsHeader) && !header_added) {
            extractor.TheRewriter.InsertTextBefore((*DR.begin())->getLocStart(),
                                                 "#include <memory>\n\n");
            extractor.TheRewriter.InsertTextBefore((*DR.begin())->getLocStart(),
                                                 "#include \"tasking.h\"\n");
            extractor.TheRewriter.InsertTextAfter((*DR.begin())->getLocStart(),
                                                 "#include \"/tmp/tasking_functions/all.hpp\"\n");
            header_added = true;
        }
        return true;
    }

    void finalize() {
        extractor.finalize();
    }

private:
    RewriterVisitor rewriter;
    ExtractorVisitor extractor;
    bool header_added = false;
};


int main(int argc, char *argv[]) {

    char * out_file_name = nullptr;

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
        if (strcmp(argv[i], "--outfile") == 0) {
            out_file_name = argv[i + 1];
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

    if ((argc == 2 && !hasInvocation) || out_file_name) {
        FileIn = FileMgr.getFile(argv[1]);
    } else {
        if (TheCompInst.getInvocation().getFrontendOpts().Inputs.empty()) {
            FileIn = FileMgr.getFile("test/simple.cpp");
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
    string file_name = FileIn->getName().str();
    replace(file_name.begin(), file_name.end(), '/', '_');
    MyASTConsumer TheConsumer(TheRewriter, file_name);

    // Parse the file to AST, registering our consumer as the AST consumer.
    ParseAST(TheCompInst.getPreprocessor(), &TheConsumer, TheCompInst.getASTContext());
    TheConsumer.finalize(); //write to disk

    // At this point the rewriter's buffer should be full with the rewritten
    // file contents.

    llvm::errs().flush();
    llvm::outs().flush();

    const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());

    if (out_file_name) {
        ofstream out_file(out_file_name);
        out_file << string(RewriteBuf->begin(), RewriteBuf->end());
        out_file.close();

        llvm::outs() << out_file_name;

    }
    else {
        llvm::outs() << string(RewriteBuf->begin(), RewriteBuf->end());
    }

    return 0;
}
