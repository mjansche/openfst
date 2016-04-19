// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// FST Class for memory-efficient representation of common types of
// FSTs: linear automata, acceptors, unweighted FSTs, ...

#ifndef FST_LIB_COMPACT_FST_H_
#define FST_LIB_COMPACT_FST_H_

#include <iterator>
#include <utility>
#include <vector>

#include <fst/cache.h>
#include <fst/expanded-fst.h>
#include <fst/fst-decl.h>  // For optional argument declarations
#include <fst/mapped-file.h>
#include <fst/matcher.h>
#include <fst/test-properties.h>
#include <fst/util.h>


namespace fst {

struct CompactFstOptions : public CacheOptions {
  // CompactFst default caching behaviour is to do no caching. Most
  // compactors are cheap and therefore we save memory by not doing
  // caching.
  CompactFstOptions() : CacheOptions(true, 0) {}
  CompactFstOptions(const CacheOptions &opts) : CacheOptions(opts) {}
};

// Compactor Interface - class determinies how arcs and final weights
// are compacted and expanded.
//
// Final weights are treated as transitions to the superfinal state,
// i.e. ilabel = olabel = kNoLabel and nextstate = kNoStateId.
//
// There are two types of compactors:
//
// * Fixed out-degree compactors: 'compactor.Size()' returns a
// positive integer 's'. An FST can be compacted by this compactor
// only if each state has exactly 's' outgoing transitions (counting a
// non-Zero() final weight as a transition). A typical example is a
// compactor for string FSTs, i.e. 's == 1'.
//
// * Variable out-degree compactors: 'compactor.Size() == -1'. There
// are no out-degree restrictions for these compactors.
//
//
// class Compactor {
//  public:
//   // Element is the type of the compacted transitions.
//   typedef ... Element;
//   // Return the compacted representation of a transition 'arc'
//   // at a state 's'.
//   Element Compact(StateId s, const Arc &arc);
//   // Return the transition at state 's' represented by the compacted
//   // transition 'e'.
//   Arc Expand(StateId s, const Element &e);
//   // Return -1 for variable out-degree compactors, and the mandatory
//   // out-degree otherwise.
//   ssize_t Size();
//   // Test whether 'fst' can be compacted by this compactor.
//   bool Compatible(const Fst<A> &fst);
//   // Return the properties that are always true for an fst
//   // compacted using this compactor
//   uint64 Properties();
//   // Return a string identifying the type of compactor.
//   static const string &Type();
//   // Write a compactor to a file.
//   bool Write(std::ostream &strm);
//   // Read a compactor from a file.
//   static Compactor *Read(std::istream &strm);
//   // Default constructor (optional, see comment below).
//   Compactor();
// };
//
// The default constructor is only required for FST_REGISTER to work
// (i.e. enabling Convert() and the command-line utilities to work
// with this new compactor). However, a default constructor always
// needs to be specified for this code to compile, but one can have it
// simply raise an error when called:
//
// Compactor::Compactor() {
//   FSTERROR() << "Compactor: No default constructor";
// }

// Default implementation data for Compact Fst, which can shared
// between otherwise independent copies.
//
// The implementation contains two arrays: 'states_' and 'compacts_'.
//
// For fixed out-degree compactors, the 'states_' array is unallocated.
// The 'compacts_' contains the compacted transitions. Its size is
// 'ncompacts_'. The outgoing transitions at a given state are stored
// consecutively. For a given state 's', its 'compactor.Size()' outgoing
// transitions (including superfinal transition when 's' is final), are
// stored in position ['s*compactor.Size()', '(s+1)*compactor.Size()').
//
// For variable out-degree compactors, the states_ array has size
// 'nstates_ + 1' and contains pointers to positions into 'compacts_'.
// For a given state 's', the compacted transitions of 's' are
// stored in positions [ 'states_[s]', 'states_[s + 1]' ) in 'compacts_'.
// By convention, 'states_[nstates_] == ncompacts_'.
//
// In both cases, the superfinal transitions (when 's' is final, i.e.
// 'Final(s) != Weight::Zero()') are stored first.
//
// The unsigned type U is used to represent indices into the compacts_
// array.
template <class E, class U>
class DefaultCompactStore {
 public:
  typedef E CompactElement;
  typedef U Unsigned;

  DefaultCompactStore()
      : states_region_(0),
        compacts_region_(0),
        states_(0),
        compacts_(0),
        nstates_(0),
        ncompacts_(0),
        narcs_(0),
        start_(kNoStateId),
        error_(false) {}

  template <class A, class Compactor>
  DefaultCompactStore(const Fst<A> &fst, const Compactor &compactor);

  template <class Iterator, class Compactor>
  DefaultCompactStore(const Iterator &begin, const Iterator &end,
                      const Compactor &compactor);

  ~DefaultCompactStore() {
    if (states_region_ == nullptr) {
      delete[] states_;
    }
    delete states_region_;
    if (compacts_region_ == nullptr) {
      delete[] compacts_;
    }
    delete compacts_region_;
  }

  template <class Compactor>
  static DefaultCompactStore<E, U> *Read(std::istream &strm,
                                         const FstReadOptions &opts,
                                         const FstHeader &hdr,
                                         const Compactor &compactor);

  bool Write(std::ostream &strm, const FstWriteOptions &opts) const;

  Unsigned States(ssize_t i) const { return states_[i]; }
  const CompactElement &Compacts(size_t i) const { return compacts_[i]; }
  size_t NumStates() const { return nstates_; }
  size_t NumCompacts() const { return ncompacts_; }
  size_t NumArcs() const { return narcs_; }
  ssize_t Start() const { return start_; }

  bool Error() const { return error_; }

  // Returns a string identifying the type of data storage container.
  static const string &Type();

 private:
  MappedFile *states_region_;
  MappedFile *compacts_region_;
  Unsigned *states_;
  CompactElement *compacts_;
  size_t nstates_;
  size_t ncompacts_;
  size_t narcs_;
  ssize_t start_;
  bool error_;
};

template <class E, class U>
template <class A, class C>
DefaultCompactStore<E, U>::DefaultCompactStore(const Fst<A> &fst,
                                               const C &compactor)
    : states_region_(0),
      compacts_region_(0),
      states_(0),
      compacts_(0),
      nstates_(0),
      ncompacts_(0),
      narcs_(0),
      start_(kNoStateId),
      error_(false) {
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  start_ = fst.Start();
  // Count # of states and arcs.
  StateId nfinals = 0;
  for (StateIterator<Fst<A>> siter(fst); !siter.Done(); siter.Next()) {
    ++nstates_;
    StateId s = siter.Value();
    for (ArcIterator<Fst<A>> aiter(fst, s); !aiter.Done(); aiter.Next())
      ++narcs_;
    if (fst.Final(s) != Weight::Zero()) ++nfinals;
  }
  if (compactor.Size() == -1) {
    states_ = new Unsigned[nstates_ + 1];
    ncompacts_ = narcs_ + nfinals;
    compacts_ = new CompactElement[ncompacts_];
    states_[nstates_] = ncompacts_;
  } else {
    states_ = 0;
    ncompacts_ = nstates_ * compactor.Size();
    if ((narcs_ + nfinals) != ncompacts_) {
      FSTERROR() << "DefaultCompactStore: Compactor incompatible with Fst";
      error_ = true;
      return;
    }
    compacts_ = new CompactElement[ncompacts_];
  }
  size_t pos = 0, fpos = 0;
  for (StateId s = 0; s < nstates_; ++s) {
    fpos = pos;
    if (compactor.Size() == -1) states_[s] = pos;
    if (fst.Final(s) != Weight::Zero())
      compacts_[pos++] =
          compactor.Compact(s, A(kNoLabel, kNoLabel, fst.Final(s), kNoStateId));
    for (ArcIterator<Fst<A>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      compacts_[pos++] = compactor.Compact(s, aiter.Value());
    }
    if ((compactor.Size() != -1) && ((pos - fpos) != compactor.Size())) {
      FSTERROR() << "DefaultCompactStore: Compactor incompatible with Fst";
      error_ = true;
      return;
    }
  }
  if (pos != ncompacts_) {
    FSTERROR() << "DefaultCompactStore: Compactor incompatible with Fst";
    error_ = true;
    return;
  }
}

template <class E, class U>
template <class Iterator, class C>
DefaultCompactStore<E, U>::DefaultCompactStore(const Iterator &begin,
                                               const Iterator &end,
                                               const C &compactor)
    : states_region_(0),
      compacts_region_(0),
      states_(0),
      compacts_(0),
      nstates_(0),
      ncompacts_(0),
      narcs_(0),
      start_(kNoStateId),
      error_(false) {
  typedef typename C::Arc Arc;
  typedef typename Arc::Weight Weight;
  if (compactor.Size() != -1) {
    ncompacts_ = std::distance(begin, end);
    if (compactor.Size() == 1) {
      // For strings, allow implicit final weight.
      // Empty input is the empty string.
      if (ncompacts_ == 0) {
        ++ncompacts_;
      } else {
        Arc arc = compactor.Expand(ncompacts_ - 1, *(begin + (ncompacts_ - 1)));
        if (arc.ilabel != kNoLabel) ++ncompacts_;
      }
    }
    if (ncompacts_ % compactor.Size()) {
      FSTERROR() << "DefaultCompactStore: Size of input container incompatible"
                 << " with compactor";
      error_ = true;
      return;
    }
    if (ncompacts_ == 0) return;
    start_ = 0;
    nstates_ = ncompacts_ / compactor.Size();
    compacts_ = new CompactElement[ncompacts_];
    size_t i = 0;
    Iterator it = begin;
    for (; it != end; ++it, ++i) {
      compacts_[i] = *it;
      if (compactor.Expand(i, *it).ilabel != kNoLabel) ++narcs_;
    }
    if (i < ncompacts_)
      compacts_[i] = compactor.Compact(
          i, Arc(kNoLabel, kNoLabel, Weight::One(), kNoStateId));
  } else {
    if (std::distance(begin, end) == 0) return;
    // Count # of states, arcs and compacts.
    Iterator it = begin;
    for (size_t i = 0; it != end; ++it, ++i) {
      Arc arc = compactor.Expand(i, *it);
      if (arc.ilabel != kNoLabel) {
        ++narcs_;
        ++ncompacts_;
      } else {
        ++nstates_;
        if (arc.weight != Weight::Zero()) ++ncompacts_;
      }
    }
    start_ = 0;
    compacts_ = new CompactElement[ncompacts_];
    states_ = new Unsigned[nstates_ + 1];
    states_[nstates_] = ncompacts_;
    size_t i = 0, s = 0;
    for (it = begin; it != end; ++it) {
      Arc arc = compactor.Expand(i, *it);
      if (arc.ilabel != kNoLabel) {
        compacts_[i++] = *it;
      } else {
        states_[s++] = i;
        if (arc.weight != Weight::Zero()) compacts_[i++] = *it;
      }
    }
    if ((s != nstates_) || (i != ncompacts_)) {
      FSTERROR() << "DefaultCompactStore: Ill-formed input container";
      error_ = true;
      return;
    }
  }
}

template <class E, class U>
template <class C>
DefaultCompactStore<E, U> *DefaultCompactStore<E, U>::Read(
    std::istream &strm, const FstReadOptions &opts, const FstHeader &hdr,
    const C &compactor) {
  DefaultCompactStore<E, U> *data = new DefaultCompactStore<E, U>();
  data->start_ = hdr.Start();
  data->nstates_ = hdr.NumStates();
  data->narcs_ = hdr.NumArcs();

  if (compactor.Size() == -1) {
    if ((hdr.GetFlags() & FstHeader::IS_ALIGNED) && !AlignInput(strm)) {
      LOG(ERROR) << "DefaultCompactStore::Read: Alignment failed: "
                 << opts.source;
      delete data;
      return 0;
    }
    size_t b = (data->nstates_ + 1) * sizeof(Unsigned);
    data->states_region_ = MappedFile::Map(
        &strm, opts.mode == FstReadOptions::MAP, opts.source, b);
    if (!strm || data->states_region_ == nullptr) {
      LOG(ERROR) << "DefaultCompactStore::Read: Read failed: " << opts.source;
      delete data;
      return 0;
    }
    data->states_ =
        static_cast<Unsigned *>(data->states_region_->mutable_data());
  } else {
    data->states_ = 0;
  }
  data->ncompacts_ = compactor.Size() == -1 ? data->states_[data->nstates_]
                                            : data->nstates_ * compactor.Size();
  if ((hdr.GetFlags() & FstHeader::IS_ALIGNED) && !AlignInput(strm)) {
    LOG(ERROR) << "DefaultCompactStore::Read: Alignment failed: "
               << opts.source;
    delete data;
    return 0;
  }
  size_t b = data->ncompacts_ * sizeof(CompactElement);
  data->compacts_region_ =
      MappedFile::Map(&strm, opts.mode == FstReadOptions::MAP, opts.source, b);
  if (!strm || data->compacts_region_ == nullptr) {
    LOG(ERROR) << "DefaultCompactStore::Read: Read failed: " << opts.source;
    delete data;
    return 0;
  }
  data->compacts_ =
      static_cast<CompactElement *>(data->compacts_region_->mutable_data());
  return data;
}

template <class E, class U>
bool DefaultCompactStore<E, U>::Write(std::ostream &strm,
                                      const FstWriteOptions &opts) const {
  if (states_) {
    if (opts.align && !AlignOutput(strm)) {
      LOG(ERROR) << "DefaultCompactStore::Write: Alignment failed: "
                 << opts.source;
      return false;
    }
    strm.write(reinterpret_cast<char *>(states_),
               (nstates_ + 1) * sizeof(Unsigned));
  }
  if (opts.align && !AlignOutput(strm)) {
    LOG(ERROR) << "DefaultCompactStore::Write: Alignment failed: "
               << opts.source;
    return false;
  }
  strm.write(reinterpret_cast<char *>(compacts_),
             ncompacts_ * sizeof(CompactElement));

  strm.flush();
  if (!strm) {
    LOG(ERROR) << "DefaultCompactStore::Write: Write failed: " << opts.source;
    return false;
  }
  return true;
}

template <class E, class U>
const string &DefaultCompactStore<E, U>::Type() {
  static const string type = "compact";
  return type;
}

template <class A, class C, class U, class S>
class CompactFst;
template <class F, class G>
void Cast(const F &, G *);

// Implementation class for CompactFst, which contains parametrizeable
// Fst data storage (DefaultCompactStore by default) and Fst cache.
template <class A, class C, class U,
          class S = DefaultCompactStore<typename C::Element, U>>
class CompactFstImpl : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;
  using FstImpl<A>::WriteHeader;

  using CacheImpl<A>::PushArc;
  using CacheImpl<A>::HasArcs;
  using CacheImpl<A>::HasFinal;
  using CacheImpl<A>::HasStart;
  using CacheImpl<A>::SetArcs;
  using CacheImpl<A>::SetFinal;
  using CacheImpl<A>::SetStart;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef C Compactor;
  typedef typename C::Element CompactElement;
  typedef U Unsigned;
  typedef S DataStorage;

  CompactFstImpl()
      : CacheImpl<A>(CompactFstOptions()),
        compactor_(0),
        own_compactor_(false),
        data_() {
    string type = "compact";
    if (sizeof(U) != sizeof(uint32)) {
      string size;
      Int64ToStr(8 * sizeof(U), &size);
      type += size;
    }
    type += "_";
    type += C::Type();
    if (DataStorage::Type() != "compact") {
      type += "_";
      type += DataStorage::Type();
    }
    SetType(type);
    SetProperties(kNullProperties | kStaticProperties);
  }

  CompactFstImpl(const Fst<Arc> &fst, const C &compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(new C(compactor)),
        own_compactor_(true),
        data_() {
    Init(fst);
  }

  CompactFstImpl(const Fst<Arc> &fst, C *compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(compactor),
        own_compactor_(false),
        data_() {
    Init(fst);
  }

  template <class Iterator>
  CompactFstImpl(const Iterator &b, const Iterator &e, const C &compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(new C(compactor)),
        own_compactor_(true),
        data_() {
    Init(b, e);
  }

  template <class Iterator>
  CompactFstImpl(const Iterator &b, const Iterator &e, C *compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(compactor),
        own_compactor_(false),
        data_() {
    Init(b, e);
  }

  CompactFstImpl(const CompactFstImpl<A, C, U, S> &impl)
      : CacheImpl<A>(impl),
        compactor_(impl.compactor_ == nullptr ? nullptr
                                              : new C(*impl.compactor_)),
        own_compactor_(true),
        data_(impl.data_) {
    SetType(impl.Type());
    SetProperties(impl.Properties());
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  ~CompactFstImpl() override {
    if (own_compactor_) delete compactor_;
  }

  StateId Start() {
    if (!HasStart()) {
      SetStart(data_->Start());
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (HasFinal(s)) return CacheImpl<A>::Final(s);
    Arc arc(kNoLabel, kNoLabel, Weight::Zero(), kNoStateId);
    if ((compactor_->Size() != -1) ||
        (data_->States(s) != data_->States(s + 1)))
      arc = ComputeArc(s, compactor_->Size() == -1 ? data_->States(s)
                                                   : s * compactor_->Size());
    return arc.ilabel == kNoLabel ? arc.weight : Weight::Zero();
  }

  StateId NumStates() const {
    if (Properties(kError)) return 0;
    return data_->NumStates();
  }

  size_t NumArcs(StateId s) {
    if (HasArcs(s)) return CacheImpl<A>::NumArcs(s);
    Unsigned i, num_arcs;
    if (compactor_->Size() == -1) {
      i = data_->States(s);
      num_arcs = data_->States(s + 1) - i;
    } else {
      i = s * compactor_->Size();
      num_arcs = compactor_->Size();
    }
    if (num_arcs > 0) {
      const A &arc = ComputeArc(s, i, kArcILabelValue);
      if (arc.ilabel == kNoStateId) {
        --num_arcs;
      }
    }
    return num_arcs;
  }

  size_t NumInputEpsilons(StateId s) {
    if (!HasArcs(s) && !Properties(kILabelSorted)) Expand(s);
    if (HasArcs(s)) return CacheImpl<A>::NumInputEpsilons(s);
    return CountEpsilons(s, false);
  }

  size_t NumOutputEpsilons(StateId s) {
    if (!HasArcs(s) && !Properties(kOLabelSorted)) Expand(s);
    if (HasArcs(s)) return CacheImpl<A>::NumOutputEpsilons(s);
    return CountEpsilons(s, true);
  }

  size_t CountEpsilons(StateId s, bool output_epsilons) {
    size_t begin =
        compactor_->Size() == -1 ? data_->States(s) : s * compactor_->Size();
    size_t end = compactor_->Size() == -1 ? data_->States(s + 1)
                                          : (s + 1) * compactor_->Size();
    size_t num_eps = 0;
    for (size_t i = begin; i < end; ++i) {
      const A &arc =
          ComputeArc(s, i, output_epsilons ? kArcOLabelValue : kArcILabelValue);
      const typename A::Label &label =
          (output_epsilons ? arc.olabel : arc.ilabel);
      if (label == kNoLabel)
        continue;
      else if (label > 0)
        break;
      ++num_eps;
    }
    return num_eps;
  }

  static CompactFstImpl<A, C, U, S> *Read(std::istream &strm,
                                          const FstReadOptions &opts) {
    CompactFstImpl<A, C, U, S> *impl = new CompactFstImpl<A, C, U, S>();
    FstHeader hdr;
    if (!impl->ReadHeader(strm, opts, kMinFileVersion, &hdr)) {
      delete impl;
      return 0;
    }

    // Ensures compatibility
    if (hdr.Version() == kAlignedFileVersion)
      hdr.SetFlags(hdr.GetFlags() | FstHeader::IS_ALIGNED);

    impl->compactor_ = C::Read(strm);
    if (!impl->compactor_) {
      delete impl;
      return nullptr;
    }
    impl->own_compactor_ = true;
    impl->data_ = std::shared_ptr<DataStorage>(
        DataStorage::Read(strm, opts, hdr, *impl->compactor_));
    if (!impl->data_) {
      delete impl;
      return nullptr;
    }
    return impl;
  }

  bool Write(std::ostream &strm, const FstWriteOptions &opts) const {
    FstHeader hdr;
    hdr.SetStart(data_->Start());
    hdr.SetNumStates(data_->NumStates());
    hdr.SetNumArcs(data_->NumArcs());

    // Ensures compatibility
    int file_version = opts.align ? kAlignedFileVersion : kFileVersion;
    WriteHeader(strm, opts, file_version, &hdr);
    compactor_->Write(strm);
    return data_->Write(strm, opts);
  }

  // Provide information needed for generic state iterator
  void InitStateIterator(StateIteratorData<A> *data) const {
    data->base = 0;
    data->nstates = data_->NumStates();
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    if (!HasArcs(s)) Expand(s);
    CacheImpl<A>::InitArcIterator(s, data);
  }

  Arc ComputeArc(StateId s, Unsigned i, uint32 f = kArcValueFlags) const {
    return compactor_->Expand(s, data_->Compacts(i), f);
  }

  void Expand(StateId s) {
    size_t begin =
        compactor_->Size() == -1 ? data_->States(s) : s * compactor_->Size();
    size_t end = compactor_->Size() == -1 ? data_->States(s + 1)
                                          : (s + 1) * compactor_->Size();
    for (size_t i = begin; i < end; ++i) {
      const Arc &arc = ComputeArc(s, i);
      if (arc.ilabel == kNoLabel)
        SetFinal(s, arc.weight);
      else
        PushArc(s, arc);
    }
    if (!HasFinal(s)) SetFinal(s, Weight::Zero());
    SetArcs(s);
  }

  template <class Iterator>
  void SetCompactElements(const Iterator &b, const Iterator &e) {
    SetProperties(kStaticProperties | compactor_->Properties());
    data_ = std::make_shared<DataStorage>(b, e, *compactor_);
    if (data_->Error()) SetProperties(kError, kError);
  }

  C *GetCompactor() const { return compactor_; }
  const DataStorage *Data() const { return data_.get(); }
  std::shared_ptr<DataStorage> SharedData() const { return data_; }

  // Properties always true of this Fst class
  static const uint64 kStaticProperties = kExpanded;

 protected:
  template <class OtherA, class OtherC>
  explicit CompactFstImpl(const CompactFstImpl<OtherA, OtherC, U, S> &impl)
      : CacheImpl<A>(CacheOptions(impl.GetCacheGc(), impl.GetCacheLimit())),
        compactor_(new C(*impl.GetCompactor())),
        own_compactor_(true),
        data_(impl.SharedData()) {
    SetType(impl.Type());
    SetProperties(impl.Properties());
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

 private:
  friend class CompactFst<A, C, U, S>;  // allow access during write.

  void Init(const Fst<Arc> &fst) {
    string type = "compact";
    if (sizeof(U) != sizeof(uint32)) {
      string size;
      Int64ToStr(8 * sizeof(U), &size);
      type += size;
    }
    type += "_";
    type += compactor_->Type();
    if (DataStorage::Type() != "compact") {
      type += "_";
      type += DataStorage::Type();
    }
    SetType(type);
    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
    data_ = std::make_shared<DataStorage>(fst, *compactor_);
    if (data_->Error()) SetProperties(kError, kError);
    uint64 copy_properties = fst.Properties(kMutable, false) ?
        fst.Properties(kCopyProperties, true):
        CheckProperties(fst,
                        kCopyProperties & ~kWeightedCycles & ~kUnweightedCycles,
                        kCopyProperties);
    if ((copy_properties & kError) || !compactor_->Compatible(fst)) {
      FSTERROR() << "CompactFstImpl: Input Fst incompatible with compactor";
      SetProperties(kError, kError);
      return;
    }
    SetProperties(copy_properties | kStaticProperties);
  }

  template <class Iterator>
  void Init(const Iterator &b, const Iterator &e) {
    string type = "compact";
    if (sizeof(U) != sizeof(uint32)) {
      string size;
      Int64ToStr(8 * sizeof(U), &size);
      type += size;
    }
    type += "_";
    type += compactor_->Type();
    SetType(type);
    SetProperties(kStaticProperties | compactor_->Properties());
    data_ = std::make_shared<DataStorage>(b, e, *compactor_);
    if (data_->Error()) SetProperties(kError, kError);
  }

  // Current unaligned file format version
  static const int kFileVersion = 2;
  // Current aligned file format version
  static const int kAlignedFileVersion = 1;
  // Minimum file format version supported
  static const int kMinFileVersion = 1;

  C *compactor_;
  bool own_compactor_;
  std::shared_ptr<DataStorage> data_;
};

template <class A, class C, class U, class S>
const uint64 CompactFstImpl<A, C, U, S>::kStaticProperties;
template <class A, class C, class U, class S>
const int CompactFstImpl<A, C, U, S>::kFileVersion;
template <class A, class C, class U, class S>
const int CompactFstImpl<A, C, U, S>::kAlignedFileVersion;
template <class A, class C, class U, class S>
const int CompactFstImpl<A, C, U, S>::kMinFileVersion;

// CompactFst.  This class attaches interface to implementation and
// handles reference counting, delegating most methods to
// ImplToExpandedFst. The unsigned type U is used to represent indices
// into the compact arc array. Type S represents the data storage.
// (Template arg defaults declared in fst-decl.h.)
template <class A, class C, class U /* = uint32 */,
          class S /* = DefaultCompactStore<typename C::Element, U> */>
class CompactFst : public ImplToExpandedFst<CompactFstImpl<A, C, U, S>> {
 public:
  friend class StateIterator<CompactFst<A, C, U, S>>;
  friend class ArcIterator<CompactFst<A, C, U, S>>;
  template <class F, class G>
  void friend Cast(const F &, G *);

  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef CompactFstImpl<A, C, U, S> Impl;
  typedef DefaultCacheStore<A> Store;
  typedef typename Store::State State;
  typedef U Unsigned;

  CompactFst() : ImplToExpandedFst<Impl>(std::make_shared<Impl>()) {}

  explicit CompactFst(const Fst<A> &fst, const C &compactor = C(),
                      const CompactFstOptions &opts = CompactFstOptions())
      : ImplToExpandedFst<Impl>(std::make_shared<Impl>(fst, compactor, opts)) {}

  CompactFst(const Fst<A> &fst, C *compactor,
             const CompactFstOptions &opts = CompactFstOptions())
      : ImplToExpandedFst<Impl>(std::make_shared<Impl>(fst, compactor, opts)) {}

  // The following 2 constructors take as input two iterators delimiting
  // a set of (already) compacted transitions, starting with the
  // transitions out of the initial state. The format of the input
  // differs for fixed out-degree and variable out-degree compactors.
  //
  // - For fixed out-degree compactors, the final weight (encoded as a
  // compacted transition) needs to be given only for final
  // states. All strings (compactor of size 1) will be assume to be
  // terminated by a final state even when the final state is not
  // implicitely given.
  //
  // - For variable out-degree compactors, the final weight (encoded
  // as a compacted transition) needs to be given for all states and
  // must appeared first in the list (for state s, final weight of s,
  // followed by outgoing transitons in s).
  //
  // These 2 constructors allows the direct construction of a CompactFst
  // without first creating a more memory hungry 'regular' FST. This
  // is useful when memory usage is severely constrained.
  template <class Iterator>
  explicit CompactFst(const Iterator &begin, const Iterator &end,
                      const C &compactor = C(),
                      const CompactFstOptions &opts = CompactFstOptions())
      : ImplToExpandedFst<Impl>(
            std::make_shared<Impl>(begin, end, compactor, opts)) {}

  template <class Iterator>
  CompactFst(const Iterator &begin, const Iterator &end, C *compactor,
             const CompactFstOptions &opts = CompactFstOptions())
      : ImplToExpandedFst<Impl>(
            std::make_shared<Impl>(begin, end, compactor, opts)) {}

  // See Fst<>::Copy() for doc.
  CompactFst(const CompactFst<A, C, U, S> &fst, bool safe = false)
      : ImplToExpandedFst<Impl>(fst, safe) {}

  // Get a copy of this CompactFst. See Fst<>::Copy() for further doc.
  CompactFst<A, C, U, S> *Copy(bool safe = false) const override {
    return new CompactFst<A, C, U, S>(*this, safe);
  }

  // Read a CompactFst from an input stream; return nullptr on error
  static CompactFst<A, C, U, S> *Read(std::istream &strm,
                                      const FstReadOptions &opts) {
    Impl *impl = Impl::Read(strm, opts);
    return impl ? new CompactFst<A, C, U, S>(std::shared_ptr<Impl>(impl))
                : nullptr;
  }

  // Read a CompactFst from a file; return nullptr on error
  // Empty filename reads from standard input
  static CompactFst<A, C, U, S> *Read(const string &filename) {
    Impl *impl = ImplToExpandedFst<Impl>::Read(filename);
    return impl ? new CompactFst<A, C, U, S>(std::shared_ptr<Impl>(impl))
                : nullptr;
  }

  bool Write(std::ostream &strm, const FstWriteOptions &opts) const override {
    return GetImpl()->Write(strm, opts);
  }

  bool Write(const string &filename) const override {
    return Fst<A>::WriteFile(filename);
  }

  template <class F>
  static bool WriteFst(const F &fst, const C &compactor, std::ostream &strm,
                       const FstWriteOptions &opts);

  void InitStateIterator(StateIteratorData<A> *data) const override {
    GetImpl()->InitStateIterator(data);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) const override {
    GetMutableImpl()->InitArcIterator(s, data);
  }

  MatcherBase<A> *InitMatcher(MatchType match_type) const override {
    return new SortedMatcher<CompactFst<A, C, U, S>>(*this, match_type);
  }

  template <class Iterator>
  void SetCompactElements(const Iterator &b, const Iterator &e) {
    GetMutableImpl()->SetCompactElements(b, e);
  }

 private:
  using ImplToFst<Impl, ExpandedFst<A>>::GetImpl;
  using ImplToFst<Impl, ExpandedFst<A>>::GetMutableImpl;

  explicit CompactFst(std::shared_ptr<Impl> impl)
      : ImplToExpandedFst<Impl>(impl) {}

  // Use overloading to extract the type of the argument.
  static Impl *GetImplIfCompactFst(const CompactFst<A, C, U, S> &compact_fst) {
    return compact_fst.GetImpl();
  }

  // This does not give privileged treatment to subclasses of CompactFst.
  template <typename NonCompactFst>
  static Impl *GetImplIfCompactFst(const NonCompactFst &fst) {
    return nullptr;
  }

  void operator=(const CompactFst<A, C, U, S> &fst);  // disallow
};

// Writes Fst in Compact format, potentially with a pass over the machine
// before writing to compute the number of states and arcs.
//
template <class A, class C, class U, class S>
template <class F>
bool CompactFst<A, C, U, S>::WriteFst(const F &fst, const C &compactor,
                                      std::ostream &strm,
                                      const FstWriteOptions &opts) {
  typedef U Unsigned;
  typedef typename C::Element CompactElement;
  typedef typename A::Weight Weight;
  int file_version =
      opts.align ? Impl::kAlignedFileVersion : Impl::kFileVersion;
  size_t num_arcs = -1, num_states = -1;
  C first_pass_compactor = compactor;
  if (Impl *impl = GetImplIfCompactFst(fst)) {
    num_arcs = impl->Data()->NumArcs();
    num_states = impl->Data()->NumStates();
    first_pass_compactor = *impl->GetCompactor();
  } else {
    // A first pass is needed to compute the state of the compactor, which
    // is saved ahead of the rest of the data structures.  This unfortunately
    // means forcing a complete double compaction when writing in this format.
    // TODO(allauzen): eliminate mutable state from compactors.
    num_arcs = 0;
    num_states = 0;
    for (StateIterator<F> siter(fst); !siter.Done(); siter.Next()) {
      const StateId s = siter.Value();
      ++num_states;
      if (fst.Final(s) != Weight::Zero()) {
        first_pass_compactor.Compact(
            s, A(kNoLabel, kNoLabel, fst.Final(s), kNoStateId));
      }
      for (ArcIterator<F> aiter(fst, s); !aiter.Done(); aiter.Next()) {
        ++num_arcs;
        first_pass_compactor.Compact(s, aiter.Value());
      }
    }
  }
  FstHeader hdr;
  hdr.SetStart(fst.Start());
  hdr.SetNumStates(num_states);
  hdr.SetNumArcs(num_arcs);
  string type = "compact";
  if (sizeof(U) != sizeof(uint32)) {
    string size;
    Int64ToStr(8 * sizeof(U), &size);
    type += size;
  }
  type += "_";
  type += C::Type();
  if (S::Type() != "compact") {
    type += "_";
    type += S::Type();
  }
  uint64 copy_properties = fst.Properties(kCopyProperties, true);
  if ((copy_properties & kError) || !compactor.Compatible(fst)) {
    FSTERROR() << "Fst incompatible with compactor";
    return false;
  }
  uint64 properties = copy_properties | Impl::kStaticProperties;
  FstImpl<A>::WriteFstHeader(fst, strm, opts, file_version, type, properties,
                             &hdr);
  first_pass_compactor.Write(strm);
  if (first_pass_compactor.Size() == -1) {
    if (opts.align && !AlignOutput(strm)) {
      LOG(ERROR) << "CompactFst::Write: Alignment failed: " << opts.source;
      return false;
    }
    Unsigned compacts = 0;
    for (StateIterator<F> siter(fst); !siter.Done(); siter.Next()) {
      const StateId s = siter.Value();
      strm.write(reinterpret_cast<const char *>(&compacts), sizeof(compacts));
      if (fst.Final(s) != Weight::Zero()) {
        ++compacts;
      }
      compacts += fst.NumArcs(s);
    }
    strm.write(reinterpret_cast<const char *>(&compacts), sizeof(compacts));
  }
  if (opts.align && !AlignOutput(strm)) {
    LOG(ERROR) << "Could not align file during write after writing states";
  }
  C second_pass_compactor = compactor;
  CompactElement element;
  for (StateIterator<F> siter(fst); !siter.Done(); siter.Next()) {
    const StateId s = siter.Value();
    if (fst.Final(s) != Weight::Zero()) {
      element = second_pass_compactor.Compact(
          s, A(kNoLabel, kNoLabel, fst.Final(s), kNoStateId));
      strm.write(reinterpret_cast<const char *>(&element), sizeof(element));
    }
    for (ArcIterator<F> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      element = second_pass_compactor.Compact(s, aiter.Value());
      strm.write(reinterpret_cast<const char *>(&element), sizeof(element));
    }
  }
  strm.flush();
  if (!strm) {
    LOG(ERROR) << "CompactFst write failed: " << opts.source;
    return false;
  }
  return true;
}

// Specialization for CompactFst; see generic version in fst.h
// for sample usage (but use the CompactFst type!). This version
// should inline.
template <class A, class C, class U>
class StateIterator<CompactFst<A, C, U>> {
 public:
  typedef typename A::StateId StateId;

  explicit StateIterator(const CompactFst<A, C, U> &fst)
      : nstates_(fst.GetImpl()->NumStates()), s_(0) {}

  bool Done() const { return s_ >= nstates_; }

  StateId Value() const { return s_; }

  void Next() { ++s_; }

  void Reset() { s_ = 0; }

 private:
  StateId nstates_;
  StateId s_;

  DISALLOW_COPY_AND_ASSIGN(StateIterator);
};

// Specialization for CompactFst.
// Never caches, always iterates over the underlying compact elements.
template <class A, class C, class U>
class ArcIterator<CompactFst<A, C, U>> {
 public:
  typedef typename A::StateId StateId;
  typedef typename C::Element CompactElement;

  ArcIterator(const CompactFst<A, C, U> &fst, StateId s)
      : compactor_(fst.GetImpl()->GetCompactor()),
        state_(s),
        compacts_(0),
        pos_(0),
        flags_(kArcValueFlags) {
    const DefaultCompactStore<CompactElement, U> *data = fst.GetImpl()->Data();
    size_t offset;
    if (compactor_->Size() == -1) {  // Variable out-degree compactor
      offset = data->States(s);
      num_arcs_ = data->States(s + 1) - offset;
    } else {  // Fixed out-degree compactor
      offset = s * compactor_->Size();
      num_arcs_ = compactor_->Size();
    }
    if (num_arcs_ > 0) {
      compacts_ = &(data->Compacts(offset));
      arc_ = compactor_->Expand(s, *compacts_, kArcILabelValue);
      if (arc_.ilabel == kNoStateId) {
        ++compacts_;
        --num_arcs_;
      }
    }
  }

  ~ArcIterator() {}

  bool Done() const { return pos_ >= num_arcs_; }

  const A &Value() const {
    arc_ = compactor_->Expand(state_, compacts_[pos_], flags_);
    return arc_;
  }

  void Next() { ++pos_; }

  size_t Position() const { return pos_; }

  void Reset() { pos_ = 0; }

  void Seek(size_t pos) { pos_ = pos; }

  uint32 Flags() const { return flags_; }

  void SetFlags(uint32 f, uint32 m) {
    flags_ &= ~m;
    flags_ |= (f & kArcValueFlags);
  }

 private:
  C *compactor_;
  StateId state_;
  const CompactElement *compacts_;
  size_t pos_;
  size_t num_arcs_;
  mutable A arc_;
  uint32 flags_;

  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};

// // Specialization for CompactFst.
// // This is an optionally caching arc iterator.
// // TODO(allauzen): implements the kArcValueFlags, the current
// // implementation only implements the kArcNoCache flag.
// template <class A, class C, class U>
// class ArcIterator< CompactFst<A, C, U>> {
//  public:
//   typedef typename A::StateId StateId;

//   ArcIterator(const CompactFst<A, C, U> &fst, StateId s)
//       : fst_(fst), state_(s), pos_(0), num_arcs_(0), offset_(0),
//         flags_(kArcValueFlags) {
//     cache_data_.ref_count = 0;

//     if (fst_.GetImpl()->HasArcs(state_)) {
//       fst_.GetImpl()->InitArcIterator(s, &cache_data_);
//       num_arcs_ = cache_data_.narcs;
//       return;
//     }

//     const C *compactor = fst_.GetImpl()->GetCompactor();
//     const DefaultCompactStore<A, C, U> *data = fst_.GetImpl()->Data();
//     if (compactor->Size() == -1) {  // Variable out-degree compactor
//       offset_ = data->States(s);
//       num_arcs_ = data->States(s + 1) - offset_;
//     } else {  // Fixed out-degree compactor
//       offset_ =  s * compactor->Size();
//       num_arcs_ = compactor->Size();
//     }
//     if (num_arcs_ > 0) {
//       const A &arc = fst_.GetImpl()->ComputeArc(s, offset_);
//       if (arc.ilabel == kNoStateId) {
//         ++offset_;
//         --num_arcs_;
//       }
//     }
//   }

//   ~ArcIterator() {
//     if (cache_data_.ref_count)
//       --(*cache_data_.ref_count);
//   }

//   bool Done() const { return pos_ >= num_arcs_; }

//   const A& Value() const {
//     if (cache_data_.ref_count == 0) {
//       if (flags_ & kArcNoCache) {
//         arc_ = fst_.GetImpl()->ComputeArc(state_, pos_ + offset_);
//         return arc_;
//       } else {
//         fst_.GetImpl()->InitArcIterator(state_, &cache_data_);
//       }
//     }
//     return cache_data_.arcs[pos_];
//   }

//   void Next() { ++pos_; }

//   size_t Position() const { return pos_; }

//   void Reset() { pos_ = 0;  }

//   void Seek(size_t pos) { pos_ = pos; }

//   uint32 Flags() const { return flags_; }

//   void SetFlags(uint32 f, uint32 m) {
//     flags_ &= ~m;
//     flags_ |= f;

//     if (!(flags_ & kArcNoCache) && cache_data_.ref_count == 0)
//       fst_.GetImpl()->InitArcIterator(state_, &cache_data_);
//   }

//  private:
//   mutable const CompactFst<A, C, U> &fst_;
//   StateId state_;
//   size_t pos_;
//   size_t num_arcs_;
//   size_t offset_;
//   uint32 flags_;
//   mutable A arc_;
//   mutable ArcIteratorData<A> cache_data_;

//   DISALLOW_COPY_AND_ASSIGN(ArcIterator);
// };

//
// Utility Compactors
//

// Compactor for unweighted string FSTs
template <class A>
class StringCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Element;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  Element Compact(StateId s, const A &arc) const { return arc.ilabel; }

  Arc Expand(StateId s, const Element &p, uint32 f = kArcValueFlags) const {
    return Arc(p, p, Weight::One(), p != kNoLabel ? s + 1 : kNoStateId);
  }

  ssize_t Size() const { return 1; }

  uint64 Properties() const { return kString | kAcceptor | kUnweighted; }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "string";
    return type;
  }

  bool Write(std::ostream &strm) const { return true; }

  static StringCompactor *Read(std::istream &strm) {
    return new StringCompactor;
  }
};

// Compactor for weighted string FSTs
template <class A>
class WeightedStringCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef std::pair<Label, Weight> Element;

  Element Compact(StateId s, const A &arc) const {
    return std::make_pair(arc.ilabel, arc.weight);
  }

  Arc Expand(StateId s, const Element &p, uint32 f = kArcValueFlags) const {
    return Arc(p.first, p.first, p.second,
               p.first != kNoLabel ? s + 1 : kNoStateId);
  }

  ssize_t Size() const { return 1; }

  uint64 Properties() const { return kString | kAcceptor; }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "weighted_string";
    return type;
  }

  bool Write(std::ostream &strm) const { return true; }

  static WeightedStringCompactor *Read(std::istream &strm) {
    return new WeightedStringCompactor;
  }
};

// Compactor for unweighted acceptor FSTs
template <class A>
class UnweightedAcceptorCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef std::pair<Label, StateId> Element;

  Element Compact(StateId s, const A &arc) const {
    return std::make_pair(arc.ilabel, arc.nextstate);
  }

  Arc Expand(StateId s, const Element &p, uint32 f = kArcValueFlags) const {
    return Arc(p.first, p.first, Weight::One(), p.second);
  }

  ssize_t Size() const { return -1; }

  uint64 Properties() const { return kAcceptor | kUnweighted; }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "unweighted_acceptor";
    return type;
  }

  bool Write(std::ostream &strm) const { return true; }

  static UnweightedAcceptorCompactor *Read(std::istream &istrm) {
    return new UnweightedAcceptorCompactor;
  }
};

// Compactor for weighted acceptor FSTs
template <class A>
class AcceptorCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef std::pair<std::pair<Label, Weight>, StateId> Element;

  Element Compact(StateId s, const A &arc) const {
    return std::make_pair(std::make_pair(arc.ilabel, arc.weight),
                          arc.nextstate);
  }

  Arc Expand(StateId s, const Element &p, uint32 f = kArcValueFlags) const {
    return Arc(p.first.first, p.first.first, p.first.second, p.second);
  }

  ssize_t Size() const { return -1; }

  uint64 Properties() const { return kAcceptor; }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "acceptor";
    return type;
  }

  bool Write(std::ostream &strm) const { return true; }

  static AcceptorCompactor *Read(std::istream &strm) {
    return new AcceptorCompactor;
  }
};

// Compactor for unweighted FSTs
template <class A>
class UnweightedCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef std::pair<std::pair<Label, Label>, StateId> Element;

  Element Compact(StateId s, const A &arc) const {
    return std::make_pair(std::make_pair(arc.ilabel, arc.olabel),
                          arc.nextstate);
  }

  Arc Expand(StateId s, const Element &p, uint32 f = kArcValueFlags) const {
    return Arc(p.first.first, p.first.second, Weight::One(), p.second);
  }

  ssize_t Size() const { return -1; }

  uint64 Properties() const { return kUnweighted; }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "unweighted";
    return type;
  }

  bool Write(std::ostream &strm) const { return true; }

  static UnweightedCompactor *Read(std::istream &strm) {
    return new UnweightedCompactor;
  }
};

// Useful aliases when using StdArc
typedef CompactFst<StdArc, StringCompactor<StdArc>> StdCompactStringFst;
typedef CompactFst<StdArc, WeightedStringCompactor<StdArc>>
    StdCompactWeightedStringFst;
typedef CompactFst<StdArc, AcceptorCompactor<StdArc>> StdCompactAcceptorFst;
typedef CompactFst<StdArc, UnweightedCompactor<StdArc>>
    StdCompactUnweightedFst;
typedef CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc>>
    StdCompactUnweightedAcceptorFst;

}  // namespace fst

#endif  // FST_LIB_COMPACT_FST_H_
