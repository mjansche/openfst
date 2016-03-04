// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_RANDEQUIVALENT_H_
#define FST_SCRIPT_RANDEQUIVALENT_H_

#include <fst/randequivalent.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/script/randgen.h>  // for RandArcSelection

namespace fst {
namespace script {

// 1
typedef args::Package<const FstClass &, const FstClass &, time_t, int32,
                      float, int32, bool *> RandEquivalentInnerArgs1;

typedef args::WithReturnValue<bool, RandEquivalentInnerArgs1>
    RandEquivalentArgs1;

template <class Arc>
void RandEquivalent(RandEquivalentArgs1 *args) {
  const Fst<Arc> &fst1 = *(args->args.arg1.GetFst<Arc>());
  const Fst<Arc> &fst2 = *(args->args.arg2.GetFst<Arc>());

  args->retval =
      RandEquivalent(fst1, fst2, args->args.arg3, args->args.arg4,
                     args->args.arg5, args->args.arg6, args->args.arg7);
}

// 2
typedef args::Package<const FstClass &, const FstClass &, time_t, int32,
                      float, const RandGenOptions<RandArcSelection> &,
                      bool *> RandEquivalentInnerArgs2;

typedef args::WithReturnValue<bool, RandEquivalentInnerArgs2>
    RandEquivalentArgs2;

template <class Arc>
void RandEquivalent(RandEquivalentArgs2 *args) {
  const Fst<Arc> &fst1 = *(args->args.arg1.GetFst<Arc>());
  const Fst<Arc> &fst2 = *(args->args.arg2.GetFst<Arc>());
  const RandGenOptions<RandArcSelection> &opts = args->args.arg6;
  time_t seed = args->args.arg3;

  if (opts.arc_selector == UNIFORM_ARC_SELECTOR) {
    UniformArcSelector<Arc> arc_selector(seed);
    RandGenOptions<UniformArcSelector<Arc>> ropts(arc_selector, opts.max_length,
                                                  opts.npath);

    args->retval = RandEquivalent(fst1, fst2, args->args.arg4, args->args.arg5,
                                  ropts, args->args.arg7);
  } else if (opts.arc_selector == FAST_LOG_PROB_ARC_SELECTOR) {
    FastLogProbArcSelector<Arc> arc_selector(seed);
    RandGenOptions<FastLogProbArcSelector<Arc>> ropts(
        arc_selector, opts.max_length, opts.npath);

    args->retval = RandEquivalent(fst1, fst2, args->args.arg4, args->args.arg5,
                                  ropts, args->args.arg7);
  } else {
    LogProbArcSelector<Arc> arc_selector(seed);
    RandGenOptions<LogProbArcSelector<Arc>> ropts(arc_selector, opts.max_length,
                                                  opts.npath);
    args->retval = RandEquivalent(fst1, fst2, args->args.arg4, args->args.arg5,
                                  ropts, args->args.arg7);
  }
}

// 1
bool RandEquivalent(const FstClass &fst1, const FstClass &fst2,
                    time_t seed = time(nullptr), int32 num_paths = 1,
                    float delta = fst::kDelta, int32 path_length = INT_MAX,
                    bool *error = nullptr);

// 2
bool RandEquivalent(
    const FstClass &fst1, const FstClass &fst2, time_t seed, int32 num_paths,
    float delta,
    const fst::RandGenOptions<fst::script::RandArcSelection> &opts,
    bool *error = nullptr);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_RANDEQUIVALENT_H_
