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

#include "clang/AST/Stmt.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/Task_parallelKinds.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/AST/Expr.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"
#include "RAIIObjectsForParser.h"

using namespace clang;
bool Parser::ParseTask_parallelDeclaration(DeclSpec &DS)
{
   assert(Tok.is(tok::kw__Task_parallel) && "Not a Task_parallel directive!");
   bool isInvalid = true;
   SourceLocation Loc = ConsumeToken();
   Task_parallelDirectiveKind Dkind = getTask_parallelDirectiveKind(PP.getSpelling(Tok));
   if(Dkind == Task_parallel_Call)
        isInvalid = DS.SetTask_parallelSpawn(Loc);
    return isInvalid;
}

///////////////////////////////////////
/// \brief ParseTask_parallelStatement
/// \return StmtResult
///////////////////////////////////////
StmtResult Parser::ParseTask_parallelStatement(StmtVector &Stmts,
                                                bool OnlyStatement,
                                                ParsedAttributesWithRange &Attrs)
{
    assert(Tok.is(tok::kw__Task_parallel) && "Not a Task_parallel directive!");
    StmtResult Res;
    SourceLocation Loc = ConsumeToken();
    Task_parallelDirectiveKind Dkind = getTask_parallelDirectiveKind(PP.getSpelling(Tok));
    const char *SemiError = 0;
    bool NoCilkPlus = false;
    
    switch(Dkind){
    case Task_parallel_Block:{
            ConsumeToken();
            if (!getLangOpts().CilkPlus) {
                Diag(Tok, diag::err_cilkplus_disable);
                NoCilkPlus = true;
            }
            if (Tok.isNot(tok::l_brace)) {
                Diag(Tok, diag::err_expected_lbrace);
                Res = StmtError();
                break;
            }
            //Create a captured Stmt so that it has its own cilkrts_stack_frame
            SourceLocation Loc = Tok.getLocation();
            ParseScope CapturedRegionScope(this, Scope::FnScope | Scope::DeclScope);
            Actions.ActOnCapturedRegionStart(Loc, getCurScope(), CR_Default,
                                             /*NumParams=*/1);
            StmtResult R = ParseCompoundStatement();
            CapturedRegionScope.Exit();
            if (R.isInvalid()) {
              Actions.ActOnCapturedRegionError();
              return StmtError();
            }
            Res = Actions.ActOnCapturedRegionEnd(R.get());
        }
        break;
    case Task_parallel_Spawn:{
            ConsumeToken();
            if (!getLangOpts().CilkPlus) {
                Diag(Tok, diag::err_cilkplus_disable);
                NoCilkPlus = true;
            }
            StmtResult AssociatedStmt;//Capture the compound stmt with  _Spawn
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
                NoCilkPlus = true;
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
    case Task_parallel_Call:{
            ConsumeToken();
           //FIXME: int a = _Task_parallel _Call spawner(b); is a declaration
           // take care of this case
            {
                ExprResult exp = ParseExpression();
                if(exp.isInvalid()) Res = StmtError();
                else{
                    Expr *E = exp.get();
                    if(CallExpr *CE = dyn_cast<CallExpr>(E)){
                        if(FunctionDecl *FD = dyn_cast<FunctionDecl>(CE->getCalleeDecl())){
                          if(FD->isTask_parallelSpawningFunction()){
                              CE->setTask_parallel_CallLoc(Loc);
                          }else{
                              Res = StmtError();
                              Diag(Tok, diag::err_Function_decl_not_Task_parallel_Call);
                          }
                        }
                    }else{
                        Res = StmtError();
                        Diag(Tok, diag::err_expected_call);
                    }
                    Res = Actions.ActOnTask_parallelCall(exp);
                    if (Tok.is(tok::semi)) {
                      ConsumeToken();
                    }
                }
            }

            if(Res.isInvalid())
                Res = StmtError();
        }
        break;
    case Task_parallel_unknown:
            Diag(Tok, diag::err_Task_parallel_unknown_directive);
        break;
    case NUM_TP_DIRECTIVES:
            Diag(Tok, diag::err_Task_parallel_unexpected_directive);
        break;
    }

    if (NoCilkPlus)
        Res = StmtError();
    return Res;
}
