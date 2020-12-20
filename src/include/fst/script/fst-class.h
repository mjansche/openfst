// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_FST_CLASS_H_
#define FST_SCRIPT_FST_CLASS_H_

#include <algorithm>
#include <limits>
#include <string>
#include <type_traits>

#include <fst/expanded-fst.h>
#include <fst/fst.h>
#include <fst/mutable-fst.h>
#include <fst/vector-fst.h>
#include <fst/script/arc-class.h>
#include <fst/script/weight-class.h>

// Classes to support "boxing" all existing types of FST arcs in a single
// FstClass which hides the arc types. This allows clients to load
// and work with FSTs without knowing the arc type. These classes are only
// recommended for use in high-level scripting applications. Most users should
// use the lower-level templated versions corresponding to these classes.

namespace fst {
namespace script {

// Abstract base class defining the set of functionalities implemented in all
// impls and passed through by all bases. Below FstClassBase the class
// hierarchy bifurcates; FstClassImplBase serves as the base class for all
// implementations (of which FstClassImpl is currently the only one) and
// FstClass serves as the base class for all interfaces.

class FstClassBase {
 public:
  virtual const string &ArcType() const = 0;
  virtual WeightClass Final(int64) const = 0;
  virtual const string &FstType() const = 0;
  virtual const SymbolTable *InputSymbols() const = 0;
  virtual size_t NumArcs(int64) const = 0;
  virtual size_t NumInputEpsilons(int64) const = 0;
  virtual size_t NumOutputEpsilons(int64) const = 0;
  virtual const SymbolTable *OutputSymbols() const = 0;
  virtual uint64 Properties(uint64, bool) const = 0;
  virtual int64 Start() const = 0;
  virtual const string &WeightType() const = 0;
  virtual bool ValidStateId(int64) const = 0;
  virtual bool Write(const string &fname) const = 0;
  virtual bool Write(std::ostream &, const FstWriteOptions &) const = 0;
  virtual ~FstClassBase() {}

 protected:
};

// Adds all the MutableFst methods.
class FstClassImplBase : public FstClassBase {
 public:
  virtual bool AddArc(int64, const ArcClass &) = 0;
  virtual int64 AddState() = 0;
  virtual FstClassImplBase *Copy() = 0;
  virtual bool DeleteArcs(int64, size_t) = 0;
  virtual bool DeleteArcs(int64) = 0;
  virtual bool DeleteStates(const std::vector<int64> &) = 0;
  virtual void DeleteStates() = 0;
  virtual SymbolTable *MutableInputSymbols() = 0;
  virtual SymbolTable *MutableOutputSymbols() = 0;
  virtual int64 NumStates() const = 0;
  virtual bool ReserveArcs(int64, size_t) = 0;
  virtual void ReserveStates(int64) = 0;
  virtual void SetInputSymbols(SymbolTable *) = 0;
  virtual bool SetFinal(int64, const WeightClass &) = 0;
  virtual void SetOutputSymbols(SymbolTable *) = 0;
  virtual void SetProperties(uint64, uint64) = 0;
  virtual bool SetStart(int64) = 0;
  ~FstClassImplBase() override {}
};

// Containiner class wrapping an Fst<Arc>, hiding its arc type. Whether this
// Fst<Arc> pointer refers to a special kind of FST (e.g. a MutableFst) is
// known by the type of interface class that owns the pointer to this
// container.

template <class Arc>
class FstClassImpl : public FstClassImplBase {
 public:
  explicit FstClassImpl(Fst<Arc> *impl, bool should_own = false)
      : impl_(should_own ? impl : impl->Copy()) {}

  explicit FstClassImpl(const Fst<Arc> &impl) : impl_(impl.Copy()) {}

  // Warning: calling this method casts the FST to a mutable FST.
  bool AddArc(int64 s, const ArcClass &a) override {
    if (!ValidStateId(s)) return false;
    // Note that we do not check that the destination state is valid, so users
    // can add arcs before they add the corresponding states. Verify can be
    // used to determine whether any arc has a nonexisting destination.
    Arc arc(a.ilabel, a.olabel, *(a.weight.GetWeight<typename Arc::Weight>()),
            a.nextstate);
    static_cast<MutableFst<Arc> *>(impl_)->AddArc(s, arc);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  int64 AddState() override {
    return static_cast<MutableFst<Arc> *>(impl_)->AddState();
  }

  const string &ArcType() const override { return Arc::Type(); }

  FstClassImpl *Copy() override { return new FstClassImpl<Arc>(impl_); }

  // Warning: calling this method casts the FST to a mutable FST.
  bool DeleteArcs(int64 s, size_t n) override {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_)->DeleteArcs(s, n);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool DeleteArcs(int64 s) override {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_)->DeleteArcs(s);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool DeleteStates(const std::vector<int64> &dstates) override {
    for (auto it = dstates.cbegin(); it != dstates.cend(); ++it) {
      if (!ValidStateId(*it)) return false;
    }
    // Warning: calling this method with any integers beyond the precision of
    // the underlying FST will result in truncation.
    std::vector<typename Arc::StateId> typed_dstates(dstates.size());
    std::copy(dstates.cbegin(), dstates.cend(), typed_dstates.begin());
    static_cast<MutableFst<Arc> *>(impl_)->DeleteStates(typed_dstates);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void DeleteStates() override {
    static_cast<MutableFst<Arc> *>(impl_)->DeleteStates();
  }

  WeightClass Final(int64 s) const override {
    if (!ValidStateId(s)) return WeightClass::NoWeight(WeightType());
    WeightClass w(impl_->Final(s));
    return w;
  }

  const string &FstType() const override { return impl_->Type(); }

  const SymbolTable *InputSymbols() const override {
    return impl_->InputSymbols();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  SymbolTable *MutableInputSymbols() override {
    return static_cast<MutableFst<Arc> *>(impl_)->MutableInputSymbols();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  SymbolTable *MutableOutputSymbols() override {
    return static_cast<MutableFst<Arc> *>(impl_)->MutableOutputSymbols();
  }

  // Signals failure by returning size_t max.
  size_t NumArcs(int64 s) const override {
    return ValidStateId(s) ? impl_->NumArcs(s)
                           : std::numeric_limits<size_t>::max();
  }

  // Signals failure by returning size_t max.
  size_t NumInputEpsilons(int64 s) const override {
    return ValidStateId(s) ? impl_->NumInputEpsilons(s)
                           : std::numeric_limits<size_t>::max();
  }

  // Signals failure by returning size_t max.
  size_t NumOutputEpsilons(int64 s) const override {
    return ValidStateId(s) ? impl_->NumOutputEpsilons(s)
                           : std::numeric_limits<size_t>::max();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  int64 NumStates() const override {
    return static_cast<MutableFst<Arc> *>(impl_)->NumStates();
  }

  uint64 Properties(uint64 mask, bool test) const override {
    return impl_->Properties(mask, test);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool ReserveArcs(int64 s, size_t n) override {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_)->ReserveArcs(s, n);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void ReserveStates(int64 s) override {
    static_cast<MutableFst<Arc> *>(impl_)->ReserveStates(s);
  }

  const SymbolTable *OutputSymbols() const override {
    return impl_->OutputSymbols();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void SetInputSymbols(SymbolTable *is) override {
    static_cast<MutableFst<Arc> *>(impl_)->SetInputSymbols(is);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool SetFinal(int64 s, const WeightClass &w) override {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_)
        ->SetFinal(s, *(w.GetWeight<typename Arc::Weight>()));
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void SetOutputSymbols(SymbolTable *os) override {
    static_cast<MutableFst<Arc> *>(impl_)->SetOutputSymbols(os);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void SetProperties(uint64 props, uint64 mask) override {
    static_cast<MutableFst<Arc> *>(impl_)->SetProperties(props, mask);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool SetStart(int64 s) override {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_)->SetStart(s);
    return true;
  }

  int64 Start() const override { return impl_->Start(); }

  bool ValidStateId(int64 s) const override {
    // This cowardly refuses to count states if the FST is not yet expanded.
    if (!Properties(kExpanded, true)) {
      FSTERROR() << "Cannot get number of states for unexpanded FST";
      return false;
    }
    // If the FST is already expanded, CountStates calls NumStates.
    if (s < 0 || s >= CountStates(*impl_)) {
      FSTERROR() << "State ID " << s << " not valid.";
      return false;
    }
    return true;
  }

  const string &WeightType() const override { return Arc::Weight::Type(); }

  bool Write(const string &fname) const override { return impl_->Write(fname); }

  bool Write(std::ostream &ostr, const FstWriteOptions &opts) const override {
    return impl_->Write(ostr, opts);
  }

  ~FstClassImpl() override { delete impl_; }

  Fst<Arc> *GetImpl() const { return impl_; }

 private:
  Fst<Arc> *impl_;
};

// BASE CLASS DEFINITIONS

class MutableFstClass;

class FstClass : public FstClassBase {
 public:
  FstClass() : impl_(nullptr) {}

  template <class Arc>
  explicit FstClass(const Fst<Arc> &fst) : impl_(new FstClassImpl<Arc>(fst)) {}

  FstClass(const FstClass &other)
      : impl_(other.impl_ == nullptr ? nullptr : other.impl_->Copy()) {}

  FstClass &operator=(const FstClass &other) {
    delete impl_;
    impl_ = other.impl_ == nullptr ? nullptr : other.impl_->Copy();
    return *this;
  }

  WeightClass Final(int64 s) const override { return impl_->Final(s); }

  const string &ArcType() const override { return impl_->ArcType(); }

  const string &FstType() const override { return impl_->FstType(); }

  const SymbolTable *InputSymbols() const override {
    return impl_->InputSymbols();
  }

  size_t NumArcs(int64 s) const override { return impl_->NumArcs(s); }

  size_t NumInputEpsilons(int64 s) const override {
    return impl_->NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(int64 s) const override {
    return impl_->NumOutputEpsilons(s);
  }

  const SymbolTable *OutputSymbols() const override {
    return impl_->OutputSymbols();
  }

  uint64 Properties(uint64 mask, bool test) const override {
    // Special handling for FSTs with a null impl.
    if (!impl_) return kError & mask;
    return impl_->Properties(mask, test);
  }

  static FstClass *Read(const string &fname);

  static FstClass *Read(std::istream &istr, const string &source);

  int64 Start() const override { return impl_->Start(); }

  bool ValidStateId(int64 s) const override { return impl_->ValidStateId(s); }

  const string &WeightType() const override { return impl_->WeightType(); }

  // Helper that logs an ERROR if the weight type of an FST and a WeightClass
  // don't match.

  bool WeightTypesMatch(const WeightClass &w, const string &op_name) const;

  bool Write(const string &fname) const override { return impl_->Write(fname); }

  bool Write(std::ostream &ostr, const FstWriteOptions &opts) const override {
    return impl_->Write(ostr, opts);
  }

  ~FstClass() override { delete impl_; }

  // These methods are required by IO registration.

  template <class Arc>
  static FstClassImplBase *Convert(const FstClass &other) {
    FSTERROR() << "Doesn't make sense to convert any class to type FstClass.";
    return nullptr;
  }

  template <class Arc>
  static FstClassImplBase *Create() {
    FSTERROR() << "Doesn't make sense to create an FstClass with a "
               << "particular arc type.";
    return nullptr;
  }

  template <class Arc>
  const Fst<Arc> *GetFst() const {
    if (Arc::Type() != ArcType()) {
      return nullptr;
    } else {
      FstClassImpl<Arc> *typed_impl = static_cast<FstClassImpl<Arc> *>(impl_);
      return typed_impl->GetImpl();
    }
  }

  template <class Arc>
  static FstClass *Read(std::istream &stream, const FstReadOptions &opts) {
    if (!opts.header) {
      LOG(ERROR) << "FstClass::Read: Options header not specified";
      return nullptr;
    }
    const FstHeader &hdr = *opts.header;
    if (hdr.Properties() & kMutable) {
      return ReadTypedFst<MutableFstClass, MutableFst<Arc>>(stream, opts);
    } else {
      return ReadTypedFst<FstClass, Fst<Arc>>(stream, opts);
    }
  }

 protected:
  explicit FstClass(FstClassImplBase *impl) : impl_(impl) {}

  FstClassImplBase *GetImpl() const { return impl_; }

  FstClassImplBase *GetImpl() { return impl_; }

  // Generic template method for reading an arc-templated FST of type
  // UnderlyingT, and returning it wrapped as FstClassT, with appropriate
  // error checking. Called from arc-templated Read() static methods.
  template <class FstClassT, class UnderlyingT>
  static FstClassT *ReadTypedFst(std::istream &stream,
                                 const FstReadOptions &opts) {
    UnderlyingT *u = UnderlyingT::Read(stream, opts);
    if (!u) {
      return nullptr;
    } else {
      FstClassT *r = new FstClassT(*u);
      delete u;
      return r;
    }
  }

 private:
  FstClassImplBase *impl_;
};

// Specific types of FstClass with special properties

class MutableFstClass : public FstClass {
 public:
  bool AddArc(int64 s, const ArcClass &a) {
    if (!WeightTypesMatch(a.weight, "AddArc")) return false;
    return GetImpl()->AddArc(s, a);
  }

  int64 AddState() { return GetImpl()->AddState(); }

  bool DeleteArcs(int64 s, size_t n) { return GetImpl()->DeleteArcs(s, n); }

  bool DeleteArcs(int64 s) { return GetImpl()->DeleteArcs(s); }

  bool DeleteStates(const std::vector<int64> &dstates) {
    return GetImpl()->DeleteStates(dstates);
  }

  void DeleteStates() { GetImpl()->DeleteStates(); }

  SymbolTable *MutableInputSymbols() {
    return GetImpl()->MutableInputSymbols();
  }

  SymbolTable *MutableOutputSymbols() {
    return GetImpl()->MutableOutputSymbols();
  }

  int64 NumStates() const { return GetImpl()->NumStates(); }

  bool ReserveArcs(int64 s, size_t n) { return GetImpl()->ReserveArcs(s, n); }

  void ReserveStates(int64 s) { GetImpl()->ReserveStates(s); }

  static MutableFstClass *Read(const string &fname, bool convert = false);

  void SetInputSymbols(SymbolTable *is) { GetImpl()->SetInputSymbols(is); }

  bool SetFinal(int64 s, const WeightClass &w) {
    if (!WeightTypesMatch(w, "SetFinal")) return false;
    return GetImpl()->SetFinal(s, w);
  }

  void SetOutputSymbols(SymbolTable *os) { GetImpl()->SetOutputSymbols(os); }

  void SetProperties(uint64 props, uint64 mask) {
    GetImpl()->SetProperties(props, mask);
  }

  bool SetStart(int64 s) { return GetImpl()->SetStart(s); }

  bool ValidStateId(int64 s) const override {
    return GetImpl()->ValidStateId(s);
  }

  bool Write(std::ostream &ostr, const FstWriteOptions &opts) const override {
    return GetImpl()->Write(ostr, opts);
  }

  bool Write(const string &fname) const override {
    return GetImpl()->Write(fname);
  }

  template <class Arc>
  explicit MutableFstClass(const MutableFst<Arc> &fst) : FstClass(fst) {}

  // These methods are required by IO registration.

  template <class Arc>
  static FstClassImplBase *Convert(const FstClass &other) {
    FSTERROR() << "Doesn't make sense to convert any class to type "
               << "MutableFstClass.";
    return nullptr;
  }

  template <class Arc>
  static FstClassImplBase *Create() {
    FSTERROR() << "Doesn't make sense to create a MutableFstClass with a "
               << "particular arc type.";
    return nullptr;
  }

  template <class Arc>
  MutableFst<Arc> *GetMutableFst() {
    Fst<Arc> *fst = const_cast<Fst<Arc> *>(this->GetFst<Arc>());
    MutableFst<Arc> *mfst = static_cast<MutableFst<Arc> *>(fst);

    return mfst;
  }

  template <class Arc>
  static MutableFstClass *Read(std::istream &stream,
                               const FstReadOptions &opts) {
    MutableFst<Arc> *mfst = MutableFst<Arc>::Read(stream, opts);
    if (!mfst) {
      return nullptr;
    } else {
      MutableFstClass *retval = new MutableFstClass(*mfst);
      delete mfst;
      return retval;
    }
  }

 protected:
  explicit MutableFstClass(FstClassImplBase *impl) : FstClass(impl) {}
};

class VectorFstClass : public MutableFstClass {
 public:
  explicit VectorFstClass(FstClassImplBase *impl) : MutableFstClass(impl) {}

  explicit VectorFstClass(const FstClass &other);

  explicit VectorFstClass(const string &arc_type);

  static VectorFstClass *Read(const string &fname);

  template <class Arc>
  static VectorFstClass *Read(std::istream &stream,
                              const FstReadOptions &opts) {
    VectorFst<Arc> *vfst = VectorFst<Arc>::Read(stream, opts);
    if (!vfst) {
      return nullptr;
    } else {
      VectorFstClass *retval = new VectorFstClass(*vfst);
      delete vfst;
      return retval;
    }
  }

  template <class Arc>
  explicit VectorFstClass(const VectorFst<Arc> &fst) : MutableFstClass(fst) {}

  template <class Arc>
  static FstClassImplBase *Convert(const FstClass &other) {
    return new FstClassImpl<Arc>(new VectorFst<Arc>(*other.GetFst<Arc>()),
                                 true);
  }

  template <class Arc>
  static FstClassImplBase *Create() {
    return new FstClassImpl<Arc>(new VectorFst<Arc>(), true);
  }
};

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_FST_CLASS_H_
