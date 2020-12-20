// minimize-main.h
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
// Author: johans@google.com (Johan Schalkwyk)
//
// \file
// Minimize a deterministic FSA. Includes helper function for
// fstminimize.cc that templates the main on the arc type to support
// multiple and extensible arc types.

#ifndef FST_MINIMIZE_MAIN_H__
#define FST_MINIMIZE_MAIN_H__

#include "fst/bin/main.h"
#include "fst/lib/minimize.h"
#include "fst/lib/vector-fst.h"

namespace fst {

// Main function for fstminimize templated on the arc type.
template <class Arc>
int MinimizeMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  MutableFst<Arc> *ofst1 = 0;
  if (ifst->Properties(kMutable, false)) {
    ofst1 = down_cast<MutableFst<Arc> *>(ifst);
  } else {
    ofst1 = new VectorFst<Arc>(*ifst);
    delete ifst;
  }
  MutableFst<Arc> *ofst2 = argc > 3 ? new VectorFst<Arc>() : 0;

  Minimize(ofst1, ofst2);
  ofst1->Write(argc > 2 ? argv[2] : "");
  if (ofst2)
    ofst2->Write(argc > 3 ? argv[3] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_MINIMIZE_MAIN_H__
