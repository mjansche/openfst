// fstsymbols.cc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2010 Google, Inc.
// Author: allauzen@google.com (Cyril Allauzen)
// Modified: jpr@google.com (Jake Ratkiewicz) to use FstClass
//
// \file
// Performs operations (set, clear, relabel) on the symbols table
// attached to the input Fst.
//

#include <fst/script/fst-class.h>
#include <fst/script/script-impl.h>
#include <fst/util.h>

DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_bool(clear_isymbols, false, "Clear input symbol table");
DEFINE_bool(clear_osymbols, false, "Clear output symbol table");
DEFINE_string(relabel_ipairs, "", "Input relabel pairs (numeric)");
DEFINE_string(relabel_opairs, "", "Output relabel pairs (numeric)");
DEFINE_bool(allow_negative_labels, false,
            "Allow negative labels (not recommended; may cause conflicts)");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::SymbolTable;

  string usage = "Performs operations (set, clear, relabel) on the symbol"
      "tables attached to an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_fname = argc > 1 && strcmp(argv[1], "-") != 0 ? argv[1] : "";
  string out_fname = argc > 2 ? argv[2] : "";

  s::FstClass *ifst = s::FstClass::Read(in_fname);
  if (!ifst) return 1;

  s::MutableFstClass *ofst = 0;
  if (ifst->Properties(fst::kMutable, false)) {
    ofst = static_cast<s::MutableFstClass *>(ifst);
  } else {
    ofst = new s::VectorFstClass(*ifst);
    delete ifst;
  }

  if (FLAGS_clear_isymbols)
    ofst->SetInputSymbols(0);
  else if (!FLAGS_isymbols.empty())
    ofst->SetInputSymbols(
        SymbolTable::ReadText(FLAGS_isymbols, FLAGS_allow_negative_labels));

  if (FLAGS_clear_osymbols)
    ofst->SetOutputSymbols(0);
  else if (!FLAGS_osymbols.empty())
    ofst->SetOutputSymbols(
        SymbolTable::ReadText(FLAGS_osymbols, FLAGS_allow_negative_labels));

  if (!FLAGS_relabel_ipairs.empty()) {
    typedef int64 Label;
    vector<pair<Label, Label> > ipairs;
    fst::ReadLabelPairs(FLAGS_relabel_ipairs, &ipairs,
                            FLAGS_allow_negative_labels);
    SymbolTable *isyms = RelabelSymbolTable(ofst->InputSymbols(), ipairs);
    ofst->SetInputSymbols(isyms);
    delete isyms;
  }

  if (!FLAGS_relabel_opairs.empty()) {
    typedef int64 Label;
    vector<pair<Label, Label> > opairs;
    fst::ReadLabelPairs(FLAGS_relabel_opairs, &opairs,
                            FLAGS_allow_negative_labels);
    SymbolTable *osyms = RelabelSymbolTable(ofst->OutputSymbols(), opairs);
    ofst->SetOutputSymbols(osyms);
    delete osyms;
  }

  ofst->Write(out_fname);
  return 0;
}
