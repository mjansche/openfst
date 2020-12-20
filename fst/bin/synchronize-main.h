// synchronize-main.h
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
// Synchronizes an FST. Includes helper function for fstsynchronize.cc
// that templates the main on the arc type to support multiple and
// extensible arc types.

#ifndef FST_SYNCHRONIZE_MAIN_H__
#define FST_SYNCHRONIZE_MAIN_H__

#include "fst/bin/main.h"
#include "fst/lib/synchronize.h"
#include "fst/lib/vector-fst.h"

namespace fst {
// Main function for fstsynchronize templated on the arc type.
template <class Arc>
int SynchronizeMain(int argc, char **argv, istream &strm,
             const FstReadOptions &iopts) {
  typedef typename Arc::Weight Weight;

  Fst<Arc> *ifst = Fst<Arc>::Read(strm, iopts);
  if (!ifst) return 1;

  VectorFst<Arc> ofst;
  Synchronize(*ifst, &ofst);
  ofst.Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_SYNCHRONIZE_MAIN_H__
