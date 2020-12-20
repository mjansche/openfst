// basic-fst.cc
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
// Source for pair arc shared object.

#include "fst/bin/mains.h"
#include "fst/lib/const-fst.h"
#include "fst/lib/vector-fst.h"
#include "fst/test/pair-arc.h"

using fst::VectorFst;
using fst::ConstFst;
using fst::PairArc;

extern "C" {

void pair_arc_init() {
  REGISTER_FST(VectorFst, PairArc);
  REGISTER_FST(ConstFst, PairArc);

  REGISTER_FST_MAINS(PairArc);
}

}
