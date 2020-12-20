// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Functions and classes to relabel an FST (either on input or output).

#ifndef FST_LIB_RELABEL_H_
#define FST_LIB_RELABEL_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fst/cache.h>
#include <fst/test-properties.h>


#include <unordered_map>

namespace fst {

//
// Relabels either the input labels or output labels. The old to
// new labels are specified using a vector of std::pair<Label,Label>.
// Any label associations not specified are assumed to be identity
// mapping. The destination labels must be valid labels (e.g. no kNoLabel).
//
// \param fst input fst, must be mutable
// \param ipairs vector of input label pairs indicating old to new mapping
// \param opairs vector of output label pairs indicating old to new mapping
//
template <class A>
void Relabel(
    MutableFst<A>* fst,
    const std::vector<std::pair<typename A::Label, typename A::Label>>& ipairs,
    const std::vector<std::pair<typename A::Label, typename A::Label>>&
        opairs) {
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;

  uint64 props = fst->Properties(kFstProperties, false);

  // construct label to label hash.
  std::unordered_map<Label, Label> input_map;
  for (size_t i = 0; i < ipairs.size(); ++i) {
    input_map[ipairs[i].first] = ipairs[i].second;
  }

  std::unordered_map<Label, Label> output_map;
  for (size_t i = 0; i < opairs.size(); ++i) {
    output_map[opairs[i].first] = opairs[i].second;
  }

  for (StateIterator<MutableFst<A>> siter(*fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    for (MutableArcIterator<MutableFst<A>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      A arc = aiter.Value();

      // relabel input
      // only relabel if relabel pair defined
      auto it = input_map.find(arc.ilabel);
      if (it != input_map.end()) {
        if (it->second == kNoLabel) {
          FSTERROR() << "Input symbol id " << arc.ilabel
                     << " missing from target vocabulary";
          fst->SetProperties(kError, kError);
          return;
        }
        arc.ilabel = it->second;
      }

      // relabel output
      it = output_map.find(arc.olabel);
      if (it != output_map.end()) {
        if (it->second == kNoLabel) {
          FSTERROR() << "Output symbol id " << arc.olabel
                     << " missing from target vocabulary";
          fst->SetProperties(kError, kError);
          return;
        }
        arc.olabel = it->second;
      }

      aiter.SetValue(arc);
    }
  }

  fst->SetProperties(RelabelProperties(props), kFstProperties);
}

//
// Relabels either the input labels or output labels. The old to
// new labels mappings are specified using an input Symbol set.
// Any label associations not specified are assumed to be identity
// mapping.
//
// \param fst input fst, must be mutable
// \param new_isymbols symbol set indicating new mapping of input symbols.
//        Must contain (at least) all input symbols in the fst.
// \param new_osymbols symbol set indicating new mapping of output symbols
//        Must contain (at least) all output symbols in the fst.
template <class A>
void Relabel(MutableFst<A>* fst, const SymbolTable* new_isymbols,
             const SymbolTable* new_osymbols) {
  Relabel(fst, fst->InputSymbols(), new_isymbols, true,  // Attach new isymbols?
          fst->OutputSymbols(), new_osymbols, true);     // Attach new osymbols?
}

// new_isymbols must contain (at least) all symbols in the fst.
// new_osymbols must contain (at least) all symbols in the fst.
template <class A>
void Relabel(MutableFst<A>* fst, const SymbolTable* old_isymbols,
             const SymbolTable* new_isymbols, bool attach_new_isymbols,
             const SymbolTable* old_osymbols, const SymbolTable* new_osymbols,
             bool attach_new_osymbols) {
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;

  std::vector<std::pair<Label, Label>> ipairs;
  if (old_isymbols && new_isymbols) {
    size_t num_missing_syms = 0;
    for (SymbolTableIterator syms_iter(*old_isymbols); !syms_iter.Done();
         syms_iter.Next()) {
      const string isymbol = syms_iter.Symbol();
      const int isymbol_val = syms_iter.Value();
      const int new_isymbol_val = new_isymbols->Find(isymbol);
      if (new_isymbol_val == kNoLabel) {
        VLOG(1) << "Input symbol id " << isymbol_val << " symbol '" << isymbol
                << "' missing from target symbol table.";
        ++num_missing_syms;
      }
      ipairs.push_back(std::make_pair(isymbol_val, new_isymbol_val));
    }
    if (num_missing_syms > 0) {
      LOG(WARNING) << "Target symbol table missing: " << num_missing_syms
                   << " input symbols.";
    }
    if (attach_new_isymbols) fst->SetInputSymbols(new_isymbols);
  }

  std::vector<std::pair<Label, Label>> opairs;
  if (old_osymbols && new_osymbols) {
    size_t num_missing_syms = 0;
    for (SymbolTableIterator syms_iter(*old_osymbols); !syms_iter.Done();
         syms_iter.Next()) {
      const string osymbol = syms_iter.Symbol();
      const int osymbol_val = syms_iter.Value();
      const int new_osymbol_val = new_osymbols->Find(osymbol);
      if (new_osymbol_val == kNoLabel) {
        VLOG(1) << "Output symbol id " << osymbol_val << " symbol '" << osymbol
                << "' missing from target symbol table.";
        ++num_missing_syms;
      }
      opairs.push_back(std::make_pair(osymbol_val, new_osymbol_val));
    }
    if (num_missing_syms > 0) {
      LOG(WARNING) << "Target symbol table missing: " << num_missing_syms
                   << " output symbols.";
    }
    if (attach_new_osymbols) fst->SetOutputSymbols(new_osymbols);
  }

  // call relabel using vector of relabel pairs.
  Relabel(fst, ipairs, opairs);
}

typedef CacheOptions RelabelFstOptions;

template <class A>
class RelabelFst;

//
// \class RelabelFstImpl
// \brief Implementation for delayed relabeling
//
// Relabels an FST from one symbol set to another. Relabeling
// can either be on input or output space. RelabelFst implements
// a delayed version of the relabel. Arcs are relabeled on the fly
// and not cached. I.e each request is recomputed.
//
template <class A>
class RelabelFstImpl : public CacheImpl<A> {
  friend class StateIterator<RelabelFst<A>>;

 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::WriteHeader;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;

  using CacheImpl<A>::PushArc;
  using CacheImpl<A>::HasArcs;
  using CacheImpl<A>::HasFinal;
  using CacheImpl<A>::HasStart;
  using CacheImpl<A>::SetArcs;
  using CacheImpl<A>::SetFinal;
  using CacheImpl<A>::SetStart;

  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef DefaultCacheStore<A> Store;
  typedef typename Store::State State;

  RelabelFstImpl(const Fst<A>& fst,
                 const std::vector<std::pair<Label, Label>>& ipairs,
                 const std::vector<std::pair<Label, Label>>& opairs,
                 const RelabelFstOptions& opts)
      : CacheImpl<A>(opts),
        fst_(fst.Copy()),
        relabel_input_(false),
        relabel_output_(false) {
    uint64 props = fst.Properties(kCopyProperties, false);
    SetProperties(RelabelProperties(props));
    SetType("relabel");

    // create input label map
    if (!ipairs.empty()) {
      for (size_t i = 0; i < ipairs.size(); ++i) {
        input_map_[ipairs[i].first] = ipairs[i].second;
      }
      relabel_input_ = true;
    }

    // create output label map
    if (!opairs.empty()) {
      for (size_t i = 0; i < opairs.size(); ++i) {
        output_map_[opairs[i].first] = opairs[i].second;
      }
      relabel_output_ = true;
    }
  }

  RelabelFstImpl(const Fst<A>& fst, const SymbolTable* old_isymbols,
                 const SymbolTable* new_isymbols,
                 const SymbolTable* old_osymbols,
                 const SymbolTable* new_osymbols, const RelabelFstOptions& opts)
      : CacheImpl<A>(opts),
        fst_(fst.Copy()),
        relabel_input_(false),
        relabel_output_(false) {
    SetType("relabel");

    uint64 props = fst.Properties(kCopyProperties, false);
    SetProperties(RelabelProperties(props));
    SetInputSymbols(old_isymbols);
    SetOutputSymbols(old_osymbols);

    if (old_isymbols && new_isymbols &&
        old_isymbols->LabeledCheckSum() != new_isymbols->LabeledCheckSum()) {
      for (SymbolTableIterator syms_iter(*old_isymbols); !syms_iter.Done();
           syms_iter.Next()) {
        input_map_[syms_iter.Value()] = new_isymbols->Find(syms_iter.Symbol());
      }
      SetInputSymbols(new_isymbols);
      relabel_input_ = true;
    }

    if (old_osymbols && new_osymbols &&
        old_osymbols->LabeledCheckSum() != new_osymbols->LabeledCheckSum()) {
      for (SymbolTableIterator syms_iter(*old_osymbols); !syms_iter.Done();
           syms_iter.Next()) {
        output_map_[syms_iter.Value()] = new_osymbols->Find(syms_iter.Symbol());
      }
      SetOutputSymbols(new_osymbols);
      relabel_output_ = true;
    }
  }

  RelabelFstImpl(const RelabelFstImpl<A>& impl)
      : CacheImpl<A>(impl),
        fst_(impl.fst_->Copy(true)),
        input_map_(impl.input_map_),
        output_map_(impl.output_map_),
        relabel_input_(impl.relabel_input_),
        relabel_output_(impl.relabel_output_) {
    SetType("relabel");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  StateId Start() {
    if (!HasStart()) {
      StateId s = fst_->Start();
      SetStart(s);
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      SetFinal(s, fst_->Final(s));
    }
    return CacheImpl<A>::Final(s);
  }

  size_t NumArcs(StateId s) {
    if (!HasArcs(s)) {
      Expand(s);
    }
    return CacheImpl<A>::NumArcs(s);
  }

  size_t NumInputEpsilons(StateId s) {
    if (!HasArcs(s)) {
      Expand(s);
    }
    return CacheImpl<A>::NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(StateId s) {
    if (!HasArcs(s)) {
      Expand(s);
    }
    return CacheImpl<A>::NumOutputEpsilons(s);
  }

  uint64 Properties() const override { return Properties(kFstProperties); }

  // Set error if found; return FST impl properties.
  uint64 Properties(uint64 mask) const override {
    if ((mask & kError) && fst_->Properties(kError, false)) {
      SetProperties(kError, kError);
    }
    return FstImpl<Arc>::Properties(mask);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A>* data) {
    if (!HasArcs(s)) {
      Expand(s);
    }
    CacheImpl<A>::InitArcIterator(s, data);
  }

  void Expand(StateId s) {
    for (ArcIterator<Fst<A>> aiter(*fst_, s); !aiter.Done(); aiter.Next()) {
      A arc = aiter.Value();

      // relabel input
      if (relabel_input_) {
        auto it = input_map_.find(arc.ilabel);
        if (it != input_map_.end()) {
          arc.ilabel = it->second;
        }
      }

      // relabel output
      if (relabel_output_) {
        auto it = output_map_.find(arc.olabel);
        if (it != output_map_.end()) {
          arc.olabel = it->second;
        }
      }

      PushArc(s, arc);
    }
    SetArcs(s);
  }

 private:
  std::unique_ptr<const Fst<A>> fst_;

  std::unordered_map<Label, Label> input_map_;
  std::unordered_map<Label, Label> output_map_;
  bool relabel_input_;
  bool relabel_output_;
};

//
// \class RelabelFst
// \brief Delayed implementation of arc relabeling
//
// This class attaches interface to implementation and handles
// reference counting, delegating most methods to ImplToFst.
template <class A>
class RelabelFst : public ImplToFst<RelabelFstImpl<A>> {
 public:
  friend class ArcIterator<RelabelFst<A>>;
  friend class StateIterator<RelabelFst<A>>;

  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef DefaultCacheStore<A> Store;
  typedef typename Store::State State;
  typedef RelabelFstImpl<A> Impl;

  RelabelFst(const Fst<A>& fst,
             const std::vector<std::pair<Label, Label>>& ipairs,
             const std::vector<std::pair<Label, Label>>& opairs)
      : ImplToFst<Impl>(
            std::make_shared<Impl>(fst, ipairs, opairs, RelabelFstOptions())) {}

  RelabelFst(const Fst<A>& fst,
             const std::vector<std::pair<Label, Label>>& ipairs,
             const std::vector<std::pair<Label, Label>>& opairs,
             const RelabelFstOptions& opts)
      : ImplToFst<Impl>(std::make_shared<Impl>(fst, ipairs, opairs, opts)) {}

  RelabelFst(const Fst<A>& fst, const SymbolTable* new_isymbols,
             const SymbolTable* new_osymbols)
      : ImplToFst<Impl>(std::make_shared<Impl>(
            fst, fst.InputSymbols(), new_isymbols, fst.OutputSymbols(),
            new_osymbols, RelabelFstOptions())) {}

  RelabelFst(const Fst<A>& fst, const SymbolTable* new_isymbols,
             const SymbolTable* new_osymbols, const RelabelFstOptions& opts)
      : ImplToFst<Impl>(
            std::make_shared<Impl>(fst, fst.InputSymbols(), new_isymbols,
                                   fst.OutputSymbols(), new_osymbols, opts)) {}

  RelabelFst(const Fst<A>& fst, const SymbolTable* old_isymbols,
             const SymbolTable* new_isymbols, const SymbolTable* old_osymbols,
             const SymbolTable* new_osymbols)
      : ImplToFst<Impl>(std::make_shared<Impl>(fst, old_isymbols, new_isymbols,
                                               old_osymbols, new_osymbols,
                                               RelabelFstOptions())) {}

  RelabelFst(const Fst<A>& fst, const SymbolTable* old_isymbols,
             const SymbolTable* new_isymbols, const SymbolTable* old_osymbols,
             const SymbolTable* new_osymbols, const RelabelFstOptions& opts)
      : ImplToFst<Impl>(std::make_shared<Impl>(fst, old_isymbols, new_isymbols,
                                               old_osymbols, new_osymbols,
                                               opts)) {}

  // See Fst<>::Copy() for doc.
  RelabelFst(const RelabelFst<A>& fst, bool safe = false)
      : ImplToFst<Impl>(fst, safe) {}

  // Get a copy of this RelabelFst. See Fst<>::Copy() for further doc.
  RelabelFst<A>* Copy(bool safe = false) const override {
    return new RelabelFst<A>(*this, safe);
  }

  void InitStateIterator(StateIteratorData<A>* data) const override;

  void InitArcIterator(StateId s, ArcIteratorData<A>* data) const override {
    return GetMutableImpl()->InitArcIterator(s, data);
  }

 private:
  using ImplToFst<Impl>::GetImpl;
  using ImplToFst<Impl>::GetMutableImpl;

  RelabelFst& operator=(const RelabelFst& fst) = delete;
};

// Specialization for RelabelFst.
template <class A>
class StateIterator<RelabelFst<A>> : public StateIteratorBase<A> {
 public:
  typedef typename A::StateId StateId;

  explicit StateIterator(const RelabelFst<A>& fst)
      : impl_(fst.GetImpl()), siter_(*impl_->fst_), s_(0) {}

  bool Done() const { return siter_.Done(); }

  StateId Value() const { return s_; }

  void Next() {
    if (!siter_.Done()) {
      ++s_;
      siter_.Next();
    }
  }

  void Reset() {
    s_ = 0;
    siter_.Reset();
  }

 private:
  bool Done_() const override { return Done(); }
  StateId Value_() const override { return Value(); }
  void Next_() override { Next(); }
  void Reset_() override { Reset(); }

  const RelabelFstImpl<A>* impl_;
  StateIterator<Fst<A>> siter_;
  StateId s_;

  StateIterator(const StateIterator&) = delete;
  StateIterator& operator=(const StateIterator&) = delete;
};

// Specialization for RelabelFst.
template <class A>
class ArcIterator<RelabelFst<A>> : public CacheArcIterator<RelabelFst<A>> {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const RelabelFst<A>& fst, StateId s)
      : CacheArcIterator<RelabelFst<A>>(fst.GetImpl(), s) {
    if (!fst.GetImpl()->HasArcs(s)) fst.GetImpl()->Expand(s);
  }
};

template <class A>
inline void RelabelFst<A>::InitStateIterator(StateIteratorData<A>* data) const {
  data->base = new StateIterator<RelabelFst<A>>(*this);
}

// Useful alias when using StdArc.
typedef RelabelFst<StdArc> StdRelabelFst;

}  // namespace fst

#endif  // FST_LIB_RELABEL_H_
