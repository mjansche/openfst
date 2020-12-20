// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Functions and classes to compute the union of two FSTs.

#ifndef FST_LIB_UNION_H__
#define FST_LIB_UNION_H__

#include <algorithm>
#include <vector>

#include <fst/mutable-fst.h>
#include <fst/rational.h>


namespace fst {

// Computes the union (sum) of two FSTs.  This version writes the
// union to an output MutableFst. If A transduces string x to y with
// weight a and B transduces string w to v with weight b, then their
// union transduces x to y with weight a and w to v with weight b.
//
// Complexity:
// - Time: (V2 + E2)
// - Space: O(V2 + E2)
// where Vi = # of states and Ei = # of arcs of the ith FST.
template <class Arc>
void Union(MutableFst<Arc> *fst1, const Fst<Arc> &fst2) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  // Check that the symbol tables are compatible
  if (!CompatSymbols(fst1->InputSymbols(), fst2.InputSymbols()) ||
      !CompatSymbols(fst1->OutputSymbols(), fst2.OutputSymbols())) {
    FSTERROR() << "Union: Input/output symbol tables of 1st argument "
               << "do not match input/output symbol tables of 2nd argument";
    fst1->SetProperties(kError, kError);
    return;
  }

  StateId numstates1 = fst1->NumStates();
  bool initial_acyclic1 = fst1->Properties(kInitialAcyclic, true);
  uint64 props1 = fst1->Properties(kFstProperties, false);
  uint64 props2 = fst2.Properties(kFstProperties, false);

  StateId start2 = fst2.Start();
  if (start2 == kNoStateId) {
    if (props2 & kError) fst1->SetProperties(kError, kError);
    return;
  }

  if (fst2.Properties(kExpanded, false)) {
    fst1->ReserveStates(numstates1 + CountStates(fst2) +
                        (initial_acyclic1 ? 0 : 1));
  }

  for (StateIterator<Fst<Arc>> siter(fst2); !siter.Done(); siter.Next()) {
    StateId s1 = fst1->AddState();
    StateId s2 = siter.Value();
    fst1->SetFinal(s1, fst2.Final(s2));
    fst1->ReserveArcs(s1, fst2.NumArcs(s2));
    for (ArcIterator<Fst<Arc>> aiter(fst2, s2); !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      arc.nextstate += numstates1;
      fst1->AddArc(s1, arc);
    }
  }
  StateId start1 = fst1->Start();
  if (start1 == kNoStateId) {
    fst1->SetStart(start2);
    fst1->SetProperties(props2, kCopyProperties);
    return;
  }

  if (initial_acyclic1) {
    fst1->AddArc(start1, Arc(0, 0, Weight::One(), start2 + numstates1));
  } else {
    StateId nstart1 = fst1->AddState();
    fst1->SetStart(nstart1);
    fst1->AddArc(nstart1, Arc(0, 0, Weight::One(), start1));
    fst1->AddArc(nstart1, Arc(0, 0, Weight::One(), start2 + numstates1));
  }
  fst1->SetProperties(UnionProperties(props1, props2), kFstProperties);
}

// Computes the union of two FSTs; this version modifies its
// RationalFst argument.
template <class Arc>
void Union(RationalFst<Arc> *fst1, const Fst<Arc> &fst2) {
  fst1->GetMutableImpl()->AddUnion(fst2);
}

typedef RationalFstOptions UnionFstOptions;

// Computes the union (sum) of two FSTs. This version is a delayed
// Fst. If A transduces string x to y with weight a and B transduces
// string w to v with weight b, then their union transduces x to y
// with weight a and w to v with weight b.
//
// Complexity:
// - Time: O(v1 + e1 + v2 + e2)
// - Space: O(v1 + v2)
// where vi = # of states visited and ei = # of arcs visited of the
// ith FST. Constant time and space to visit an input state or arc
// is assumed and exclusive of caching.
template <class A>
class UnionFst : public RationalFst<A> {
 public:
  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  UnionFst(const Fst<A> &fst1, const Fst<A> &fst2) {
    GetMutableImpl()->InitUnion(fst1, fst2);
  }

  UnionFst(const Fst<A> &fst1, const Fst<A> &fst2, const UnionFstOptions &opts)
      : RationalFst<A>(opts) {
    GetMutableImpl()->InitUnion(fst1, fst2);
  }

  // See Fst<>::Copy() for doc.
  UnionFst(const UnionFst<A> &fst, bool safe = false)
      : RationalFst<A>(fst, safe) {}

  // Get a copy of this UnionFst. See Fst<>::Copy() for further doc.
  UnionFst<A> *Copy(bool safe = false) const override {
    return new UnionFst<A>(*this, safe);
  }

 private:
  using ImplToFst<RationalFstImpl<A>>::GetImpl;
  using ImplToFst<RationalFstImpl<A>>::GetMutableImpl;
};

// Specialization for UnionFst.
template <class A>
class StateIterator<UnionFst<A>> : public StateIterator<RationalFst<A>> {
 public:
  explicit StateIterator(const UnionFst<A> &fst)
      : StateIterator<RationalFst<A>>(fst) {}
};

// Specialization for UnionFst.
template <class A>
class ArcIterator<UnionFst<A>> : public ArcIterator<RationalFst<A>> {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const UnionFst<A> &fst, StateId s)
      : ArcIterator<RationalFst<A>>(fst, s) {}
};

// Useful alias when using StdArc.
typedef UnionFst<StdArc> StdUnionFst;

}  // namespace fst

#endif  // FST_LIB_UNION_H__
