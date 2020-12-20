// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Simple concrete immutable FST whose states and arcs are each stored in
// single arrays.

#ifndef FST_LIB_CONST_FST_H_
#define FST_LIB_CONST_FST_H_

#include <string>
#include <vector>

#include <fst/expanded-fst.h>
#include <fst/fst-decl.h>  // For optional argument declarations
#include <fst/mapped-file.h>
#include <fst/test-properties.h>
#include <fst/util.h>


namespace fst {

template <class A, class U>
class ConstFst;
template <class F, class G>
void Cast(const F &, G *);

// States and arcs each implemented by single arrays, templated on the
// Arc definition. The unsigned type U is used to represent indices into
// the arc array.
template <class A, class U>
class ConstFstImpl : public FstImpl<A> {
 public:
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef U Unsigned;

  ConstFstImpl()
      : states_(nullptr),
        arcs_(nullptr),
        nstates_(0),
        narcs_(0),
        start_(kNoStateId) {
    string type = "const";
    if (sizeof(U) != sizeof(uint32)) {
      string size;
      Int64ToStr(8 * sizeof(U), &size);
      type += size;
    }
    SetType(type);
    SetProperties(kNullProperties | kStaticProperties);
  }

  explicit ConstFstImpl(const Fst<A> &fst);

  StateId Start() const { return start_; }

  Weight Final(StateId s) const { return states_[s].final; }

  StateId NumStates() const { return nstates_; }

  size_t NumArcs(StateId s) const { return states_[s].narcs; }

  size_t NumInputEpsilons(StateId s) const { return states_[s].niepsilons; }

  size_t NumOutputEpsilons(StateId s) const { return states_[s].noepsilons; }

  static ConstFstImpl<A, U> *Read(std::istream &strm,
                                  const FstReadOptions &opts);

  const A *Arcs(StateId s) const { return arcs_ + states_[s].pos; }

  // Provide information needed for generic state iterator
  void InitStateIterator(StateIteratorData<A> *data) const {
    data->base = nullptr;
    data->nstates = nstates_;
  }

  // Provide information needed for the generic arc iterator
  void InitArcIterator(StateId s, ArcIteratorData<A> *data) const {
    data->base = nullptr;
    data->arcs = arcs_ + states_[s].pos;
    data->narcs = states_[s].narcs;
    data->ref_count = nullptr;
  }

 private:
  friend class ConstFst<A, U>;  // Allow finding narcs_, nstates_ during Write

  // States implemented by array *states_ below, arcs by (single) *arcs_.
  struct State {
    Weight final;         // Final weight
    Unsigned pos;         // Start of state's arcs in *arcs_
    Unsigned narcs;       // Number of arcs (per state)
    Unsigned niepsilons;  // # of input epsilons
    Unsigned noepsilons;  // # of output epsilons
    State() : final(Weight::Zero()), niepsilons(0), noepsilons(0) {}
  };

  // Properties always true of this Fst class
  static const uint64 kStaticProperties = kExpanded;
  // Current unaligned file format version. The unaligned version was added and
  // made the default since the aligned version does not work on pipes.
  static const int kFileVersion = 2;
  // Current aligned file format version
  static const int kAlignedFileVersion = 1;
  // Minimum file format version supported
  static const int kMinFileVersion = 1;

  std::unique_ptr<MappedFile> states_region_;  // Mapped file for states
  std::unique_ptr<MappedFile> arcs_region_;    // Mapped file for arcs
  State *states_;              // States represenation
  A *arcs_;                    // Arcs representation
  StateId nstates_;            // Number of states
  size_t narcs_;               // Number of arcs (per FST)
  StateId start_;              // Initial state

  ConstFstImpl(const ConstFstImpl &) = delete;
  ConstFstImpl &operator=(const ConstFstImpl &) = delete;
};

template <class A, class U>
const uint64 ConstFstImpl<A, U>::kStaticProperties;
template <class A, class U>
const int ConstFstImpl<A, U>::kFileVersion;
template <class A, class U>
const int ConstFstImpl<A, U>::kAlignedFileVersion;
template <class A, class U>
const int ConstFstImpl<A, U>::kMinFileVersion;

template <class A, class U>
ConstFstImpl<A, U>::ConstFstImpl(const Fst<A> &fst) : nstates_(0), narcs_(0) {
  string type = "const";
  if (sizeof(U) != sizeof(uint32)) {
    string size;
    Int64ToStr(sizeof(U) * 8, &size);
    type += size;
  }
  SetType(type);
  SetInputSymbols(fst.InputSymbols());
  SetOutputSymbols(fst.OutputSymbols());
  start_ = fst.Start();

  // Count # of states and arcs.
  for (StateIterator<Fst<A>> siter(fst); !siter.Done(); siter.Next()) {
    ++nstates_;
    StateId s = siter.Value();
    for (ArcIterator<Fst<A>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      ++narcs_;
    }
  }
  states_region_.reset(MappedFile::Allocate(nstates_ * sizeof(*states_)));
  arcs_region_.reset(MappedFile::Allocate(narcs_ * sizeof(*arcs_)));
  states_ = reinterpret_cast<State *>(states_region_->mutable_data());
  arcs_ = reinterpret_cast<A *>(arcs_region_->mutable_data());
  size_t pos = 0;
  for (StateId s = 0; s < nstates_; ++s) {
    states_[s].final = fst.Final(s);
    states_[s].pos = pos;
    states_[s].narcs = 0;
    states_[s].niepsilons = 0;
    states_[s].noepsilons = 0;
    for (ArcIterator<Fst<A>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const A &arc = aiter.Value();
      ++states_[s].narcs;
      if (arc.ilabel == 0) ++states_[s].niepsilons;
      if (arc.olabel == 0) ++states_[s].noepsilons;
      arcs_[pos++] = arc;
    }
  }

  uint64 props = fst.Properties(kMutable, false) ?
      fst.Properties(kCopyProperties, true) :
      CheckProperties(fst,
                      kCopyProperties & ~kWeightedCycles & ~kUnweightedCycles,
                      kCopyProperties);
  SetProperties(props | kStaticProperties);
}

template <class A, class U>
ConstFstImpl<A, U> *ConstFstImpl<A, U>::Read(std::istream &strm,
                                             const FstReadOptions &opts) {
  std::unique_ptr<ConstFstImpl<A, U>> impl(new ConstFstImpl<A, U>());
  FstHeader hdr;
  if (!impl->ReadHeader(strm, opts, kMinFileVersion, &hdr)) {
    return nullptr;
  }
  impl->start_ = hdr.Start();
  impl->nstates_ = hdr.NumStates();
  impl->narcs_ = hdr.NumArcs();

  // Ensures compatibility
  if (hdr.Version() == kAlignedFileVersion) {
    hdr.SetFlags(hdr.GetFlags() | FstHeader::IS_ALIGNED);
  }

  if ((hdr.GetFlags() & FstHeader::IS_ALIGNED) && !AlignInput(strm)) {
    LOG(ERROR) << "ConstFst::Read: Alignment failed: " << opts.source;
    return nullptr;
  }

  size_t b = impl->nstates_ * sizeof(typename ConstFstImpl<A, U>::State);
  impl->states_region_.reset(
      MappedFile::Map(&strm, opts.mode == FstReadOptions::MAP, opts.source, b));
  if (!strm || !impl->states_region_) {
    LOG(ERROR) << "ConstFst::Read: Read failed: " << opts.source;
    return nullptr;
  }
  impl->states_ =
      reinterpret_cast<State *>(impl->states_region_->mutable_data());
  if ((hdr.GetFlags() & FstHeader::IS_ALIGNED) && !AlignInput(strm)) {
    LOG(ERROR) << "ConstFst::Read: Alignment failed: " << opts.source;
    return nullptr;
  }

  b = impl->narcs_ * sizeof(A);
  impl->arcs_region_.reset(
      MappedFile::Map(&strm, opts.mode == FstReadOptions::MAP, opts.source, b));
  if (!strm || !impl->arcs_region_) {
    LOG(ERROR) << "ConstFst::Read: Read failed: " << opts.source;
    return nullptr;
  }
  impl->arcs_ = reinterpret_cast<A *>(impl->arcs_region_->mutable_data());
  return impl.release();
}

// Simple concrete immutable FST.  This class attaches interface to
// implementation and handles reference counting, delegating most
// methods to ImplToExpandedFst. The unsigned type U is used to
// represent indices into the arc array (default declared
// in fst-decl.h).
template <class A, class U /* = uint32 */>
class ConstFst : public ImplToExpandedFst<ConstFstImpl<A, U>> {
 public:
  friend class StateIterator<ConstFst<A, U>>;
  friend class ArcIterator<ConstFst<A, U>>;
  template <class F, class G>
  void friend Cast(const F &, G *);

  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef ConstFstImpl<A, U> Impl;
  typedef U Unsigned;

  ConstFst() : ImplToExpandedFst<Impl>(std::make_shared<Impl>()) {}

  explicit ConstFst(const Fst<A> &fst)
      : ImplToExpandedFst<Impl>(std::make_shared<Impl>(fst)) {}

  ConstFst(const ConstFst<A, U> &fst, bool safe = false)
      : ImplToExpandedFst<Impl>(fst) {}

  // Get a copy of this ConstFst. See Fst<>::Copy() for further doc.
  ConstFst<A, U> *Copy(bool safe = false) const override {
    return new ConstFst<A, U>(*this, safe);
  }

  // Read a ConstFst from an input stream; return nullptr on error
  static ConstFst<A, U> *Read(std::istream &strm, const FstReadOptions &opts) {
    Impl *impl = Impl::Read(strm, opts);
    return impl ? new ConstFst<A, U>(std::shared_ptr<Impl>(impl)) : nullptr;
  }

  // Read a ConstFst from a file; return nullptr on error
  // Empty filename reads from standard input
  static ConstFst<A, U> *Read(const string &filename) {
    Impl *impl = ImplToExpandedFst<Impl>::Read(filename);
    return impl ? new ConstFst<A, U>(std::shared_ptr<Impl>(impl)) : nullptr;
  }

  bool Write(std::ostream &strm, const FstWriteOptions &opts) const override {
    return WriteFst(*this, strm, opts);
  }

  bool Write(const string &filename) const override {
    return Fst<A>::WriteFile(filename);
  }

  template <class F>
  static bool WriteFst(const F &fst, std::ostream &strm,
                       const FstWriteOptions &opts);

  void InitStateIterator(StateIteratorData<Arc> *data) const override {
    GetImpl()->InitStateIterator(data);
  }

  void InitArcIterator(StateId s, ArcIteratorData<Arc> *data) const override {
    GetImpl()->InitArcIterator(s, data);
  }

 private:
  explicit ConstFst(std::shared_ptr<Impl> impl)
      : ImplToExpandedFst<Impl>(impl) {}

  using ImplToFst<Impl, ExpandedFst<A>>::GetImpl;

  // Use overloading to extract the type of the argument.
  static const Impl *GetImplIfConstFst(const ConstFst &const_fst) {
    return const_fst.GetImpl();
  }

  // Note that this does not give privileged treatment to subtypes of ConstFst.
  template <typename NonConstFst>
  static Impl *GetImplIfConstFst(const NonConstFst &fst) {
    return nullptr;
  }

  ConstFst &operator=(const ConstFst &fst) = delete;
};

// Writes Fst in Const format, potentially with a pass over the machine
// before writing to compute number of states and arcs.
//
template <class A, class U>
template <class F>
bool ConstFst<A, U>::WriteFst(const F &fst, std::ostream &strm,
                              const FstWriteOptions &opts) {
  int file_version = opts.align ? ConstFstImpl<A, U>::kAlignedFileVersion
                                : ConstFstImpl<A, U>::kFileVersion;
  size_t num_arcs = -1, num_states = -1;
  size_t start_offset = 0;
  bool update_header = true;
  if (const Impl *impl = GetImplIfConstFst(fst)) {
    num_arcs = impl->narcs_;
    num_states = impl->nstates_;
    update_header = false;
  } else if (opts.stream_write || (start_offset = strm.tellp()) == -1) {
    // precompute values needed for header when we cannot seek to rewrite it.
    num_arcs = 0;
    num_states = 0;
    for (StateIterator<F> siter(fst); !siter.Done(); siter.Next()) {
      num_arcs += fst.NumArcs(siter.Value());
      ++num_states;
    }
    update_header = false;
  }
  FstHeader hdr;
  hdr.SetStart(fst.Start());
  hdr.SetNumStates(num_states);
  hdr.SetNumArcs(num_arcs);
  string type = "const";
  if (sizeof(U) != sizeof(uint32)) {
    string size;
    Int64ToStr(8 * sizeof(U), &size);
    type += size;
  }
  uint64 properties = fst.Properties(kCopyProperties, true) |
                      ConstFstImpl<A, U>::kStaticProperties;
  FstImpl<A>::WriteFstHeader(fst, strm, opts, file_version, type, properties,
                             &hdr);
  if (opts.align && !AlignOutput(strm)) {
    LOG(ERROR) << "Could not align file during write after header";
    return false;
  }
  size_t pos = 0, states = 0;
  typename ConstFstImpl<A, U>::State state;
  for (StateIterator<F> siter(fst); !siter.Done(); siter.Next()) {
    state.final = fst.Final(siter.Value());
    state.pos = pos;
    state.narcs = fst.NumArcs(siter.Value());
    state.niepsilons = fst.NumInputEpsilons(siter.Value());
    state.noepsilons = fst.NumOutputEpsilons(siter.Value());
    strm.write(reinterpret_cast<const char *>(&state), sizeof(state));
    pos += state.narcs;
    ++states;
  }
  hdr.SetNumStates(states);
  hdr.SetNumArcs(pos);
  if (opts.align && !AlignOutput(strm)) {
    LOG(ERROR) << "Could not align file during write after writing states";
  }
  for (StateIterator<F> siter(fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    for (ArcIterator<F> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const A &arc = aiter.Value();
      strm.write(reinterpret_cast<const char *>(&arc), sizeof(arc));
    }
  }
  strm.flush();
  if (!strm) {
    LOG(ERROR) << "ConstFst::WriteFst: write failed: " << opts.source;
    return false;
  }
  if (update_header) {
    return FstImpl<A>::UpdateFstHeader(fst, strm, opts, file_version, type,
                                       properties, &hdr, start_offset);
  } else {
    if (hdr.NumStates() != num_states) {
      LOG(ERROR) << "Inconsistent number of states observed during write";
      return false;
    }
    if (hdr.NumArcs() != num_arcs) {
      LOG(ERROR) << "Inconsistent number of arcs observed during write";
      return false;
    }
  }
  return true;
}

// Specialization for ConstFst; see generic version in fst.h
// for sample usage (but use the ConstFst type!). This version
// should inline.
template <class A, class U>
class StateIterator<ConstFst<A, U>> {
 public:
  typedef typename A::StateId StateId;

  explicit StateIterator(const ConstFst<A, U> &fst)
      : nstates_(fst.GetImpl()->NumStates()), s_(0) {}

  bool Done() const { return s_ >= nstates_; }

  StateId Value() const { return s_; }

  void Next() { ++s_; }

  void Reset() { s_ = 0; }

 private:
  StateId nstates_;
  StateId s_;
};

// Specialization for ConstFst; see generic version in fst.h
// for sample usage (but use the ConstFst type!). This version
// should inline.
template <class A, class U>
class ArcIterator<ConstFst<A, U>> {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const ConstFst<A, U> &fst, StateId s)
      : arcs_(fst.GetImpl()->Arcs(s)),
        narcs_(fst.GetImpl()->NumArcs(s)),
        i_(0) {}

  bool Done() const { return i_ >= narcs_; }

  const A &Value() const { return arcs_[i_]; }

  void Next() { ++i_; }

  size_t Position() const { return i_; }

  void Reset() { i_ = 0; }

  void Seek(size_t a) { i_ = a; }

  uint32 Flags() const { return kArcValueFlags; }

  void SetFlags(uint32 f, uint32 m) {}

 private:
  const A *arcs_;
  size_t narcs_;
  size_t i_;
};

// A useful alias when using StdArc.
typedef ConstFst<StdArc> StdConstFst;

}  // namespace fst

#endif  // FST_LIB_CONST_FST_H_
