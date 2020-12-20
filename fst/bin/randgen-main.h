// randgen-main.h
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
// Generates random paths through an FST. Includes helper function for
// fstrandgen.cc that templates the main on the arc type to support
// multiple and extensible arc types.

#ifndef FST_RANDGEN_MAIN_H__
#define FST_RANDGEN_MAIN_H__

#include "fst/bin/main.h"
#include "fst/lib/randgen.h"
#include "fst/lib/vector-fst.h"

DECLARE_int32(max_length);
DECLARE_int32(npath);
DECLARE_int32(seed);

namespace fst {

// Main function for fstrandgen templated on the arc type.
template <class Arc>
int RandGenMain(int argc, char **argv, istream &strm,
              const FstReadOptions &iopts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, iopts);
  if (!ifst) return 1;

  VectorFst<Arc> ofst;

  RandGenOptions< UniformArcSelector<Arc> >
      ropts(UniformArcSelector<Arc>(FLAGS_seed), FLAGS_max_length,
	   FLAGS_npath);
  RandGen(*ifst, &ofst, ropts);
  ofst.Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_RANDGEN_MAIN_H__
