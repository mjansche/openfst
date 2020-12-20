// equal-main.h
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
// Author: riley@google.com (Michael Riley)
//
// \file
// Two FSTs are equal iff their exit status is zero.  Includes
// helper function for fstequal.cc that templates the main on the
// arc type to support multiple and extensible arc types.

#ifndef FST_EQUAL_MAIN_H__
#define FST_EQUAL_MAIN_H__

#include "fst/lib/equal.h"
#include "fst/bin/main.h"
#include "fst/lib/vector-fst.h"

namespace fst {

// Main function for fstequal templated on the arc type.
template <class Arc>
int EqualMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  Fst<Arc> *ifst1 = Fst<Arc>::Read(strm, opts);
  if (!ifst1) return 1;

  Fst<Arc> *ifst2 = Fst<Arc>::Read(argv[2]);
  if (!ifst2) return 1;

  bool result = Equal(*ifst1, *ifst2);
  if (!result)
    VLOG(1) << "FSTs are not equal";
  return  result ? 0 : 2;
}

}  // namespace fst

#endif  // FST_EQUAL_MAIN_H__
