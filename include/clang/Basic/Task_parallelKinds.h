//===--- Task_parallelKinds.h - Task_parallel enums ---------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines some Task_parallel-specific enums and functions.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_TPKINDS_H
#define LLVM_CLANG_BASIC_TPKINDS_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
namespace clang {

/// \brief Task_parallel directives.
enum Task_parallelDirectiveKind {
  Task_parallel_unknown = 0,
#define TP_DIRECTIVE(Name) \
  Task_parallel##Name,
#include "clang/Basic/Task_parallelKinds.def"
  NUM_TP_DIRECTIVES
#undef TP_DIRECTIVE
};

Task_parallelDirectiveKind getTask_parallelDirectiveKind(llvm::StringRef Str){
    return llvm::StringSwitch<Task_parallelDirectiveKind>(Str)
  #define TP_DIRECTIVE(Name) \
             .Case(#Name, Task_parallel##Name)
  #include "clang/Basic/Task_parallelKinds.def"
             .Default(Task_parallel_unknown);
  #undef TP_DIRECTIVE
  }

const char *getTPDirectiveName(Task_parallelDirectiveKind Kind){
    assert(Kind < NUM_TP_DIRECTIVES);
    switch (Kind) {
    case Task_parallel_unknown:
      return "unknown";
  #define TP_DIRECTIVE(Name) \
    case Task_parallel##Name : return #Name;
  #include "clang/Basic/Task_parallelKinds.def"
    case NUM_TP_DIRECTIVES:
      break;
  #undef TP_DIRECTIVE
    }
    llvm_unreachable("Invalid Task_parallel directive kind");
  }

}

#endif