//===- unittest/Format/FormatTest.cpp - Formatting unit tests -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Format/Format.h"

#include "../Tooling/ReplacementTest.h"
#include "FormatTestUtils.h"

#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MemoryBuffer.h"
#include "gtest/gtest.h"

#define DEBUG_TYPE "format-test"

using clang::tooling::ReplacementTest;
using clang::tooling::toReplacements;

namespace clang {
namespace format {
namespace {

FormatStyle getGoogleStyle() { return getGoogleStyle(FormatStyle::LK_Cpp); }

class FormatTestInitializer : public ::testing::Test {
protected:
  enum StatusCheck { SC_ExpectComplete, SC_ExpectIncomplete, SC_DoNotCheck };

  std::string format(llvm::StringRef Code,
                     const FormatStyle &Style = getLLVMStyle(),
                     StatusCheck CheckComplete = SC_ExpectComplete) {
    LLVM_DEBUG(llvm::errs() << "---\n");
    LLVM_DEBUG(llvm::errs() << Code << "\n\n");
    std::vector<tooling::Range> Ranges(1, tooling::Range(0, Code.size()));
    FormattingAttemptStatus Status;
    tooling::Replacements Replaces =
        reformat(Style, Code, Ranges, "<stdin>", &Status);
    if (CheckComplete != SC_DoNotCheck) {
      bool ExpectedCompleteFormat = CheckComplete == SC_ExpectComplete;
      EXPECT_EQ(ExpectedCompleteFormat, Status.FormatComplete)
          << Code << "\n\n";
    }
    ReplacementCount = Replaces.size();
    auto Result = applyAllReplacements(Code, Replaces);
    EXPECT_TRUE(static_cast<bool>(Result));
    LLVM_DEBUG(llvm::errs() << "\n" << *Result << "\n\n");
    return *Result;
  }

  FormatStyle getStyleWithColumns(FormatStyle Style, unsigned ColumnLimit) {
    Style.ColumnLimit = ColumnLimit;
    return Style;
  }

  FormatStyle getLLVMStyleWithColumns(unsigned ColumnLimit) {
    return getStyleWithColumns(getLLVMStyle(), ColumnLimit);
  }

  FormatStyle getGoogleStyleWithColumns(unsigned ColumnLimit) {
    return getStyleWithColumns(getGoogleStyle(), ColumnLimit);
  }

  void verifyFormat(llvm::StringRef Expected, llvm::StringRef Code,
                    const FormatStyle &Style = getLLVMStyle()) {
    EXPECT_EQ(Expected.str(), format(Expected, Style))
        << "Expected code is not stable";
    EXPECT_EQ(Expected.str(), format(Code, Style));
    if (Style.Language == FormatStyle::LK_Cpp) {
      // Objective-C++ is a superset of C++, so everything checked for C++
      // needs to be checked for Objective-C++ as well.
      FormatStyle ObjCStyle = Style;
      ObjCStyle.Language = FormatStyle::LK_ObjC;
      EXPECT_EQ(Expected.str(), format(test::messUp(Code), ObjCStyle));
    }
  }

  void verifyFormat(llvm::StringRef Code,
                    const FormatStyle &Style = getLLVMStyle()) {
    verifyFormat(Code, test::messUp(Code), Style);
  }

  void verifyIncompleteFormat(llvm::StringRef Code,
                              const FormatStyle &Style = getLLVMStyle()) {
    EXPECT_EQ(Code.str(),
              format(test::messUp(Code), Style, SC_ExpectIncomplete));
  }

  void verifyGoogleFormat(llvm::StringRef Code) {
    verifyFormat(Code, getGoogleStyle());
  }

  void verifyIndependentOfContext(llvm::StringRef text) {
    verifyFormat(text);
    verifyFormat(llvm::Twine("void f() { " + text + " }").str());
  }

  /// \brief Verify that clang-format does not crash on the given input.
  void verifyNoCrash(llvm::StringRef Code,
                     const FormatStyle &Style = getLLVMStyle()) {
    format(Code, Style, SC_DoNotCheck);
  }

  int ReplacementCount;
};



TEST_F(FormatTestInitializer, ConstructorInitializer_BreakAfterColonAndComma) {
  FormatStyle Style = getLLVMStyle();
  Style.ColumnLimit = 160;
 Style.BinPackParameters = false;
  Style.BreakConstructorInitializers = FormatStyle::BCIS_AfterColon;
  Style.AllowAllConstructorInitializersOnNextLine = false;
  Style.ConstructorInitializerAllOnOneLineOrOnePerLine = true;
   verifyFormat("Constructor() :\n"
               "    aaaaaaaaaaaaaaaaaa(a),\n"
               "    bbbbbbbbbbbbbbbbbbbbb(b) {}",
               Style);

  Style.AllowAllConstructorInitializersOnNextLine = true;
  verifyFormat("Constructor() :\n"
               "    aaaaaaaaaaaaaaaaaa(a), bbbbbbbbbbbbbbbbbbbbb(b) {}",
               Style);


  Style.ConstructorInitializerAllOnOneLineOrOnePerLine = false;
  verifyFormat("Constructor() : a(a), b(b) {}",
               Style);

}

}
}
}
