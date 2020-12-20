
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
// Author: jpr@google.com (Jake Ratkiewicz)

#ifndef FST_SCRIPT_MAP_H_
#define FST_SCRIPT_MAP_H_

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/script/weight-class.h>
#include <fst/map.h>

namespace fst {
namespace script {

typedef args::Package<MutableFstClass*, MapType, float,
                      const WeightClass &> MapArgs;

template<class Arc>
void Map(MapArgs *args) {
  MutableFst<Arc> *ofst = args->arg1->GetMutableFst<Arc>();
  MapType map_type = args->arg2;
  float delta = args->arg3;
  typename Arc::Weight w = *(args->arg4.GetWeight<typename Arc::Weight>());

  Map(ofst, map_type, delta, w);
}

void Map(MutableFstClass *ofst, MapType map_type,
         float delta = fst::kDelta,
         const WeightClass &w = fst::script::WeightClass::Zero());

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_MAP_H_
