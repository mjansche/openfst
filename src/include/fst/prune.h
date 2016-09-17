// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Functions implementing pruning.

#ifndef FST_LIB_PRUNE_H_
#define FST_LIB_PRUNE_H_

#include <utility>
#include <vector>

#include <fst/arcfilter.h>
#include <fst/heap.h>
#include <fst/shortest-distance.h>


namespace fst {

template <class A, class ArcFilter>
class PruneOptions {
 public:
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  // Pruning weight threshold.
  Weight weight_threshold;
  // Pruning state threshold.
  StateId state_threshold;
  // Arc filter.
  ArcFilter filter;
  // If non-zero, passes in pre-computed shortest distance to final states.
  const std::vector<Weight> *distance;
  // Determines the degree of convergence required when computing shortest
  // distances.
  float delta;

  explicit PruneOptions(const Weight &w, StateId s, ArcFilter f,
                        std::vector<Weight> *d = nullptr, float e = kDelta)
      : weight_threshold(w),
        state_threshold(s),
        filter(std::move(f)),
        distance(d),
        delta(e) {}
};

template <class S, class W>
class PruneCompare {
 public:
  typedef S StateId;
  typedef W Weight;

  PruneCompare(const std::vector<Weight> &idistance,
               const std::vector<Weight> &fdistance)
      : idistance_(idistance), fdistance_(fdistance) {}

  bool operator()(const StateId x, const StateId y) const {
    Weight wx = Times(x < idistance_.size() ? idistance_[x] : Weight::Zero(),
                      x < fdistance_.size() ? fdistance_[x] : Weight::Zero());
    Weight wy = Times(y < idistance_.size() ? idistance_[y] : Weight::Zero(),
                      y < fdistance_.size() ? fdistance_[y] : Weight::Zero());
    return less_(wx, wy);
  }

 private:
  const std::vector<Weight> &idistance_;
  const std::vector<Weight> &fdistance_;
  NaturalLess<Weight> less_;
};

// Pruning algorithm: this version modifies its input and it takes an
// options class as an argment. Delete states and arcs in 'fst' that
// do not belong to a successful path whose weight is no more than
// the weight of the shortest path Times() 'opts.weight_threshold'.
// When 'opts.state_threshold != kNoStateId', the resulting transducer
// will restricted further to have at most 'opts.state_threshold'
// states. Weights need to be commutative and have the path
// property. The weight 'w' of any cycle needs to be bounded, i.e.,
// 'Plus(w, W::One()) = One()'.
template <class Arc, class ArcFilter>
void Prune(MutableFst<Arc> *fst, const PruneOptions<Arc, ArcFilter> &opts) {
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;

  if ((Weight::Properties() & (kPath | kCommutative)) !=
      (kPath | kCommutative)) {
    FSTERROR() << "Prune: Weight needs to have the path property and"
               << " be commutative: " << Weight::Type();
    fst->SetProperties(kError, kError);
    return;
  }
  StateId ns = fst->NumStates();
  if (ns == 0) return;
  std::vector<Weight> idistance(ns, Weight::Zero());
  std::vector<Weight> tmp;
  if (!opts.distance) {
    tmp.reserve(ns);
    ShortestDistance(*fst, &tmp, true, opts.delta);
  }
  const std::vector<Weight> *fdistance = opts.distance ? opts.distance : &tmp;

  if ((opts.state_threshold == 0) || (fdistance->size() <= fst->Start()) ||
      ((*fdistance)[fst->Start()] == Weight::Zero())) {
    fst->DeleteStates();
    return;
  }
  PruneCompare<StateId, Weight> compare(idistance, *fdistance);
  using StateHeap = Heap<StateId, PruneCompare<StateId, Weight>>;
  StateHeap heap(compare);
  std::vector<bool> visited(ns, false);
  std::vector<size_t> enqueued(ns, StateHeap::kNoKey);
  std::vector<StateId> dead;
  dead.push_back(fst->AddState());
  NaturalLess<Weight> less;
  Weight limit = Times((*fdistance)[fst->Start()], opts.weight_threshold);

  StateId num_visited = 0;
  StateId s = fst->Start();
  if (!less(limit, (*fdistance)[s])) {
    idistance[s] = Weight::One();
    enqueued[s] = heap.Insert(s);
    ++num_visited;
  }

  while (!heap.Empty()) {
    s = heap.Top();
    heap.Pop();
    enqueued[s] = StateHeap::kNoKey;
    visited[s] = true;
    if (less(limit, Times(idistance[s], fst->Final(s)))) {
      fst->SetFinal(s, Weight::Zero());
    }
    for (MutableArcIterator<MutableFst<Arc>> ait(fst, s); !ait.Done();
         ait.Next()) {
      Arc arc = ait.Value();
      if (!opts.filter(arc)) continue;
      Weight weight =
          Times(Times(idistance[s], arc.weight),
                arc.nextstate < fdistance->size() ? (*fdistance)[arc.nextstate]
                                                  : Weight::Zero());
      if (less(limit, weight)) {
        arc.nextstate = dead[0];
        ait.SetValue(arc);
        continue;
      }
      if (less(Times(idistance[s], arc.weight), idistance[arc.nextstate])) {
        idistance[arc.nextstate] = Times(idistance[s], arc.weight);
      }
      if (visited[arc.nextstate]) continue;
      if ((opts.state_threshold != kNoStateId) &&
          (num_visited >= opts.state_threshold)) {
        continue;
      }
      if (enqueued[arc.nextstate] == StateHeap::kNoKey) {
        enqueued[arc.nextstate] = heap.Insert(arc.nextstate);
        ++num_visited;
      } else {
        heap.Update(enqueued[arc.nextstate], arc.nextstate);
      }
    }
  }
  for (size_t i = 0; i < visited.size(); ++i) {
    if (!visited[i]) dead.push_back(i);
  }
  fst->DeleteStates(dead);
}

// Pruning algorithm: this version modifies its input and simply takes
// the pruning threshold as an argument. Delete states and arcs in
// 'fst' that do not belong to a successful path whose weight is no
// more than the weight of the shortest path Times()
// 'weight_threshold'.  When 'state_threshold != kNoStateId', the
// resulting transducer will be restricted further to have at most
// 'opts.state_threshold' states. Weights need to be commutative and
// have the path property. The weight 'w' of any cycle needs to be
// bounded, i.e., 'Plus(w, W::One()) = One()'.
template <class Arc>
void Prune(MutableFst<Arc> *fst, typename Arc::Weight weight_threshold,
           typename Arc::StateId state_threshold = kNoStateId,
           double delta = kDelta) {
  PruneOptions<Arc, AnyArcFilter<Arc>> opts(
      weight_threshold, state_threshold, AnyArcFilter<Arc>(), nullptr, delta);
  Prune(fst, opts);
}

// Pruning algorithm: this version writes the pruned input Fst to an
// output MutableFst and it takes an options class as an argument.
// 'ofst' contains states and arcs that belong to a successful path in
// 'ifst' whose weight is no more than the weight of the shortest path
// Times() 'opts.weight_threshold'. When 'opts.state_threshold !=
// kNoStateId', 'ofst' will be restricted further to have at most
// 'opts.state_threshold' states. Weights need to be commutative and
// have the path property. The weight 'w' of any cycle needs to be
// bounded, i.e., 'Plus(w, W::One()) = One()'.
template <class Arc, class ArcFilter>
void Prune(const Fst<Arc> &ifst, MutableFst<Arc> *ofst,
           const PruneOptions<Arc, ArcFilter> &opts) {
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;

  if ((Weight::Properties() & (kPath | kCommutative)) !=
      (kPath | kCommutative)) {
    FSTERROR() << "Prune: Weight needs to have the path property and"
               << " be commutative: " << Weight::Type();
    ofst->SetProperties(kError, kError);
    return;
  }
  ofst->DeleteStates();
  ofst->SetInputSymbols(ifst.InputSymbols());
  ofst->SetOutputSymbols(ifst.OutputSymbols());
  if (ifst.Start() == kNoStateId) return;
  NaturalLess<Weight> less;
  if (less(opts.weight_threshold, Weight::One()) ||
      (opts.state_threshold == 0)) {
    return;
  }
  std::vector<Weight> idistance;
  std::vector<Weight> tmp;
  if (!opts.distance) ShortestDistance(ifst, &tmp, true, opts.delta);
  const std::vector<Weight> *fdistance = opts.distance ? opts.distance : &tmp;

  if ((fdistance->size() <= ifst.Start()) ||
      ((*fdistance)[ifst.Start()] == Weight::Zero())) {
    return;
  }
  PruneCompare<StateId, Weight> compare(idistance, *fdistance);
  using StateHeap = Heap<StateId, PruneCompare<StateId, Weight>>;
  StateHeap heap(compare);
  std::vector<StateId> copy;
  std::vector<size_t> enqueued;
  std::vector<bool> visited;

  StateId s = ifst.Start();
  Weight limit = Times(s < fdistance->size() ? (*fdistance)[s] : Weight::Zero(),
                       opts.weight_threshold);
  while (copy.size() <= s) copy.push_back(kNoStateId);
  copy[s] = ofst->AddState();
  ofst->SetStart(copy[s]);
  while (idistance.size() <= s) idistance.push_back(Weight::Zero());
  idistance[s] = Weight::One();
  while (enqueued.size() <= s) {
    enqueued.push_back(StateHeap::kNoKey);
    visited.push_back(false);
  }
  enqueued[s] = heap.Insert(s);

  while (!heap.Empty()) {
    s = heap.Top();
    heap.Pop();
    enqueued[s] = StateHeap::kNoKey;
    visited[s] = true;
    if (!less(limit, Times(idistance[s], ifst.Final(s)))) {
      ofst->SetFinal(copy[s], ifst.Final(s));
    }
    for (ArcIterator<Fst<Arc>> ait(ifst, s); !ait.Done(); ait.Next()) {
      const Arc &arc = ait.Value();
      if (!opts.filter(arc)) continue;
      Weight weight =
          Times(Times(idistance[s], arc.weight),
                arc.nextstate < fdistance->size() ? (*fdistance)[arc.nextstate]
                                                  : Weight::Zero());
      if (less(limit, weight)) continue;
      if ((opts.state_threshold != kNoStateId) &&
          (ofst->NumStates() >= opts.state_threshold)) {
        continue;
      }
      while (idistance.size() <= arc.nextstate) {
        idistance.push_back(Weight::Zero());
      }
      if (less(Times(idistance[s], arc.weight), idistance[arc.nextstate])) {
        idistance[arc.nextstate] = Times(idistance[s], arc.weight);
      }
      while (copy.size() <= arc.nextstate) copy.push_back(kNoStateId);
      if (copy[arc.nextstate] == kNoStateId) {
        copy[arc.nextstate] = ofst->AddState();
      }
      ofst->AddArc(copy[s], Arc(arc.ilabel, arc.olabel, arc.weight,
                                copy[arc.nextstate]));
      while (enqueued.size() <= arc.nextstate) {
        enqueued.push_back(StateHeap::kNoKey);
        visited.push_back(false);
      }
      if (visited[arc.nextstate]) continue;
      if (enqueued[arc.nextstate] == StateHeap::kNoKey) {
        enqueued[arc.nextstate] = heap.Insert(arc.nextstate);
      } else {
        heap.Update(enqueued[arc.nextstate], arc.nextstate);
      }
    }
  }
}

// Pruning algorithm: this version writes the pruned input Fst to an
// output MutableFst and simply takes the pruning threshold as an
// argument.  'ofst' contains states and arcs that belong to a
// successful path in 'ifst' whose weight is no more than
// the weight of the shortest path Times() 'weight_threshold'. When
// 'state_threshold != kNoStateId', 'ofst' will be restricted further
// to have at most 'opts.state_threshold' states. Weights need to be
// commutative and have the path property. The weight 'w' of any cycle
// needs to be bounded, i.e., 'Plus(w, W::One()) = W::One()'.
template <class Arc>
void Prune(const Fst<Arc> &ifst, MutableFst<Arc> *ofst,
           typename Arc::Weight weight_threshold,
           typename Arc::StateId state_threshold = kNoStateId,
           float delta = kDelta) {
  PruneOptions<Arc, AnyArcFilter<Arc>> opts(
      weight_threshold, state_threshold, AnyArcFilter<Arc>(), nullptr, delta);
  Prune(ifst, ofst, opts);
}

}  // namespace fst

#endif  // FST_LIB_PRUNE_H_
