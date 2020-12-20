// replace-main.h
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
// Recursively replaces Fst arcs with other Fst(s).  Includes helper
// function for fstproject.cc that templates the main on the arc type
// to support multiple and extensible arc types.

#ifndef FST_REPLACE_MAIN_H__
#define FST_REPLACE_MAIN_H__

#include "fst/bin/main.h"
#include "fst/lib/replace.h"
#include "fst/lib/vector-fst.h"

namespace fst {

// Main function for fstreplace templated on the arc type.
template <class Arc>
int ReplaceMain(int argc, char **argv, istream &strm,
                const FstReadOptions &opts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  typedef typename Arc::Label Label;
  typedef pair<Label, const Fst<Arc>* > FstTuple;
  vector<FstTuple> fst_tuples;
  Label root = atoi(argv[2]);
  fst_tuples.push_back(make_pair(root, ifst));

  for (size_t i = 3; i < argc - 1; i += 2) {
    ifst = Fst<Arc>::Read(argv[i]);
    if (!ifst) return 1;
    Label lab = atoi(argv[i + 1]);
    fst_tuples.push_back(make_pair(lab, ifst));
  }

  VectorFst<Arc> ofst;
  Replace(fst_tuples, &ofst, root);

  return ofst.Write(argc % 2 == 0 ? argv[argc - 1] : "");
}

}  // namespace fst

#endif  // FST_REPLACE_MAIN_H__
