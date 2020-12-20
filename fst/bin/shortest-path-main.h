// shortest-path-main.h
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
// Find shortest path(s) in an FST.  Includes helper function for
// fstshortestpath.cc that templates the main on the arc type to
// support multiple and extensible arc types.

#ifndef FST_SHORTEST_PATH_MAIN_H__
#define FST_SHORTEST_PATH_MAIN_H__

#include "fst/bin/main.h"
#include "fst/lib/shortest-path.h"
#include "fst/lib/vector-fst.h"

DECLARE_int64(nshortest);

namespace fst {

// Main function for fstshortestpath templated on the arc type.
template <class Arc>
int ShortestPathMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  VectorFst<Arc> ofst;
  ShortestPath(*ifst, &ofst, FLAGS_nshortest);
  ofst.Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_SHORTEST_PATH_MAIN_H__
