// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_ISOMORPHIC_H_
#define FST_SCRIPT_ISOMORPHIC_H_

#include <fst/isomorphic.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>

namespace fst {
namespace script {

typedef args::Package<const FstClass &, const FstClass &, float, bool *>
    IsomorphicInnerArgs;
typedef args::WithReturnValue<bool, IsomorphicInnerArgs> IsomorphicArgs;

template <class Arc>
void Isomorphic(IsomorphicArgs *args) {
  const Fst<Arc> &fst1 = *(args->args.arg1.GetFst<Arc>());
  const Fst<Arc> &fst2 = *(args->args.arg2.GetFst<Arc>());

  args->retval = Isomorphic(fst1, fst2, args->args.arg3, args->args.arg4);
}

bool Isomorphic(const FstClass &fst1, const FstClass &fst2,
                float delta = kDelta, bool *error = nullptr);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_ISOMORPHIC_H_
