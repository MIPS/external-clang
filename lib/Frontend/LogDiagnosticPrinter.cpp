//===--- LogDiagnosticPrinter.cpp - Log Diagnostic Printer ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/LogDiagnosticPrinter.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

LogDiagnosticPrinter::LogDiagnosticPrinter(llvm::raw_ostream &os,
                                           const DiagnosticOptions &diags,
                                           bool _OwnsOutputStream)
  : OS(os), LangOpts(0), DiagOpts(&diags),
    OwnsOutputStream(_OwnsOutputStream) {
}

LogDiagnosticPrinter::~LogDiagnosticPrinter() {
  if (OwnsOutputStream)
    delete &OS;
}

static llvm::StringRef getLevelName(Diagnostic::Level Level) {
  switch (Level) {
  default:
    return "<unknown>";
  case Diagnostic::Ignored: return "ignored";
  case Diagnostic::Note:    return "note";
  case Diagnostic::Warning: return "warning";
  case Diagnostic::Error:   return "error";
  case Diagnostic::Fatal:   return "fatal error";
  }
}

void LogDiagnosticPrinter::EndSourceFile() {
  // We emit all the diagnostics in EndSourceFile. However, we don't emit any
  // entry if no diagnostics were present.
  //
  // Note that DiagnosticClient has no "end-of-compilation" callback, so we will
  // miss any diagnostics which are emitted after and outside the translation
  // unit processing.
  if (Entries.empty())
    return;

  // Write to a temporary string to ensure atomic write of diagnostic object.
  llvm::SmallString<512> Msg;
  llvm::raw_svector_ostream OS(Msg);

  OS << "<dict>\n";
  if (!MainFilename.empty()) {
    OS << "  <key>main-file</key>\n"
       << "  <string>" << MainFilename << "</string>\n";
  }
  if (!DwarfDebugFlags.empty()) {
    OS << "  <key>dwarf-debug-flags</key>\n"
       << "  <string>" << DwarfDebugFlags << "</string>\n";
  }
  OS << "  <key>diagnostics</key>\n";
  OS << "  <array>\n";
  for (unsigned i = 0, e = Entries.size(); i != e; ++i) {
    DiagEntry &DE = Entries[i];

    OS << "    <dict>\n";
    OS << "      <key>level</key>\n"
       << "      <string>" << getLevelName(DE.DiagnosticLevel) << "</string>\n";
    if (!DE.Filename.empty()) {
      OS << "      <key>filename</key>\n"
         << "      <string>" << DE.Filename << "</string>\n";
    }
    if (DE.Line != 0) {
      OS << "      <key>line</key>\n"
         << "      <integer>" << DE.Line << "</integer>\n";
    }
    if (DE.Column != 0) {
      OS << "      <key>column</key>\n"
         << "      <integer>" << DE.Column << "</integer>\n";
    }
    if (!DE.Message.empty()) {
      OS << "      <key>message</key>\n"
         << "      <string>" << DE.Message << "</string>\n";
    }
    OS << "    </dict>\n";
  }
  OS << "  </array>\n";
  OS << "</dict>\n";

  this->OS << OS.str();
}

void LogDiagnosticPrinter::HandleDiagnostic(Diagnostic::Level Level,
                                            const DiagnosticInfo &Info) {
  // Default implementation (Warnings/errors count).
  DiagnosticClient::HandleDiagnostic(Level, Info);

  // Initialize the main file name, if we haven't already fetched it.
  if (MainFilename.empty() && Info.hasSourceManager()) {
    const SourceManager &SM = Info.getSourceManager();
    FileID FID = SM.getMainFileID();
    if (!FID.isInvalid()) {
      const FileEntry *FE = SM.getFileEntryForID(FID);
      if (FE && FE->getName())
        MainFilename = FE->getName();
    }
  }

  // Create the diag entry.
  DiagEntry DE;
  DE.DiagnosticID = Info.getID();
  DE.DiagnosticLevel = Level;

  // Format the message.
  llvm::SmallString<100> MessageStr;
  Info.FormatDiagnostic(MessageStr);
  DE.Message = MessageStr.str();

  // Set the location information.
  DE.Filename = "";
  DE.Line = DE.Column = 0;
  if (Info.getLocation().isValid() && Info.hasSourceManager()) {
    const SourceManager &SM = Info.getSourceManager();
    PresumedLoc PLoc = SM.getPresumedLoc(Info.getLocation());

    if (PLoc.isInvalid()) {
      // At least print the file name if available:
      FileID FID = SM.getFileID(Info.getLocation());
      if (!FID.isInvalid()) {
        const FileEntry *FE = SM.getFileEntryForID(FID);
        if (FE && FE->getName())
          DE.Filename = FE->getName();
      }
    } else {
      DE.Filename = PLoc.getFilename();
      DE.Line = PLoc.getLine();
      DE.Column = PLoc.getColumn();
    }
  }

  // Record the diagnostic entry.
  Entries.push_back(DE);
}
