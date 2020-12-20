// mains.h
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
//
// This convenience file includes all Fst main inl.h files.
//

#ifndef FST_MAINS_H__
#define FST_MAINS_H__

#include "fst/bin/arcsort-main.h"
#include "fst/bin/closure-main.h"
#include "fst/bin/compile-main.h"
#include "fst/bin/compose-main.h"
#include "fst/bin/concat-main.h"
#include "fst/bin/connect-main.h"
#include "fst/bin/convert-main.h"
#include "fst/bin/determinize-main.h"
#include "fst/bin/difference-main.h"
#include "fst/bin/epsnormalize-main.h"
#include "fst/bin/equivalent-main.h"
#include "fst/bin/encode-main.h"
#include "fst/bin/info-main.h"
#include "fst/bin/intersect-main.h"
#include "fst/bin/invert-main.h"
#include "fst/bin/map-main.h"
#include "fst/bin/minimize-main.h"
#include "fst/bin/print-main.h"
#include "fst/bin/project-main.h"
#include "fst/bin/prune-main.h"
#include "fst/bin/push-main.h"
#include "fst/bin/randgen-main.h"
#include "fst/bin/relabel-main.h"
#include "fst/bin/replace-main.h"
#include "fst/bin/reverse-main.h"
#include "fst/bin/reweight-main.h"
#include "fst/bin/rmepsilon-main.h"
#include "fst/bin/shortest-distance-main.h"
#include "fst/bin/shortest-path-main.h"
// #include "fst/bin/synchronize-main.h"
#include "fst/bin/topsort-main.h"
#include "fst/bin/union-main.h"

namespace fst {

// Invokes REGISTER_FST_MAIN on all mains.
template <class A> void RegisterFstMains() {
  REGISTER_FST_MAIN(ArcSortMain, A);
  REGISTER_FST_MAIN(ClosureMain, A);
  REGISTER_FST_MAIN(CompileMain, A);
  REGISTER_FST_MAIN(ComposeMain, A);
  REGISTER_FST_MAIN(ConcatMain, A);
  REGISTER_FST_MAIN(ConnectMain, A);
  REGISTER_FST_MAIN(ConvertMain, A);
  REGISTER_FST_MAIN(DeterminizeMain, A);
  REGISTER_FST_MAIN(DifferenceMain, A);
  REGISTER_FST_MAIN(EquivalentMain, A);
  REGISTER_FST_MAIN(EncodeMain, A);
  REGISTER_FST_MAIN(InfoMain, A);
  REGISTER_FST_MAIN(IntersectMain, A);
  REGISTER_FST_MAIN(InvertMain, A);
  REGISTER_FST_MAIN(MapMain, A);
  REGISTER_FST_MAIN(MinimizeMain, A);
  REGISTER_FST_MAIN(PrintMain, A);
  REGISTER_FST_MAIN(ProjectMain, A);
  REGISTER_FST_MAIN(PruneMain, A);
  REGISTER_FST_MAIN(PushMain, A);
  REGISTER_FST_MAIN(RandGenMain, A);
  REGISTER_FST_MAIN(RelabelMain, A);
  REGISTER_FST_MAIN(ReplaceMain, A);
  REGISTER_FST_MAIN(ReverseMain, A);
  REGISTER_FST_MAIN(ReweightMain, A);
  REGISTER_FST_MAIN(RmEpsilonMain, A);
  REGISTER_FST_MAIN(ShortestDistanceMain, A);
  REGISTER_FST_MAIN(ShortestPathMain, A);
  // REGISTER_FST_MAIN(SynchronizeMain, A);
  REGISTER_FST_MAIN(TopSortMain, A);
  REGISTER_FST_MAIN(UnionMain, A);
}

// Convenience macro to invoking RegisterFstMains.
#define REGISTER_FST_MAINS(A) fst::RegisterFstMains<A>();

}

#endif  // FST_MAINS_H__
