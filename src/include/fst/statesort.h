// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Function to sort states of an FST.

#ifndef FST_LIB_STATESORT_H_
#define FST_LIB_STATESORT_H_

#include <algorithm>
#include <vector>

#include <fst/mutable-fst.h>


namespace fst {

// Sorts the input states of an FST, modifying it. ORDER[i] gives the
// the state Id after sorting that corresponds to state Id i before
// sorting.  ORDER must be a permutation of FST's states ID sequence:
// (0, 1, 2, ..., fst->NumStates() - 1).
template <class Arc>
void StateSort(MutableFst<Arc> *fst,
               const std::vector<typename Arc::StateId> &order) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  if (order.size() != fst->NumStates()) {
    FSTERROR() << "StateSort: Bad order vector size: " << order.size();
    fst->SetProperties(kError, kError);
    return;
  }

  if (fst->Start() == kNoStateId) return;

  uint64 props = fst->Properties(kStateSortProperties, false);

  std::vector<bool> done(order.size(), false);
  std::vector<Arc> arcsa, arcsb;
  std::vector<Arc> *arcs1 = &arcsa, *arcs2 = &arcsb;

  fst->SetStart(order[fst->Start()]);

  for (StateIterator<MutableFst<Arc>> siter(*fst); !siter.Done();
       siter.Next()) {
    StateId s1 = siter.Value(), s2;
    if (done[s1]) continue;
    Weight final1 = fst->Final(s1), final2 = Weight::Zero();
    arcs1->clear();
    for (ArcIterator<MutableFst<Arc>> aiter(*fst, s1); !aiter.Done();
         aiter.Next())
      arcs1->push_back(aiter.Value());
    for (; !done[s1]; s1 = s2, final1 = final2, std::swap(arcs1, arcs2)) {
      s2 = order[s1];
      if (!done[s2]) {
        final2 = fst->Final(s2);
        arcs2->clear();
        for (ArcIterator<MutableFst<Arc>> aiter(*fst, s2); !aiter.Done();
             aiter.Next())
          arcs2->push_back(aiter.Value());
      }
      fst->SetFinal(s2, final1);
      fst->DeleteArcs(s2);
      for (size_t i = 0; i < arcs1->size(); ++i) {
        Arc arc = (*arcs1)[i];
        arc.nextstate = order[arc.nextstate];
        fst->AddArc(s2, arc);
      }
      done[s1] = true;
    }
  }
  fst->SetProperties(props, kFstProperties);
}

}  // namespace fst

#endif  // FST_LIB_STATESORT_H_
