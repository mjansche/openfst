// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Class to compute the intersection of two FSAs.

#ifndef FST_LIB_INTERSECT_H_
#define FST_LIB_INTERSECT_H_

#include <algorithm>
#include <vector>

#include <fst/cache.h>
#include <fst/compose.h>


namespace fst {

template <class A, class M = Matcher<Fst<A>>,
          class F = SequenceComposeFilter<M>,
          class T = GenericComposeStateTable<A, typename F::FilterState>>
struct IntersectFstOptions : public ComposeFstOptions<A, M, F, T> {
  explicit IntersectFstOptions(const CacheOptions &opts, M *mat1 = 0,
                               M *mat2 = 0, F *filt = 0, T *sttable = 0)
      : ComposeFstOptions<A, M, F, T>(opts, mat1, mat2, filt, sttable) {}

  IntersectFstOptions() {}
};

// Computes the intersection (Hadamard product) of two FSAs. This
// version is a delayed Fst.  Only strings that are in both automata
// are retained in the result.
//
// The two arguments must be acceptors. One of the arguments must be
// label-sorted.
//
// Complexity: same as ComposeFst.
//
// Caveats:  same as ComposeFst.
template <class A>
class IntersectFst : public ComposeFst<A> {
 public:
  using ComposeFst<A>::CreateBase;
  using ComposeFst<A>::CreateBase1;
  using ComposeFst<A>::Properties;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  IntersectFst(const Fst<A> &fst1, const Fst<A> &fst2,
               const CacheOptions opts = CacheOptions())
      : ComposeFst<A>(CreateBase(fst1, fst2, opts)) {
    bool acceptors =
        fst1.Properties(kAcceptor, true) && fst2.Properties(kAcceptor, true);
    if (!acceptors) {
      FSTERROR() << "IntersectFst: Input Fsts are not acceptors";
      GetMutableImpl()->SetProperties(kError);
    }
  }

  template <class M, class F, class T>
  IntersectFst(const Fst<A> &fst1, const Fst<A> &fst2,
               const IntersectFstOptions<A, M, F, T> &opts)
      : ComposeFst<A>(CreateBase1(fst1, fst2, opts)) {
    bool acceptors =
        fst1.Properties(kAcceptor, true) && fst2.Properties(kAcceptor, true);
    if (!acceptors) {
      FSTERROR() << "IntersectFst: input Fsts are not acceptors";
      GetMutableImpl()->SetProperties(kError);
    }
  }

  // See Fst<>::Copy() for doc.
  IntersectFst(const IntersectFst<A> &fst, bool safe = false)
      : ComposeFst<A>(fst, safe) {}

  // Get a copy of this IntersectFst. See Fst<>::Copy() for further doc.
  IntersectFst<A> *Copy(bool safe = false) const override {
    return new IntersectFst<A>(*this, safe);
  }

 private:
  using ImplToFst<ComposeFstImplBase<A>>::GetImpl;
  using ImplToFst<ComposeFstImplBase<A>>::GetMutableImpl;
};

// Specialization for IntersectFst.
template <class A>
class StateIterator<IntersectFst<A>> : public StateIterator<ComposeFst<A>> {
 public:
  explicit StateIterator(const IntersectFst<A> &fst)
      : StateIterator<ComposeFst<A>>(fst) {}
};

// Specialization for IntersectFst.
template <class A>
class ArcIterator<IntersectFst<A>> : public ArcIterator<ComposeFst<A>> {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const IntersectFst<A> &fst, StateId s)
      : ArcIterator<ComposeFst<A>>(fst, s) {}
};

// Useful alias when using StdArc.
typedef IntersectFst<StdArc> StdIntersectFst;

typedef ComposeOptions IntersectOptions;

// Computes the intersection (Hadamard product) of two FSAs. This
// version writes the intersection to an output MurableFst. Only
// strings that are in both automata are retained in the result.
//
// The two arguments must be acceptors. One of the arguments must be
// label-sorted.
//
// Complexity: same as Compose.
//
// Caveats:  same as Compose.
template <class Arc>
void Intersect(const Fst<Arc> &ifst1, const Fst<Arc> &ifst2,
               MutableFst<Arc> *ofst,
               const IntersectOptions &opts = IntersectOptions()) {
  typedef Matcher<Fst<Arc>> M;

  if (opts.filter_type == AUTO_FILTER) {
    CacheOptions nopts;
    nopts.gc_limit = 0;  // Cache only the last state for fastest copy.
    *ofst = IntersectFst<Arc>(ifst1, ifst2, nopts);
  } else if (opts.filter_type == SEQUENCE_FILTER) {
    IntersectFstOptions<Arc> iopts;
    iopts.gc_limit = 0;  // Cache only the last state for fastest copy.
    *ofst = IntersectFst<Arc>(ifst1, ifst2, iopts);
  } else if (opts.filter_type == ALT_SEQUENCE_FILTER) {
    IntersectFstOptions<Arc, M, AltSequenceComposeFilter<M>> iopts;
    iopts.gc_limit = 0;  // Cache only the last state for fastest copy.
    *ofst = IntersectFst<Arc>(ifst1, ifst2, iopts);
  } else if (opts.filter_type == MATCH_FILTER) {
    IntersectFstOptions<Arc, M, MatchComposeFilter<M>> iopts;
    iopts.gc_limit = 0;  // Cache only the last state for fastest copy.
    *ofst = IntersectFst<Arc>(ifst1, ifst2, iopts);
  }

  if (opts.connect) Connect(ofst);
}

}  // namespace fst

#endif  // FST_LIB_INTERSECT_H_
