// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_RANDGEN_H_
#define FST_SCRIPT_RANDGEN_H_

#include <fst/randgen.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>

namespace fst {
namespace script {

enum RandArcSelection {
  UNIFORM_ARC_SELECTOR,
  LOG_PROB_ARC_SELECTOR,
  FAST_LOG_PROB_ARC_SELECTOR
};

typedef args::Package<const FstClass &, MutableFstClass *, time_t,
                      const RandGenOptions<RandArcSelection> &> RandGenArgs;

template <class Arc>
void RandGen(RandGenArgs *args) {
  const Fst<Arc> &ifst = *(args->arg1.GetFst<Arc>());
  MutableFst<Arc> *ofst = args->arg2->GetMutableFst<Arc>();
  time_t seed = args->arg3;
  const RandGenOptions<RandArcSelection> &opts = args->arg4;

  if (opts.arc_selector == UNIFORM_ARC_SELECTOR) {
    UniformArcSelector<Arc> arc_selector(seed);
    RandGenOptions<UniformArcSelector<Arc>> ropts(arc_selector, opts.max_length,
                                                  opts.npath, opts.weighted,
                                                  opts.remove_total_weight);
    RandGen(ifst, ofst, ropts);
  } else if (opts.arc_selector == FAST_LOG_PROB_ARC_SELECTOR) {
    FastLogProbArcSelector<Arc> arc_selector(seed);
    RandGenOptions<FastLogProbArcSelector<Arc>> ropts(
        arc_selector, opts.max_length, opts.npath, opts.weighted,
        opts.remove_total_weight);
    RandGen(ifst, ofst, ropts);
  } else {
    LogProbArcSelector<Arc> arc_selector(seed);
    RandGenOptions<LogProbArcSelector<Arc>> ropts(arc_selector, opts.max_length,
                                                  opts.npath, opts.weighted,
                                                  opts.remove_total_weight);
    RandGen(ifst, ofst, ropts);
  }
}

// Client-facing prototype
void RandGen(const FstClass &ifst, MutableFstClass *ofst,
             time_t seed = time(nullptr),
             const RandGenOptions<RandArcSelection> &opts =
                 fst::RandGenOptions<fst::script::RandArcSelection>(
                     fst::script::UNIFORM_ARC_SELECTOR));

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_RANDGEN_H_
