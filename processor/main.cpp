#include <algorithm>
#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
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
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"


using namespace clang;
using namespace std;
namespace fs = std::experimental::filesystem;


// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
    bool needsHeader;

private:
    Rewriter &TheRewriter;
    vector<string> handled_vars;
    vector<string> generated_ids;
    hash<string> hasher;
    stringstream out;
    string FileName;

public:
    MyASTVisitor(Rewriter &R)
        : TheRewriter(R), needsHeader(false),
          FileName(R.getSourceMgr().getFileEntryForID(R.getSourceMgr().getMainFileID())->getName().str())
    {
        string f(FileName);
        std::replace(f.begin(), f.end(), '.', '_');

        out << "#ifndef __" << f << "__" << endl;
        out << "#define __" << f << "__" << endl;
        out << "#ifndef __TASKING_FUNCTION_MAP_GUARD__" <<  endl;
        out << "#define __TASKING_FUNCTION_MAP_GUARD__" <<  endl;
        out << "#include <map>" << endl;
        out << "std::map<unsigned long, void (*)(void **)> tasking_function_map;" << endl;
        out << "#endif" << endl << endl;
    }

    ~MyASTVisitor() {
        if (!generated_ids.empty()) {
            out << "int setup_" << generated_ids[0] << "() {" << endl;
            for (auto &id : generated_ids) {
                out << "    tasking_function_map[" << id << "ull] = &x_" << id << ";" << endl;
            }
            out << "    return 1;" << endl << "}" << endl << endl;
            out << "int tasking_setup = setup_" << generated_ids[0] << "();" << endl;

            stringstream name;
            name << "/tmp/tasking_functions/";
            fs::create_directories(fs::path(name.str()));
            name << FileName;
            name << ".hpp";

            out << "#endif" << endl;

            ofstream file(name.str());
            file << out.str();
            file.close();

            ofstream all("/tmp/tasking_functions/all.hpp", ofstream::app);
            all << "#include \"";
            all << FileName;
            all << ".hpp\"";
            all << endl;
        }
    }

    bool VisitOMPTaskDirective(OMPTaskDirective* task) {
        auto *code = task->getCapturedStmt(task->getDirectiveKind());

        this->needsHeader = true;
        this->handled_vars.clear();

        stringstream hash_stream;
        unsigned long long hash_numeric = hasher(task->getLocStart().printToString(TheRewriter.getSourceMgr()));
        hash_stream << hash_numeric;
        string hash = hash_stream.str();

        TheRewriter.InsertTextBefore(task->getLocStart(), "//");
        TheRewriter.InsertTextAfterToken(task->getLocEnd(), "\nTask t(" + hash + "ull);\n");

        out << "void x_" << hash << " (void* arguments[]) {" << endl;

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
        TheRewriter.ReplaceText(code->getLocStart(), (unsigned int) source.length(), "t.schedule();");

        if (source.back() != '}') { // single line
            source = "{ " + source + "; }";
        }

        out << "    " << source;
        out << endl << "}" << endl;

        generated_ids.push_back(hash);

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

            for (auto && clause : task.getClausesOfKind<ClauseType>()) {
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

        auto canon = var.getType().getCanonicalType();
        int layers = 0;

        while (isa<PointerType>(canon)) {
            auto t = cast<PointerType>(canon);
            canon = t->getPointeeType();
            layers++;
        }

        if (find(this->handled_vars.begin(), this->handled_vars.end(), name) == this->handled_vars.end()) {
            stringstream c;
            c << "{\n\tVar " << name << "_var = {\"" << name << "\", ";

            c << "&(";
            for (int i = 0; i < layers; i++) {
                c << "*";
            }
            c << name << "), at_" << access_name << ", " << getVarSize(var) << "};";
            c << "\n\tt.vars.push_back(" << name << "_var);\n}\n";
            TheRewriter.InsertTextAfterToken(destination, c.str());

            out << "    void * " << name << "_pointer_" << layers <<" = arguments[" << this->handled_vars.size() << "];" << endl;


            for (; layers > 0; layers--)
            {
                out << "    void * " << name << "_pointer_" << layers - 1 << " = &(";
                out << name << "_pointer_" << layers << ");" << endl;
            }
            out << "    " << type << " " << name << " = *((" << type << "*) " << name << "_pointer_0);" << endl;
            this->handled_vars.push_back(name);
        }
    }

    size_t getVarSize(const ValueDecl& var) {
        auto t = var.getType();

        if (isa<BuiltinType>(t)) {
            return var.getASTContext().getTypeInfo(var.getType()).Width / 8;
        }
        else if (isa<PointerType>(t)) {
            auto pt = cast<PointerType>(t);
            return var.getASTContext().getTypeInfo(pt->getPointeeOrArrayElementType()).Width / 8;
        }
        else if (isa<DecayedType>(t)) {
            auto dt = cast<DecayedType>(t);
            auto source = dt->getOriginalType();
            if (isa<ConstantArrayType>(source)) {
                auto array = cast<ConstantArrayType>(source);
                auto size = array->getSize().getLimitedValue();
                if (size == UINT64_MAX) {
                    array->dump();
                    llvm::errs() << "The array is most likely larger than the supported RAM for a 64 bit machine.";
                    llvm::errs().flush();
                    exit(1);
                }

                return var.getASTContext().getTypeInfo(array->getElementType()).Width / 8 * size;
            }
            else {
                // TODO Documentation
                llvm::errs() << "Passing pointers is not supported.";
                llvm::errs().flush();
                exit(1);
            }
        }
        else {
            var.getType().dump();
            // TODO Documentation
            llvm::errs() << "Unknown size of the type above.";
            llvm::errs().flush();
            exit(1);
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
