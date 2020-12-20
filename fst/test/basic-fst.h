// basic-fst.h
// Author: riley@google.com (Michael Riley)
//
// \file
// Similar to VectorFst but with simplified property maintainence.

#ifndef FST_TEST_BASIC_FST_H__
#define FST_TEST_BASIC_FST_H__

#include "fst/lib/vector-fst.h"

namespace fst {

template <class A> class BasicFst;

// This is a BasicFstBaseImpl container that holds VectorState's.  It
// manages the # of input and output epsilons.
template <class A>
class BasicFstImpl : public VectorFstBaseImpl< VectorState<A> > {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::WriteHeaderAndSymbols;

  using VectorFstBaseImpl<VectorState<A> >::Start;
  using VectorFstBaseImpl<VectorState<A> >::NumStates;

  friend class MutableArcIterator< BasicFst<A> >;

  typedef VectorFstBaseImpl< VectorState<A> > BaseImpl;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  BasicFstImpl() {
    SetType("basic");
    SetProperties(kNullProperties | kStaticProperties);
  }
  explicit BasicFstImpl(const Fst<A> &fst);

  static BasicFstImpl<A> *Read(istream &strm, const FstReadOptions &opts);

  size_t NumInputEpsilons(StateId s) const { return GetState(s)->niepsilons; }

  size_t NumOutputEpsilons(StateId s) const { return GetState(s)->noepsilons; }

  bool Write(ostream &strm, const FstWriteOptions &opts) const;

  void SetStart(StateId s) {
    BaseImpl::SetStart(s);
    SetProperties(Properties() & kSetStartProperties);
  }

  void SetFinal(StateId s, Weight w) {
    Weight ow = Final(s);
    BaseImpl::SetFinal(s, w);
    SetProperties(Properties() & kSetFinalProperties);
  }

  StateId AddState() {
    StateId s = BaseImpl::AddState();
    SetProperties(Properties() & kAddStateProperties);
    return s;
  }

  void AddArc(StateId s, const A &arc) {
    VectorState<A> *state = GetState(s);
    if (arc.ilabel == 0)
      ++state->niepsilons;
    if (arc.olabel == 0)
      ++state->noepsilons;
    BaseImpl::AddArc(s, arc);
    SetProperties(Properties() & kAddArcProperties);
  }

  void DeleteStates(const vector<StateId> &dstates) {
    BaseImpl::DeleteStates(dstates);
    SetProperties(Properties() & kDeleteStatesProperties);
  }

  void DeleteStates() {
    BaseImpl::DeleteStates();
    SetProperties(kNullProperties | kStaticProperties);
  }

  void DeleteArcs(StateId s, size_t n) {
    const vector<A> &arcs = GetState(s)->arcs;
    for (size_t i = 0; i < n; ++i) {
      size_t j = arcs.size() - i - 1;
      if (arcs[j].ilabel == 0)
        --GetState(s)->niepsilons;
      if (arcs[j].olabel == 0)
        --GetState(s)->noepsilons;
    }
    BaseImpl::DeleteArcs(s, n);
    SetProperties(Properties() & kDeleteArcsProperties);
  }

  void DeleteArcs(StateId s) {
    GetState(s)->niepsilons = 0;
    GetState(s)->noepsilons = 0;
    BaseImpl::DeleteArcs(s);
    SetProperties(Properties() & kDeleteArcsProperties);
  }

 private:
  // Properties always true of this Fst class
  static const uint64 kStaticProperties = kExpanded | kMutable;
  // Current file format version
  static const int kFileVersion = 1;
  // Minimum file format version supported
  static const int kMinFileVersion = 1;

  DISALLOW_EVIL_CONSTRUCTORS(BasicFstImpl);
};

template<class A>
BasicFstImpl<A>::BasicFstImpl(const Fst<A> &fst) {
  SetType("basic");
  SetProperties(fst.Properties(kCopyProperties, false) | kStaticProperties);
  SetInputSymbols(fst.InputSymbols());
  SetOutputSymbols(fst.OutputSymbols());
  BaseImpl::SetStart(fst.Start());

  for (StateIterator< Fst<A> > siter(fst);
       !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    BaseImpl::AddState();
    BaseImpl::SetFinal(s, fst.Final(s));
    ReserveArcs(s, fst.NumArcs(s));
    for (ArcIterator< Fst<A> > aiter(fst, s);
         !aiter.Done();
         aiter.Next()) {
      const A &arc = aiter.Value();
      BaseImpl::AddArc(s, arc);
      if (arc.ilabel == 0)
        ++GetState(s)->niepsilons;
      if (arc.olabel == 0)
        ++GetState(s)->noepsilons;
    }
  }
}

template<class A>
BasicFstImpl<A> *BasicFstImpl<A>::Read(istream &strm,
                                         const FstReadOptions &opts) {
  BasicFstImpl<A> *impl = new BasicFstImpl;
  FstHeader hdr;
  if (!impl->ReadHeaderAndSymbols(strm, opts, kMinFileVersion, &hdr))
    return 0;
  impl->BaseImpl::SetStart(hdr.Start());
  impl->ReserveStates(hdr.NumStates());

  for (StateId s = 0; s < hdr.NumStates(); ++s) {
    impl->BaseImpl::AddState();
    VectorState<A> *state = impl->GetState(s);
    state->final.Read(strm);
    int64 narcs;
    ReadType(strm, &narcs);
    if (!strm) {
      LOG(ERROR) << "BasicFst::Read: read failed: " << opts.source;
      return 0;
    }
    impl->ReserveArcs(s, narcs);
    for (size_t j = 0; j < narcs; ++j) {
      A arc;
      ReadType(strm, &arc.ilabel);
      ReadType(strm, &arc.olabel);
      arc.weight.Read(strm);
      ReadType(strm, &arc.nextstate);
      if (!strm) {
        LOG(ERROR) << "BasicFst::Read: read failed: " << opts.source;
        return 0;
      }
      impl->BaseImpl::AddArc(s, arc);
      if (arc.ilabel == 0)
        ++state->niepsilons;
      if (arc.olabel == 0)
        ++state->noepsilons;
    }
  }
  return impl;
}

template<class A>
bool BasicFstImpl<A>::Write(ostream &strm,
                             const FstWriteOptions &opts) const {
  FstHeader hdr;
  hdr.SetStart(Start());
  hdr.SetNumStates(NumStates());
  WriteHeaderAndSymbols(strm, opts, kFileVersion, &hdr);
  if (!strm)
    return false;

  for (StateId s = 0; s < NumStates(); ++s) {
    const VectorState<A> *state = GetState(s);
    state->final.Write(strm);
    int64 narcs = state->arcs.size();
    WriteType(strm, narcs);
    for (size_t a = 0; a < narcs; ++a) {
      const A &arc = state->arcs[a];
      WriteType(strm, arc.ilabel);
      WriteType(strm, arc.olabel);
      arc.weight.Write(strm);
      WriteType(strm, arc.nextstate);
    }
  }
  strm.flush();
  if (!strm)
    LOG(ERROR) << "BasicFst::Write: write failed: " << opts.source;
  return strm;
}

// Simple concrete, mutable FST. Supports additional operations:
// ReserveStates and ReserveArcs (cf. STL vectors). This class
// attaches interface to implementation and handles reference
// counting.
template <class A>
class BasicFst : public MutableFst<A> {
 public:
  friend class StateIterator< BasicFst<A> >;
  friend class ArcIterator< BasicFst<A> >;
  friend class MutableArcIterator< BasicFst<A> >;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  BasicFst() : impl_(new BasicFstImpl<A>) {}

  BasicFst(const BasicFst<A> &fst) : impl_(fst.impl_) {
    impl_->IncrRefCount();
  }
  explicit BasicFst(const Fst<A> &fst) : impl_(new BasicFstImpl<A>(fst)) {}

  virtual ~BasicFst() { if (!impl_->DecrRefCount()) { delete impl_; }}

  virtual BasicFst<A> &operator=(const BasicFst<A> &fst) {
    if (this != &fst) {
      if (!impl_->DecrRefCount()) delete impl_;
      fst.impl_->IncrRefCount();
      impl_ = fst.impl_;
    }
    return *this;
  }

  virtual BasicFst<A> &operator=(const Fst<A> &fst) {
    if (this != &fst) {
      if (!impl_->DecrRefCount()) delete impl_;
      impl_ = new BasicFstImpl<A>(fst);
    }
    return *this;
  }

  virtual StateId Start() const { return impl_->Start(); }

  virtual Weight Final(StateId s) const { return impl_->Final(s); }

  virtual StateId NumStates() const { return impl_->NumStates(); }

  virtual size_t NumArcs(StateId s) const { return impl_->NumArcs(s); }

  virtual size_t NumInputEpsilons(StateId s) const {
    return impl_->NumInputEpsilons(s);
  }

  virtual size_t NumOutputEpsilons(StateId s) const {
    return impl_->NumOutputEpsilons(s);
  }

  virtual uint64 Properties(uint64 mask, bool test) const {
    if (test) {
      uint64 known, test = TestProperties(*this, mask, &known);
      impl_->SetProperties(test, known);
      return test & mask;
    } else {
      return impl_->Properties(mask);
    }
  }

  virtual const string& Type() const { return impl_->Type(); }

  // Get a copy of this BasicFst
  virtual BasicFst<A> *Copy() const {
    impl_->IncrRefCount();
    return new BasicFst<A>(impl_);
  }

  // Read a BasicFst from an input stream; return NULL on error
  static BasicFst<A> *Read(istream &strm, const FstReadOptions &opts) {
    BasicFstImpl<A>* impl = BasicFstImpl<A>::Read(strm, opts);
    return impl ? new BasicFst<A>(impl) : 0;
  }

  // Read a BasicFst from a file; return NULL on error
  static BasicFst<A> *Read(const string &filename) {
    ifstream strm(filename.c_str());
    if (!strm) {
      LOG(ERROR) << "BasicFst::Read: Can't open file: " << filename;
      return 0;
    }
    return Read(strm, FstReadOptions(filename));
  }

  // Write a BasicFst to an output stream; return false on error
  virtual bool Write(ostream &strm, const FstWriteOptions &opts) const {
    return impl_->Write(strm, opts);
  }

  // Write a BasicFst to a file; return false on error
  virtual bool Write(const string &filename) const {
    if (!filename.empty()) {
      ofstream strm(filename.c_str());
      if (!strm) {
        LOG(ERROR) << "BasicrFst::Write: Can't open file: " << filename;
        return false;
      }
      return Write(strm, FstWriteOptions(filename));
    } else {
      return Write(std::cout, FstWriteOptions("standard output"));
    }
  }

  virtual SymbolTable* InputSymbols() {
    return impl_->InputSymbols();
  }

  virtual SymbolTable* OutputSymbols() {
    return impl_->OutputSymbols();
  }

  virtual const SymbolTable* InputSymbols() const {
    return impl_->InputSymbols();
  }

  virtual const SymbolTable* OutputSymbols() const {
    return impl_->OutputSymbols();
  }

  virtual void SetStart(StateId s) {
    MutateCheck();
    impl_->SetStart(s);
  }

  virtual void SetFinal(StateId s, Weight w) {
    MutateCheck();
    impl_->SetFinal(s, w);
  }

  virtual void SetProperties(uint64 props, uint64 mask) {
    impl_->SetProperties(props, mask);
  }

  virtual StateId AddState() {
    MutateCheck();
    return impl_->AddState();
  }

  virtual void AddArc(StateId s, const A &arc) {
    MutateCheck();
    impl_->AddArc(s, arc);
  }

  virtual void DeleteStates(const vector<StateId> &dstates) {
    MutateCheck();
    impl_->DeleteStates(dstates);
  }

  virtual void DeleteStates() {
    MutateCheck();
    impl_->DeleteStates();
  }

  virtual void DeleteArcs(StateId s, size_t n) {
    MutateCheck();
    impl_->DeleteArcs(s, n);
  }

  virtual void DeleteArcs(StateId s) {
    MutateCheck();
    impl_->DeleteArcs(s);
  }

  virtual void SetInputSymbols(const SymbolTable* isyms) {
    MutateCheck();
    impl_->SetInputSymbols(isyms);
  }

  virtual void SetOutputSymbols(const SymbolTable* osyms) {
    MutateCheck();
    impl_->SetOutputSymbols(osyms);
  }

  void ReserveStates(StateId n) {
    MutateCheck();
    impl_->ReserveStates(n);
  }

  void ReserveArcs(StateId s, size_t n) {
    MutateCheck();
    impl_->ReserveArcs(s, n);
  }

  virtual void InitStateIterator(StateIteratorData<A> *data) const {
    impl_->InitStateIterator(data);
  }

  virtual void InitArcIterator(StateId s, ArcIteratorData<A> *data) const {
    impl_->InitArcIterator(s, data);
  }

  virtual inline
  void InitMutableArcIterator(StateId s, MutableArcIteratorData<A> *);

 private:
  explicit BasicFst(BasicFstImpl<A> *impl) : impl_(impl) {}

  void MutateCheck() {
    // Copy on write
    if (impl_->RefCount() > 1) {
      impl_->DecrRefCount();
      impl_ = new BasicFstImpl<A>(*this);
    }
  }

  BasicFstImpl<A> *impl_;  // FST's impl
};

// Specialization for BasicFst; see generic version in fst.h
// for sample usage (but use the BasicFst type!). This version
// should inline.
template <class A>
class StateIterator< BasicFst<A> > {
 public:
  typedef typename A::StateId StateId;

  explicit StateIterator(const BasicFst<A> &fst)
    : nstates_(fst.impl_->NumStates()), s_(0) {}

  bool Done() const { return s_ >= nstates_; }

  StateId Value() const { return s_; }

  void Next() { ++s_; }

  void Reset() { s_ = 0; }

 private:
  StateId nstates_;
  StateId s_;

  DISALLOW_EVIL_CONSTRUCTORS(StateIterator);
};

// Specialization for BasicFst; see generic version in fst.h
// for sample usage (but use the BasicFst type!). This version
// should inline.
template <class A>
class ArcIterator< BasicFst<A> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const BasicFst<A> &fst, StateId s)
    : arcs_(fst.impl_->GetState(s)->arcs), i_(0) {}

  bool Done() const { return i_ >= arcs_.size(); }

  const A& Value() const { return arcs_[i_]; }

  void Next() { ++i_; }

  void Reset() { i_ = 0; }

  void Seek(size_t a) { i_ = a; }

 private:
  const vector<A>& arcs_;
  size_t i_;

  DISALLOW_EVIL_CONSTRUCTORS(ArcIterator);
};

// Specialization for BasicFst; see generic version in fst.h
// for sample usage (but use the BasicFst type!). This version
// should inline.
template <class A>
class MutableArcIterator< BasicFst<A> >
  : public MutableArcIteratorBase<A> {
 public:
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  MutableArcIterator(BasicFst<A> *fst, StateId s) : i_(0) {
    fst->MutateCheck();
    state_ = fst->impl_->GetState(s);
    properties_ = &fst->impl_->properties_;
  }

  virtual bool Done() const { return i_ >= state_->arcs.size(); }

  virtual const A& Value() const { return state_->arcs[i_]; }

  virtual void Next() { ++i_; }

  virtual void Reset() { i_ = 0; }

  virtual void Seek(size_t a) { i_ = a; }

  virtual void SetValue(const A &arc) {
    A& oarc = state_->arcs[i_];
    if (oarc.ilabel == 0)
      --state_->niepsilons;
    if (oarc.olabel == 0)
      --state_->noepsilons;
    oarc = arc;
    if (arc.ilabel == 0)
      ++state_->niepsilons;
    if (arc.olabel == 0)
      ++state_->noepsilons;
    *properties_ &= kSetArcProperties;
  }

 private:
  struct VectorState<A> *state_;
  uint64 *properties_;
  size_t i_;

  DISALLOW_EVIL_CONSTRUCTORS(MutableArcIterator);
};

// Provide information needed for the generic mutable arc iterator
template <class A> inline
void BasicFst<A>::InitMutableArcIterator(
    StateId s, MutableArcIteratorData<A> *data) {
  data->base = new MutableArcIterator< BasicFst<A> >(this, s);
}

// A useful alias when using StdArc.
typedef BasicFst<StdArc> StdBasicFst;

}  // namespace fst

#endif  // FST_TEST_BASIC_FST_H__
