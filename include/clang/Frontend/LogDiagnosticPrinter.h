//===--- LogDiagnosticPrinter.h - Log Diagnostic Client ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_FRONTEND_LOG_DIAGNOSTIC_PRINTER_H_
#define LLVM_CLANG_FRONTEND_LOG_DIAGNOSTIC_PRINTER_H_

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"

namespace clang {
class DiagnosticOptions;
class LangOptions;

class LogDiagnosticPrinter : public DiagnosticClient {
  struct DiagEntry {
    /// The primary message line of the diagnostic.
    std::string Message;
  
    /// The source file name, if available.
    std::string Filename;
  
    /// The source file line number, if available.
    unsigned Line;
  
    /// The source file column number, if available.
    unsigned Column;
  
    /// The ID of the diagnostic.
    unsigned DiagnosticID;
  
    /// The level of the diagnostic.
    Diagnostic::Level DiagnosticLevel;
  };
  
  llvm::raw_ostream &OS;
  const LangOptions *LangOpts;
  const DiagnosticOptions *DiagOpts;

  SourceLocation LastWarningLoc;
  FullSourceLoc LastLoc;
  unsigned OwnsOutputStream : 1;

  llvm::SmallVector<DiagEntry, 8> Entries;

  std::string MainFilename;
  std::string DwarfDebugFlags;

public:
  LogDiagnosticPrinter(llvm::raw_ostream &OS, const DiagnosticOptions &Diags,
                       bool OwnsOutputStream = false);
  virtual ~LogDiagnosticPrinter();

  void setDwarfDebugFlags(llvm::StringRef Value) {
    DwarfDebugFlags = Value;
  }

  void BeginSourceFile(const LangOptions &LO, const Preprocessor *PP) {
    LangOpts = &LO;
  }

  void EndSourceFile();

  virtual void HandleDiagnostic(Diagnostic::Level DiagLevel,
                                const DiagnosticInfo &Info);
};

} // end namespace clang

#endif
