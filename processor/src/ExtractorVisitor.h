//
// Created by markus on 4/11/18.
//

#ifndef PROCESSOR_EXTRACTORVISITOR_H
#define PROCESSOR_EXTRACTORVISITOR_H

#include <string>
#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "VisitorBase.h"

/**
 * Extracts all the rewritten code from the @RewriterVisitor and rewrites task definitions.
 * This runs extra because inner code might be changed during the rewrite step.
 */
class ExtractorVisitor : public VisitorBase, public clang::RecursiveASTVisitor<ExtractorVisitor>
{

private:
    /**
     * Temporary list which stores which of the captured variables are already handled
     * Has to be reset for every captured task
     */
    std::vector<std::string> handled_vars;

    /**
     * Generated ids of all task codes in this compilation unit
     * They are also used to generate the names of the static init functions
     */
    std::vector<std::string> generated_ids;

    /**
     * Hash instance which is used to compute the hashes of the generated_ids
     */
    std::hash<std::string> hasher;

    /**
     * The additional header file with all extracted and boilerplate code
     */
    std::stringstream out;

public:
    /**
     * Construct the ExtractorVisitor and start the boilerplate code
     * @param R The Rewriter for the current compilation unit
     * @param main_file the name of the input file
     */
    ExtractorVisitor(clang::Rewriter &R, std::string main_file);

    /**
     * Finish all code to a usable state and write it to disk
     */
    void finalize();

    /**
     * Visit each OMP task directive in turn and rewrites it.
     * Inner tasks are currently handled afterwards and are thus not rewritten in the resulting code.
     * @param task The current task
     * @return Whether or not to recurse into the function (always true)
     */
    bool VisitOMPTaskDirective(clang::OMPTaskDirective* task);

    /**
     * Extract a conditional clause like if, final etc. if there is one.
     * @tparam ClauseType The clang type of the clause (e.g. OMPIfClause)
     * @param task The task which should be inspected
     * @param property_name The property name in the task struct
     */
    template<typename ClauseType>
    void extractConditionClause(const clang::OMPTaskDirective& task, const std::string& property_name);

    /**
     * Extract an access clause like private, shared etc. if there is one.
     * Directly calls @insertVarCapture with the correct access value.
     * Here the list of already handled variable is important, because at the end of the rewrite process all captured
     * variables are looked at.
     * @tparam ClauseType The clang type of the clause (e.g. OMPPrivateClause)
     * @param task The task which should be inspected
     * @param property_name The property name in the task struct
     */
    template<typename ClauseType>
    void extractAccessClause(const clang::OMPTaskDirective& task, const std::string& access_name);

    /**
     * Insert a variable capture for the given variable if it is not yet processed (see @handled_vars)
     * Also tries to get to the exact memory location of the variable and reconstructs the pointer structure afterwards
     * @param var The variable which should be extracted
     * @param destination The destination at which the variable capture should be inserted in the original file
     * @param access_type The sharing type of this variable (One of "private", "firstprivate", "shared" or "default")
     */
    void insertVarCapture(const clang::ValueDecl& var, const clang::SourceLocation& destination,
                          const std::string& access_type);

    /**
     * Try to get the size of a given variable capture by looking at the type
     * @param var The variable which size should be determined
     * @return The calculated size or 0 if it cannot be determined, e.g. if it is a pointer which might point into an array
     */
    size_t getVarSize(const clang::ValueDecl& var);
};

#endif //PROCESSOR_EXTRACTORVISITOR_H
