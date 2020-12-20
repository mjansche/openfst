// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_EQUAL_H_
#define FST_SCRIPT_EQUAL_H_

#include <fst/equal.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>

namespace fst {
namespace script {

typedef args::Package<const FstClass &, const FstClass &, float> EqualInnerArgs;
typedef args::WithReturnValue<bool, EqualInnerArgs> EqualArgs;

template <class Arc>
void Equal(EqualArgs *args) {
  const Fst<Arc> &fst1 = *(args->args.arg1.GetFst<Arc>());
  const Fst<Arc> &fst2 = *(args->args.arg2.GetFst<Arc>());

  args->retval = Equal(fst1, fst2, args->args.arg3);
}

bool Equal(const FstClass &fst1, const FstClass &fst2, float delta = kDelta);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_EQUAL_H_
