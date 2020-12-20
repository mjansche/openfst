// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_ARCSORT_H_
#define FST_SCRIPT_ARCSORT_H_

#include <fst/arcsort.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>

namespace fst {
namespace script {

enum ArcSortType { ILABEL_COMPARE, OLABEL_COMPARE };

typedef args::Package<MutableFstClass *, ArcSortType> ArcSortArgs;

template <class Arc>
void ArcSort(ArcSortArgs *args) {
  MutableFst<Arc> *fst = args->arg1->GetMutableFst<Arc>();

  if (args->arg2 == ILABEL_COMPARE) {
    ILabelCompare<Arc> icomp;
    ArcSort(fst, icomp);
  } else {  // OLABEL_COMPARE
    OLabelCompare<Arc> ocomp;
    ArcSort(fst, ocomp);
  }
}

void ArcSort(MutableFstClass *ofst, ArcSortType sort_type);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_ARCSORT_H_
