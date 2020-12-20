
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

#ifndef FST_SCRIPT_RANDGEN_H_
#define FST_SCRIPT_RANDGEN_H_

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/randgen.h>

namespace fst {
namespace script {

enum RandArcSelection {
  UNIFORM_ARC_SELECTOR,
  LOG_PROB_ARC_SELECTOR,
  FAST_LOG_PROB_ARC_SELECTOR
};

// This is to prevent log probabilities from working with other than
// StdArc and LogArc
template<class A>
struct LogProbArcSelectorGuard {
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  explicit LogProbArcSelectorGuard(int seed) {
    LOG(FATAL) << "LogProbArcSelectorGuard: bad weight type: "
               << Weight::Type();
  }
  size_t operator()(const Fst<A> &fst, StateId s) const { return 0; }
};

template<class T>
struct LogProbArcSelectorGuard<ArcTpl<TropicalWeightTpl<T> > >
    : public LogProbArcSelector<ArcTpl<TropicalWeightTpl<T> > > {
  LogProbArcSelectorGuard(int seed = time(0))
      : LogProbArcSelector<ArcTpl<TropicalWeightTpl<T> > >(seed) {}
};

template<class T>
struct LogProbArcSelectorGuard<ArcTpl<LogWeightTpl<T> > >
    : public LogProbArcSelector<ArcTpl<LogWeightTpl<T> > > {
  LogProbArcSelectorGuard(int seed = time(0))
      : LogProbArcSelector<ArcTpl<LogWeightTpl<T> > >(seed) {}
};

// This is to prevent log probabilities from working with other than
// StdArc and LogArc
template<class A>
struct FastLogProbArcSelectorGuard {
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  explicit FastLogProbArcSelectorGuard(int seed) {
    LOG(FATAL) << "LogProbArcSelectorGuard: bad weight type: "
               << Weight::Type();
  }
  size_t operator()(const Fst<A> &fst, StateId s) const { return 0; }
};

template<class T>
struct FastLogProbArcSelectorGuard<ArcTpl<TropicalWeightTpl<T> > >
    : public FastLogProbArcSelector<ArcTpl<TropicalWeightTpl<T> > > {
  FastLogProbArcSelectorGuard(int seed = time(0))
      : FastLogProbArcSelector<ArcTpl<TropicalWeightTpl<T> > >(seed) {}
};

template<class T>
struct FastLogProbArcSelectorGuard<ArcTpl<LogWeightTpl<T> > >
    : public FastLogProbArcSelector<ArcTpl<LogWeightTpl<T> > > {
  FastLogProbArcSelectorGuard(int seed = time(0))
      : FastLogProbArcSelector<ArcTpl<LogWeightTpl<T> > >(seed) {}
};

typedef args::Package<const FstClass &, MutableFstClass*, int32,
                      const RandGenOptions<RandArcSelection> &> RandGenArgs;

template<class Arc>
void RandGen(RandGenArgs *args) {
  const Fst<Arc> &ifst = *(args->arg1.GetFst<Arc>());
  MutableFst<Arc> *ofst = args->arg2->GetMutableFst<Arc>();
  int32 seed = args->arg3;
  const RandGenOptions<RandArcSelection> &opts = args->arg4;

  if (opts.arc_selector == UNIFORM_ARC_SELECTOR) {
    UniformArcSelector<Arc> arc_selector(seed);
    RandGenOptions< UniformArcSelector<Arc> >
        ropts(arc_selector, opts.max_length,
              opts.npath);
    RandGen(ifst, ofst, ropts);
  } else if (opts.arc_selector == FAST_LOG_PROB_ARC_SELECTOR) {
    FastLogProbArcSelectorGuard<Arc> arc_selector(seed);
    RandGenOptions< FastLogProbArcSelectorGuard<Arc> >
        ropts(arc_selector, opts.max_length,
              opts.npath);
    RandGen(ifst, ofst, ropts);
  } else {
    LogProbArcSelectorGuard<Arc> arc_selector(seed);
    RandGenOptions< LogProbArcSelectorGuard<Arc> >
        ropts(arc_selector, opts.max_length,
              opts.npath);
    RandGen(ifst, ofst, ropts);
  }
}


// Client-facing prototype
void RandGen(const FstClass &ifst, MutableFstClass *ofst, int32 seed = time(0),
             const RandGenOptions<RandArcSelection> &opts =
             fst::RandGenOptions<fst::script::RandArcSelection>(
                 fst::script::UNIFORM_ARC_SELECTOR));

}  // namespace script
}  // namespace fst



#endif  // FST_SCRIPT_RANDGEN_H_
