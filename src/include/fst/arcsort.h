// arcsort.h

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
// Author: riley@google.com (Michael Riley)
//
// \file
// Functions and classes to sort arcs in an FST.

#ifndef FST_LIB_ARCSORT_H__
#define FST_LIB_ARCSORT_H__

#include <algorithm>
#include <string>
#include <vector>
using std::vector;
#include <fst/cache.h>
#include <fst/test-properties.h>

namespace fst {

// Sorts the arcs in an FST according to function object 'comp' of
// type Compare. This version modifies its input.  Comparison function
// objects IlabelCompare and OlabelCompare are provived by the
// library. In general, Compare must meet the requirements for an STL
// sort comparision function object. It must also have a member
// Properties(uint64) that specifies the known properties of the
// sorted FST; it takes as argument the input FST's known properties
// before the sort.
//
// Complexity:
// - Time: O(V D log D)
// - Space: O(D)
// where V = # of states and D = maximum out-degree.
template<class Arc, class Compare>
void ArcSort(MutableFst<Arc> *fst, Compare comp) {
  typedef typename Arc::StateId StateId;

  uint64 props = fst->Properties(kFstProperties, false);

  vector<Arc> arcs;
  for (StateIterator< MutableFst<Arc> > siter(*fst);
       !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    arcs.clear();
    for (ArcIterator< MutableFst<Arc> > aiter(*fst, s);
         !aiter.Done();
         aiter.Next())
      arcs.push_back(aiter.Value());
    sort(arcs.begin(), arcs.end(), comp);
    fst->DeleteArcs(s);
    for (size_t a = 0; a < arcs.size(); ++a)
      fst->AddArc(s, arcs[a]);
  }

  fst->SetProperties(comp.Properties(props), kFstProperties);
}

typedef CacheOptions ArcSortFstOptions;

// Implementation of delayed ArcSortFst.
template<class A, class C>
class ArcSortFstImpl : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;
  using FstImpl<A>::InputSymbols;
  using FstImpl<A>::OutputSymbols;

  using VectorFstBaseImpl<typename CacheImpl<A>::State>::NumStates;

  using CacheImpl<A>::AddArc;
  using CacheImpl<A>::GetState;
  using CacheImpl<A>::HasArcs;
  using CacheImpl<A>::HasFinal;
  using CacheImpl<A>::HasStart;
  using CacheImpl<A>::SetArcs;
  using CacheImpl<A>::SetFinal;
  using CacheImpl<A>::SetStart;

  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  ArcSortFstImpl(const Fst<A> &fst, const C &comp,
                 const ArcSortFstOptions &opts)
      : CacheImpl<A>(opts), fst_(fst.Copy()), comp_(comp) {
    SetType("arcsort");
    uint64 props = fst_->Properties(kCopyProperties, false);
    SetProperties(comp_.Properties(props));
    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
  }

  ArcSortFstImpl(const ArcSortFstImpl& impl)
      : CacheImpl<A>(impl),
        fst_(impl.fst_->Copy(true)),
        comp_(impl.comp_) {
    SetType("arcsort");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  ~ArcSortFstImpl() { delete fst_; }

  StateId Start() {
    if (!HasStart())
      SetStart(fst_->Start());
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s))
      SetFinal(s, fst_->Final(s));
    return CacheImpl<A>::Final(s);
  }

  size_t NumArcs(StateId s) {
    if (!HasArcs(s))
      Expand(s);
    return CacheImpl<A>::NumArcs(s);
  }

  size_t NumInputEpsilons(StateId s) {
    if (!HasArcs(s))
      Expand(s);
    return CacheImpl<A>::NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(StateId s) {
    if (!HasArcs(s))
      Expand(s);
    return CacheImpl<A>::NumOutputEpsilons(s);
  }

  void InitStateIterator(StateIteratorData<A> *data) const {
    fst_->InitStateIterator(data);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    if (!HasArcs(s))
      Expand(s);
    CacheImpl<A>::InitArcIterator(s, data);
  }

  void Expand(StateId s) {
    for (ArcIterator< Fst<A> > aiter(*fst_, s); !aiter.Done(); aiter.Next())
      AddArc(s, aiter.Value());
    SetArcs(s);

    vector<A> &carcs = GetState(s)->arcs;
    sort(carcs.begin(), carcs.end(), comp_);
  }

 private:
  const Fst<A> *fst_;
  C comp_;

  void operator=(const ArcSortFstImpl<A, C> &impl);  // Disallow
};


// Sorts the arcs in an FST according to function object 'comp' of
// type Compare. This version is a delayed Fst.  Comparsion function
// objects IlabelCompare and OlabelCompare are provided by the
// library. In general, Compare must meet the requirements for an STL
// comparision function object (e.g. as used for STL sort). It must
// also have a member Properties(uint64) that specifies the known
// properties of the sorted FST; it takes as argument the input FST's
// known properties.
//
// Complexity:
// - Time: O(v d log d)
// - Space: O(d)
// where v = # of states visited, d = maximum out-degree of states
// visited. Constant time and space to visit an input state is assumed
// and exclusive of caching.
//
// This class attaches interface to implementation and handles
// reference counting, delegating most methods to ImplToFst.
template <class A, class C>
class ArcSortFst : public ImplToFst< ArcSortFstImpl<A, C> > {
 public:
  friend class ArcIterator< ArcSortFst<A, C> >;

  typedef A Arc;
  typedef C Compare;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;
  typedef ArcSortFstImpl<A, C> Impl;

  ArcSortFst(const Fst<A> &fst, const C &comp)
      : ImplToFst<Impl>(new Impl(fst, comp, ArcSortFstOptions())) {}

  ArcSortFst(const Fst<A> &fst, const C &comp, const ArcSortFstOptions &opts)
      : ImplToFst<Impl>(new Impl(fst, comp, opts)) {}

  // See Fst<>::Copy() for doc.
  ArcSortFst(const ArcSortFst<A, C> &fst, bool safe = false)
      : ImplToFst<Impl> (fst, safe) {}

  // Get a copy of this ArcSortFst. See Fst<>::Copy() for further doc.
  virtual ArcSortFst<A, C> *Copy(bool safe = false) const {
    return new ArcSortFst<A, C>(*this, safe);
  }

  virtual void InitStateIterator(StateIteratorData<A> *data) const {
    GetImpl()->InitStateIterator(data);
  }

  virtual void InitArcIterator(StateId s, ArcIteratorData<A> *data) const {
    GetImpl()->InitArcIterator(s, data);
  }

 private:
  // Makes visible to friends.
  Impl *GetImpl() const { return ImplToFst<Impl>::GetImpl(); }

  void operator=(const ArcSortFst<A, C> &fst);  // Disallow
};


// Specialization for ArcSortFst.
template <class A, class C>
class ArcIterator< ArcSortFst<A, C> >
    : public CacheArcIterator< ArcSortFst<A, C> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const ArcSortFst<A, C> &fst, StateId s)
      : CacheArcIterator< ArcSortFst<A, C> >(fst.GetImpl(), s) {
    if (!fst.GetImpl()->HasArcs(s))
      fst.GetImpl()->Expand(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};


// Compare class for comparing input labels of arcs.
template<class A> class ILabelCompare {
 public:
  bool operator() (A arc1, A arc2) const {
    return arc1.ilabel < arc2.ilabel;
  }

  uint64 Properties(uint64 props) const {
    return (props & kArcSortProperties) | kILabelSorted |
        (props & kAcceptor ? kOLabelSorted : 0);
  }
};


// Compare class for comparing output labels of arcs.
template<class A> class OLabelCompare {
 public:
  bool operator() (const A &arc1, const A &arc2) const {
    return arc1.olabel < arc2.olabel;
  }

  uint64 Properties(uint64 props) const {
    return (props & kArcSortProperties) | kOLabelSorted |
        (props & kAcceptor ? kILabelSorted : 0);
  }
};


// Useful aliases when using StdArc.
template<class C> class StdArcSortFst : public ArcSortFst<StdArc, C> {
 public:
  typedef StdArc Arc;
  typedef C Compare;
};

typedef ILabelCompare<StdArc> StdILabelCompare;

typedef OLabelCompare<StdArc> StdOLabelCompare;

}  // namespace fst

#endif  // FST_LIB_ARCSORT_H__
