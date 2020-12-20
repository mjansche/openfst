// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Expands an MPDT to an FST.

#ifndef FST_EXTENSIONS_MPDT_EXPAND_H__
#define FST_EXTENSIONS_MPDT_EXPAND_H__

#include <vector>

#include <fst/extensions/mpdt/mpdt.h>
#include <fst/extensions/pdt/paren.h>
#include <fst/cache.h>
#include <fst/mutable-fst.h>
#include <fst/queue.h>
#include <fst/state-table.h>
#include <fst/test-properties.h>

namespace fst {

template <class Arc>
struct MPdtExpandFstOptions : public CacheOptions {
  bool keep_parentheses;
  MPdtStack<typename Arc::StateId, typename Arc::Label> *stack;
  PdtStateTable<typename Arc::StateId, typename Arc::StateId> *state_table;

  MPdtExpandFstOptions(
      const CacheOptions &opts = CacheOptions(), bool kp = false,
      MPdtStack<typename Arc::StateId, typename Arc::Label> *s = nullptr,
      PdtStateTable<typename Arc::StateId, typename Arc::StateId> *st = nullptr)
      : CacheOptions(opts), keep_parentheses(kp), stack(s), state_table(st) {}
};

// Properties for an expanded PDT.
inline uint64 MPdtExpandProperties(uint64 inprops) {
  return inprops & (kAcceptor | kAcyclic | kInitialAcyclic | kUnweighted);
}

// Implementation class for ExpandFst
template <class A>
class MPdtExpandFstImpl : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;

  using CacheBaseImpl<CacheState<A>>::PushArc;
  using CacheBaseImpl<CacheState<A>>::HasArcs;
  using CacheBaseImpl<CacheState<A>>::HasFinal;
  using CacheBaseImpl<CacheState<A>>::HasStart;
  using CacheBaseImpl<CacheState<A>>::SetArcs;
  using CacheBaseImpl<CacheState<A>>::SetFinal;
  using CacheBaseImpl<CacheState<A>>::SetStart;

  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef StateId StackId;
  typedef PdtStateTuple<StateId, StackId> StateTuple;

  MPdtExpandFstImpl(
      const Fst<A> &fst,
      const std::vector<std::pair<typename Arc::Label, typename Arc::Label>>
          &parens,
      const std::vector<typename Arc::Label> &assignments,
      const MPdtExpandFstOptions<A> &opts)
      : CacheImpl<A>(opts),
        fst_(fst.Copy()),
        stack_(opts.stack ? opts.stack
                          : new MPdtStack<StateId, Label>(parens, assignments)),
        state_table_(opts.state_table ? opts.state_table
                                      : new PdtStateTable<StateId, StackId>()),
        own_stack_(opts.stack == 0),
        own_state_table_(opts.state_table == 0),
        keep_parentheses_(opts.keep_parentheses) {
    SetType("expand");

    uint64 props = fst.Properties(kFstProperties, false);
    SetProperties(MPdtExpandProperties(props), kCopyProperties);

    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
  }

  MPdtExpandFstImpl(const MPdtExpandFstImpl &impl)
      : CacheImpl<A>(impl),
        fst_(impl.fst_->Copy(true)),
        stack_(new MPdtStack<StateId, Label>(*impl.stack_)),
        state_table_(new PdtStateTable<StateId, StackId>()),
        own_stack_(true),
        own_state_table_(true),
        keep_parentheses_(impl.keep_parentheses_) {
    SetType("expand");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  ~MPdtExpandFstImpl() override {
    delete fst_;
    if (own_stack_) delete stack_;
    if (own_state_table_) delete state_table_;
  }

  StateId Start() {
    if (!HasStart()) {
      StateId s = fst_->Start();
      if (s == kNoStateId) return kNoStateId;
      StateTuple tuple(s, 0);
      StateId start = state_table_->FindState(tuple);
      SetStart(start);
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      const StateTuple &tuple = state_table_->Tuple(s);
      Weight w = fst_->Final(tuple.state_id);
      if (w != Weight::Zero() && tuple.stack_id == 0)
        SetFinal(s, w);
      else
        SetFinal(s, Weight::Zero());
    }
    return CacheImpl<A>::Final(s);
  }

  size_t NumArcs(StateId s) {
    if (!HasArcs(s)) {
      ExpandState(s);
    }
    return CacheImpl<A>::NumArcs(s);
  }

  size_t NumInputEpsilons(StateId s) {
    if (!HasArcs(s)) ExpandState(s);
    return CacheImpl<A>::NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(StateId s) {
    if (!HasArcs(s)) ExpandState(s);
    return CacheImpl<A>::NumOutputEpsilons(s);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    if (!HasArcs(s)) ExpandState(s);
    CacheImpl<A>::InitArcIterator(s, data);
  }

  // Computes the outgoing transitions from a state, creating new destination
  // states as needed.
  void ExpandState(StateId s) {
    StateTuple tuple = state_table_->Tuple(s);
    for (ArcIterator<Fst<A>> aiter(*fst_, tuple.state_id); !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      StackId stack_id = stack_->Find(tuple.stack_id, arc.ilabel);
      if (stack_id == -1) {
        // Non-matching close parenthesis
        continue;
      } else if ((stack_id != tuple.stack_id) && !keep_parentheses_) {
        // Stack push/pop
        arc.ilabel = arc.olabel = 0;
      }
      StateTuple ntuple(arc.nextstate, stack_id);
      arc.nextstate = state_table_->FindState(ntuple);
      PushArc(s, arc);
    }
    SetArcs(s);
  }

  const MPdtStack<StackId, Label> &GetStack() const { return *stack_; }

  const PdtStateTable<StateId, StackId> &GetStateTable() const {
    return *state_table_;
  }

 private:
  const Fst<A> *fst_;

  MPdtStack<StackId, Label> *stack_;
  PdtStateTable<StateId, StackId> *state_table_;
  bool own_stack_;
  bool own_state_table_;
  bool keep_parentheses_;

  void operator=(const MPdtExpandFstImpl<A> &);  // disallow
};

// Expands a multi-pushdown transducer (MPDT) encoded as an FST into an FST.
// This version is a delayed Fst. In the MPDT, some transitions are labeled with
// open or close parentheses. To be interpreted as an MPDT, the parens for each
// stack must balance on a path. The open-close parenthesis label pair sets are
// passed in 'parens', and the assignment of those pairs to stacks in
// 'paren_assignments'. The expansion enforces the parenthesis constraints. The
// MPDT must be expandable as an FST.
//
// This class attaches interface to implementation and handles
// reference counting, delegating most methods to ImplToFst.
template <class A>
class MPdtExpandFst : public ImplToFst<MPdtExpandFstImpl<A>> {
 public:
  friend class ArcIterator<MPdtExpandFst<A>>;
  friend class StateIterator<MPdtExpandFst<A>>;

  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef StateId StackId;
  typedef DefaultCacheStore<A> Store;
  typedef typename Store::State State;
  typedef MPdtExpandFstImpl<A> Impl;

  MPdtExpandFst(const Fst<A> &fst,
                const std::vector<
                std::pair<typename Arc::Label, typename Arc::Label>> &parens,
                const std::vector<typename Arc::Label> &assignments)
      : ImplToFst<Impl>(std::make_shared<Impl>(fst, parens, assignments,
                                               MPdtExpandFstOptions<A>())) {}

  MPdtExpandFst(const Fst<A> &fst,
                const std::vector<
                std::pair<typename Arc::Label, typename Arc::Label>> &parens,
                const std::vector<typename Arc::Label> &assignments,
                const MPdtExpandFstOptions<A> &opts)
      : ImplToFst<Impl>(
            std::make_shared<Impl>(fst, parens, assignments, opts)) {}

  // See Fst<>::Copy() for doc.
  MPdtExpandFst(const MPdtExpandFst<A> &fst, bool safe = false)
      : ImplToFst<Impl>(fst, safe) {}

  // Get a copy of this ExpandFst. See Fst<>::Copy() for further doc.
  MPdtExpandFst<A> *Copy(bool safe = false) const override {
    return new MPdtExpandFst<A>(*this, safe);
  }

  inline void InitStateIterator(StateIteratorData<A> *data) const override;

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) const override {
    GetMutableImpl()->InitArcIterator(s, data);
  }

  const MPdtStack<StackId, Label> &GetStack() const {
    return GetImpl()->GetStack();
  }

  const PdtStateTable<StateId, StackId> &GetStateTable() const {
    return GetImpl()->GetStateTable();
  }

 private:
  using ImplToFst<Impl>::GetImpl;
  using ImplToFst<Impl>::GetMutableImpl;

  void operator=(const MPdtExpandFst<A> &fst);  // Disallow
};

// Specialization for ExpandFst.
template <class A>
class StateIterator<MPdtExpandFst<A>> :
    public CacheStateIterator<MPdtExpandFst<A>> {
 public:
  explicit StateIterator(const MPdtExpandFst<A> &fst)
      : CacheStateIterator<MPdtExpandFst<A>>(fst, fst.GetMutableImpl()) {}
};

// Specialization for ExpandFst.
template <class A>
class ArcIterator<MPdtExpandFst<A>>
    : public CacheArcIterator<MPdtExpandFst<A>> {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const MPdtExpandFst<A> &fst, StateId s)
      : CacheArcIterator<MPdtExpandFst<A>>(fst.GetMutableImpl(), s) {
    if (!fst.GetImpl()->HasArcs(s)) fst.GetMutableImpl()->ExpandState(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};

template <class A>
inline void MPdtExpandFst<A>::InitStateIterator(StateIteratorData<A> *data)
    const {
  data->base = new StateIterator<MPdtExpandFst<A>>(*this);
}

//
// Expand() Functions
//

struct MPdtExpandOptions {
  bool connect;
  bool keep_parentheses;

  MPdtExpandOptions(bool c = true, bool k = false)
      : connect(c), keep_parentheses(k) {}
};

// Expands a multi-pushdown transducer (MPDT) encoded as an FST into an FST.
// This version writes the expanded PDT result to a MutableFst.  In the MPDT,
// some transitions are labeled with open or close parentheses. To be
// interpreted as an MPDT, the parens for each stack must balance on a path. The
// open-close parenthesis label pair sets are passed in 'parens', and the
// assignment of those pairs to stacks in 'paren_assignments'. The expansion
// enforces the parenthesis constraints. The MPDT must be expandable as an FST.
template <class Arc>
void Expand(const Fst<Arc> &ifst,
            const std::vector<
            std::pair<typename Arc::Label, typename Arc::Label>> &parens,
            const std::vector<typename Arc::Label> &assignments,
            MutableFst<Arc> *ofst, const MPdtExpandOptions &opts) {
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename MPdtExpandFst<Arc>::StackId StackId;
  MPdtExpandFstOptions<Arc> eopts;
  eopts.gc_limit = 0;
  eopts.keep_parentheses = opts.keep_parentheses;
  *ofst = MPdtExpandFst<Arc>(ifst, parens, assignments, eopts);
  if (opts.connect) Connect(ofst);
}

// Expands a multi-pushdown transducer (MPDT) encoded as an FST into an FST.
// This version writes the expanded PDT result to a MutableFst.  In the MPDT,
// some transitions are labeled with open or close parentheses. To be
// interpreted as an MPDT, the parens for each stack must balance on a path. The
// open-close parenthesis label pair sets are passed in 'parens', and the
// assignment of those pairs to stacks in 'paren_assignments'. The expansion
// enforces the parenthesis constraints. The MPDT must be expandable as an FST.
template <class Arc>
void Expand(const Fst<Arc> &ifst,
            const std::vector<std::pair<typename Arc::Label,
            typename Arc::Label>> &parens,
            const std::vector<typename Arc::Label> &assignments,
            MutableFst<Arc> *ofst, bool connect = true,
            bool keep_parentheses = false) {
  Expand(ifst, parens, assignments, ofst,
         MPdtExpandOptions(connect, keep_parentheses));
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_EXPAND_H__
