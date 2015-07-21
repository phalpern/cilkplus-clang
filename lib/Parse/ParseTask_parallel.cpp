//===--- ParseTask_parallel.cpp - Task_parallel directives parsing ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief This file implements parsing of all Task_paralell constructs.
///
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/Task_parallelKinds.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"
#include "RAIIObjectsForParser.h"

using namespace clang;

///////////////////////////////////////
/// \brief ParseTask_parallelStatement
/// \return StmtResult
///////////////////////////////////////
StmtResult Parser::ParseTask_parallelStatement()
{
    assert(Tok.is(tok::kw__Task_parallel) && "Not a Task_parallel directive!");
    StmtResult Res;
    SourceLocation Loc = ConsumeToken();
    Task_parallelDirectiveKind Dkind = getTask_parallelDirectiveKind(PP.getSpelling(Tok));
    const char *SemiError = 0;

    
    
    switch(Dkind){
    case Task_parallel_Block:
        ConsumeToken();
        break;
    case Task_parallel_Spawn:{
        ConsumeToken();
        if (!getLangOpts().CilkPlus) {
            Diag(Tok, diag::err_cilkplus_disable);
            SkipUntil(tok::r_brace);
            Res = StmtError();
            break;
        }
        StmtResult AssociatedStmt;
        Actions.ActOnCapturedRegionStart(Loc, getCurScope(),CR_CilkSpawn,1);
        Actions.ActOnStartOfCompoundStmt();
        //Parse Stmt
        AssociatedStmt = ParseStatement();
        Actions.ActOnFinishOfCompoundStmt();
        if(AssociatedStmt.isUsable()){
            Res = Actions.ActOnTask_parallelSpawnStmt(AssociatedStmt);
        }else{
            Res = StmtError();
        }
      }
      break;
    case Task_parallel_Sync:
        if (!getLangOpts().CilkPlus) {
            Diag(Tok, diag::err_cilkplus_disable);
            SkipUntil(tok::semi);
            Res = StmtError();
            break;
        }
        Res = Actions.ActOnCilkSyncStmt(ConsumeToken());
        SemiError = "_Task_parallel _Sync";
        if (Tok.is(tok::semi)) {
            ConsumeToken();
        }else if (!Res.isInvalid()) {
            // If the result was valid, then we do want to diagnose this.  Use
            // ExpectAndConsume to emit the diagnostic, even though we know it won't
            // succeed.
            ExpectAndConsume(tok::semi, diag::err_expected_semi_after_stmt, SemiError);
            // Skip until we see a } or ;, but don't eat it.
            SkipUntil(tok::r_brace, StopAtSemi | StopBeforeMatch);
        }
        break;
    case Task_parallel_Call:
        ConsumeToken();
        break;
    case Task_parallel_unknown:
        Diag(Tok, diag::err_Task_parallel_unknown_directive);
        break;
    case NUM_TP_DIRECTIVES:
        Diag(Tok, diag::err_Task_parallel_unexpected_directive);
        break;
    }

    return Res;
}
