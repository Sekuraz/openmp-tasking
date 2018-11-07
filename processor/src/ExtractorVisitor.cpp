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


ExtractorVisitor::ExtractorVisitor(Rewriter &R, string main_file)
    : VisitorBase(R, main_file)
{
    string f(FileName);
    std::replace(f.begin(), f.end(), '.', '_');

    out << "#ifndef __" << f << "__" << endl;
    out << "#define __" << f << "__" << endl;
    out << "#include <map>" << endl;
    out << "extern std::map<int, void (*)(void **)> tasking_function_map;" << endl;
}

void ExtractorVisitor::finalize() {
    if (!generated_ids.empty()) {
        out << "int setup_" << generated_ids[0] << "() {" << endl;
        for (auto &id : generated_ids) {
            out << "    tasking_function_map[" << id << "] = &x_" << id << ";" << endl;
        }
        out << "    return 1;" << endl << "}" << endl << endl;
        out << "int tasking_setup_" << generated_ids[0] << " = setup_" << generated_ids[0] << "();" << endl;

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

bool ExtractorVisitor::VisitOMPTaskDirective(OMPTaskDirective* task) {
    auto *code = task->getCapturedStmt(task->getDirectiveKind());

    this->needsHeader = true;
    this->handled_vars.clear();

    // This is also the code id of the task
    stringstream hash_stream;
    unsigned long long hash_long = hasher(task->getLocStart().printToString(TheRewriter.getSourceMgr()));
    int hash_int = hash_long & INT_MAX;
    hash_stream << hash_int;
    hash = hash_stream.str();

    TheRewriter.InsertTextBefore(task->getLocStart(), "//");
    TheRewriter.InsertTextAfterToken(task->getLocEnd(), "\nauto t_" + hash + " = std::make_shared<Task>(" + hash + ");\n");

    // start the function which is executed remotely
    out << "void x_" << hash << " (void* arguments[]) {" << endl;

    this->extractConditionClause<OMPIfClause>(*task, "if_clause");
    this->extractConditionClause<OMPFinalClause>(*task, "final");

    if (task->hasClausesOfKind<OMPUntiedClause>()) {
        TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t_" + hash + "->untied = true;\n");
    }
    if (task->hasClausesOfKind<OMPDefaultClause>()) {
        auto clause = task->getSingleClause<OMPDefaultClause>();
        auto kind = clause->getDefaultKind();

        if (kind != OMPC_DEFAULT_shared) {
            TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t_" + hash + "->shared_by_default = false;\n");
        }
    }
    if (task->hasClausesOfKind<OMPMergeableClause>()) {
        TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t_" + hash + "->mergeable = true;\n");
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

                    string c = "t_" + hash + "->" + dep_type + ".emplace_back(\"" + name.str() + "\");\n";
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

        TheRewriter.InsertTextAfterToken(task->getLocEnd(), "t_" + hash + "->priority = (" + source + ");\n");
    }

    for (auto var : code->captures()) {
        this->insertVarCapture(*var.getCapturedVar(), task->getLocEnd(), "default");
    }

    // replace the source code of the task with a call to the schedule function of the generated task struct
    auto source = this->getSourceCode(*code);
    TheRewriter.ReplaceText(code->getLocStart(), (unsigned int) source.length(), "current_task->worker.lock()->handle_create_task(t_" + hash + ");");

    if (source.back() != '}') { // single line
        source = "{ " + source + "; }";
    }

    // variable unpacking is done by the insertVarCapture function
    out << "    " << source;
    out << endl << "}" << endl;

    generated_ids.emplace_back(hash);

    return true;
}

template<typename ClauseType>
void ExtractorVisitor::extractConditionClause(const OMPTaskDirective& task, const string& property_name) {
    if (task.hasClausesOfKind<ClauseType>()) {

        auto clause = task.getSingleClause<ClauseType>();
        auto cond = clause->getCondition();

        auto source = this->getSourceCode(*cond);

        if (source.front() != '(' && source.back() == ')') {
            source.pop_back();
        }

        TheRewriter.InsertTextAfterToken(task.getLocEnd(), "t_" + hash + "->" + property_name + " = (" + source + ");\n");
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

void ExtractorVisitor::insertVarCapture(const ValueDecl& var, const SourceLocation& destination, const string& access_type) {

    string name = var.getName().str(), type = var.getType().getAsString();

    auto canon = var.getType().getCanonicalType();
    int layers = 0;

    while (isa<PointerType>(canon)) {
        auto t = cast<PointerType>(canon);
        canon = t->getPointeeType();
        layers++;
    }

    if (find(this->handled_vars.begin(), this->handled_vars.end(), name) == this->handled_vars.end()) {
        stringstream capture;
        capture << "{\n\tVar " << name << "_var = {\"" << name << "\", ";

        capture << "&(";
        for (int i = 0; i < layers; i++) {
            capture << "*";
        }
        capture << name << "), at_" << access_type << ", " << getVarSize(var) << "};";
        capture << "\n\tt_" + hash + "->vars.emplace_back(" << name << "_var);\n}\n";
        TheRewriter.InsertTextAfterToken(destination, capture.str());

        out << "    void * " << name << "_pointer_" << layers <<" = arguments[" << this->handled_vars.size() << "];" << endl;


        for (; layers > 0; layers--)
        {
            out << "    void * " << name << "_pointer_" << layers - 1 << " = &(";
            out << name << "_pointer_" << layers << ");" << endl;
        }
        out << "    " << type << " " << name << " = *((" << type << "*) " << name << "_pointer_0);" << endl;
        this->handled_vars.emplace_back(name);
    }
}

/**
 * Helper function for getVarSize
 * @param type A type which can be casted to a ConstantArrayType
 * @return the size of the array
 */
size_t getArraySize(const QualType t, const ASTContext& c) {
    auto array = cast<ConstantArrayType>(t);
    auto size = array->getSize().getLimitedValue();

    return c.getTypeInfo(array->getElementType()).Width / 8 * size;
}

size_t ExtractorVisitor::getVarSize(const ValueDecl& var) {
    auto t = var.getType();

    size_t return_value = 0; // the runtime decides

    if (isa<BuiltinType>(t)) {
        return_value = var.getASTContext().getTypeInfo(t).Width / 8;
    }
    else if (isa<PointerType>(t)) {
        // This might also be a pointer into an array defined in another compilation unit, then the calculation would be wrong
//        auto pt = cast<PointerType>(t);
//        return_value = var.getASTContext().getTypeInfo(pt->getPointeeOrArrayElementType()).Width / 8;
    }
    else if (isa<DecayedType>(t)) {
        // If we know that it is a pointer to an array
        auto dt = cast<DecayedType>(t);
        auto source = dt->getOriginalType();
        if (isa<ConstantArrayType>(source)) {
            return_value = getArraySize(source, var.getASTContext());
        }
        else {
            // This should be unreachable because a decayed pointer which is only a pointer doesn't make any sense
            llvm::errs() << "Passing pointers is not supported.";
            llvm::errs().flush();
            exit(1);
        }
    }
    else if (isa<ConstantArrayType>(t)) {
        return_value = getArraySize(t, var.getASTContext());
    }
    else {
        // Missing detailed implementation, warn about it
        t.dump();
        llvm::errs() << "Unknown size of the type above.";
        llvm::errs().flush();
    }

    if (return_value >= UINT64_MAX / 2) {
        t.dump();
        llvm::errs() << "The array is most likely larger than the supported RAM for a 64 bit machine.";
        llvm::errs().flush();
        exit(1);
    }

    return return_value;
}
