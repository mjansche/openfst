// fstdraw.cc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: allauzen@cs.nyu.edu (Cyril Allauzen)
//
// \file
// Draws a binary FSTs in the Graphviz dot text format

#include "fst/bin/draw-main.h"

namespace fst {

// Register templated main for common arcs types.
REGISTER_FST_MAIN(DrawMain, StdArc);
REGISTER_FST_MAIN(DrawMain, LogArc);

}  // namespace fst


int main(int argc, char **argv) {
  string usage = "Prints out binary FSTs in simple text format.\n\n  Usage: ";
  usage += argv[0];
  usage += " binary.fst [text.fst]\n";
  usage += "  Flags: acceptor, isymbols, numeric, osymbols, save_isymbols\n";
  usage += "  save_osymbols, ssymbols\n";

  InitFst(usage.c_str(), &argc, &argv, true);
  if (argc < 2 || argc > 3) {
    ShowUsage();
    return 1;
  }

  // Invokes PrintMain<Arc> where arc type is determined from argv[1].
  return CALL_FST_MAIN(DrawMain, argc, argv);
}
