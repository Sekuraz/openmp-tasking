//
// Created by markus on 4/11/18.
//

#include "ExtractorVisitor.h"

#include <fstream>
#include <experimental/filesystem>

#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"


using namespace clang;
using namespace std;
namespace fs = std::experimental::filesystem;

ExtractorVisitor::ExtractorVisitor(Rewriter &R)
    : TheRewriter(R), needsHeader(false),
      FileName(fs::path(R.getSourceMgr().getFileEntryForID(R.getSourceMgr().getMainFileID())->getName().str()).filename())
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

ExtractorVisitor::~ExtractorVisitor() {
    llvm::outs() << "hello";
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

bool ExtractorVisitor::VisitFunctionDecl(FunctionDecl *f) {
    if (f->isMain() && f->hasBody()) {
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

bool ExtractorVisitor::VisitOMPTaskDirective(OMPTaskDirective* task) {
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

    return true;
}

template<typename ClauseType>
void ExtractorVisitor::extractConditionClause(const OMPTaskDirective& task, const string& property_name) {
    if (task.hasClausesOfKind<ClauseType>()) {

        auto clause = task.getSingleClause<ClauseType>();
        auto cond = clause->getCondition();

        auto source = this->getSourceCode(*cond);

        llvm::outs() << source;

        TheRewriter.InsertTextAfterToken(task.getLocEnd(), "t." + property_name + " = (" + source + ");\n");
    }
}

template<typename ClauseType>
void ExtractorVisitor::extractAccessClause(const OMPTaskDirective& task, const string& access_name) {
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

string ExtractorVisitor::getSourceCode(const Stmt& stmt) {
    auto sr = stmt.getSourceRange();
    sr.setEnd(Lexer::getLocForEndOfToken(sr.getEnd(), 0, TheRewriter.getSourceMgr(), TheRewriter.getLangOpts()));

    auto source = TheRewriter.getRewrittenText(sr);

    return source;
}

void ExtractorVisitor::insertVarCapture(const ValueDecl& var, const SourceLocation& destination, const string& access_name) {

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

size_t ExtractorVisitor::getVarSize(const ValueDecl& var) {
    auto t = var.getType();

    if (isa<BuiltinType>(t)) {
        return var.getASTContext().getTypeInfo(var.getType()).Width / 8;
    }
    else if (isa<PointerType>(t)) {
//            auto pt = cast<PointerType>(t);
//            return var.getASTContext().getTypeInfo(pt->getPointeeOrArrayElementType()).Width / 8;
        return 0; // let the runtime decide
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
    else if (isa<ConstantArrayType>(t)) {
        auto array = cast<ConstantArrayType>(t);
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
        var.getType().dump();
        // TODO Documentation
        llvm::errs() << "Unknown size of the type above.";
        llvm::errs().flush();
        return 0; // let the runtime decide
    }
}
