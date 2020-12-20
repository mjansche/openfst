// pdtexpand.cc

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
// Author: riley@google.com (Michael Riley)
// Modified: jpr@google.com (Jake Ratkiewicz) to use FstClass
//
// \file
// Expands a PDT and an FST.
//

#include <fst/extensions/pdt/pdtscript.h>
#include <fst/util.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs.");
DEFINE_bool(connect, true, "Trim output");


int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Expand a PDT and an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  s::FstClass *ifst = s::FstClass::Read(argv[1]);
  if (!ifst) return 1;

  if (FLAGS_pdt_parentheses.empty())
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";

  vector<pair<int64, int64> > parens;
  fst::ReadLabelPairs(FLAGS_pdt_parentheses, &parens, false);

  s::VectorFstClass ofst(ifst->ArcType());
  s::PdtExpand(*ifst, parens, &ofst, FLAGS_connect);

  ofst.Write(argc > 2 ? argv[2] : "");

  return 0;
}
