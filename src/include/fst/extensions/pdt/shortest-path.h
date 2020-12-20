// shortest-path.h

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
// Functions to find shortest paths in a PDT.

#ifndef FST_EXTENSIONS_PDT_SHORTEST_PATH_H__
#define FST_EXTENSIONS_PDT_SHORTEST_PATH_H__

#include <fst/shortest-path.h>

#include <tr1/unordered_map>
using std::tr1::unordered_map;
using std::tr1::unordered_multimap;
#include <stack>
#include <vector>
using std::vector;

namespace fst {

// Class to store PDT shortest path results.
template <class Arc>
class PdtShortestPathData {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  struct SearchState {
    SearchState() : state(kNoStateId), start(kNoStateId) {}

    SearchState(StateId s, StateId t) : state(s), start(t) {}

    bool operator==(const SearchState &s) const {
      if (&s == this)
        return true;
      return s.state == this->state && s.start == this->start;
    }

    StateId state;  // PDT state
    StateId start;  // PDT paren 'source' state
  };

  struct SearchData {
    SearchData() : distance(Weight::Zero()),
                   flags(0),
                   parent(kNoStateId, kNoStateId),
                   arc_parent(kNoArc) {}

    Weight distance;
    uint8 flags;
    SearchState parent;
    Arc arc_parent;
  };

  PdtShortestPathData(int nstates)
      : search_map_(nstates),
        last_state_(kNoStateId, kNoStateId) {}

  void Clear() {
    search_map_.clear();
    last_state_ = SearchState(kNoStateId, kNoStateId);
  }

  Weight Distance(SearchState s) const {
    SearchData *data = GetSearchData(s);
    return data->distance;
  }

  uint8 Flags(SearchState s) const {
    SearchData *data = GetSearchData(s);
    return data->flags;
  }

  SearchState Parent(SearchState s) const {
    SearchData *data = GetSearchData(s);
    return data->parent;
  }

  const Arc &ArcParent(SearchState s) const {
    SearchData *data = GetSearchData(s);
    return data->arc_parent;
  }

  void SetDistance(SearchState s, Weight w) {
    SearchData *data = GetSearchData(s);
    data->distance = w;
  }

  void SetFlags(SearchState s, uint8 f, uint8 mask) {
    SearchData *data = GetSearchData(s);
    data->flags &= ~mask;
    data->flags |= f & mask;
  }

  void SetParent(SearchState s, SearchState p) {
    SearchData *data = GetSearchData(s);
    data->parent = p;
  }

  void SetArcParent(SearchState s, const Arc &a) {
    SearchData *data = GetSearchData(s);
    data->arc_parent = a;
  }

  size_t Size() const { return search_map_.size(); }

 private:
  static const Arc kNoArc;
  static const size_t kPrime0;

  // Hash for search state
  struct SearchStateHash {
    size_t operator()(const SearchState &s) const {
      return s.state + s.start * kPrime0;
    }
  };

  typedef unordered_map<SearchState, SearchData, SearchStateHash> SearchMap;

  SearchData *GetSearchData(SearchState s) const {
    if (s == last_state_)
      return last_data_;
    last_state_ = s;
    return last_data_ = &search_map_[s];
  }

  mutable SearchMap search_map_;
  mutable SearchState last_state_;     // Last state accessed
  mutable SearchData *last_data_;      // Last data accessed

  DISALLOW_COPY_AND_ASSIGN(PdtShortestPathData);
};

template<class Arc> const Arc PdtShortestPathData<Arc>::kNoArc
    = Arc(kNoLabel, kNoLabel, Weight::Zero(), kNoStateId);

template<class Arc> const size_t PdtShortestPathData<Arc>::kPrime0 = 7853;


// This computes the single source shortest (balanced) path (SSSP)
// through a weighted PDT that is strongly regular (i.e. is expandable
// as an FST). It is a generalization of the classic SSSP graph
// algorithm that removes a state s from a user-provided queue and
// relaxes the destination states of transitions leaving s. In this
// PDT version, states that have entering open parentheses are treated
// as source states for a sub-graph SSSP problem with the shortest
// path up to the open parenthesis being first saved. When a close
// parenthesis is then encountered any balancing open parenthesis is
// examined for this saved information and multiplied back. In this
// way, each sub-graph is entered only once rather than repeatedly.
// If every state in the input PDT has the property that there is a
// unique 'start' state for it with entering open parentheses, then
// this algorithm is quite straight-forward. In general, this will not
// be the case, so the algorithm (implicitly) creates a new graph
// where each state is a pair of an original state and a possible
// parenthesis 'start' state for that state.
template<class Arc, class Queue, class ArcFilter>
class PdtShortestPath {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;
  typedef typename PdtShortestPathData<Arc>::SearchState SearchState;

  PdtShortestPath(const Fst<Arc> &ifst,
                  const vector<pair<Label, Label> > &parens,
                  ShortestPathOptions<Arc, Queue, ArcFilter> &opts)
      : ifst_(ifst.Copy()),
        parens_(parens),
        nstates_(CountStates(ifst)),
        state_queue_(opts.state_queue),
        source_(opts.source == kNoStateId ? ifst.Start() : opts.source),
        first_path_(opts.first_path),
        sp_data_(nstates_) {

    if (opts.nshortest != 1)
      LOG(FATAL) << "SingleShortestPath: for nshortest > 1, use ShortestPath"
                 << " instead";
    if (opts.weight_threshold != Weight::Zero() ||
        opts.state_threshold != kNoStateId)
      LOG(FATAL) <<
          "SingleShortestPath: weight and state thresholds not applicable";

    if ((Weight::Properties() & (kPath | kRightSemiring))
        != (kPath | kRightSemiring))
      LOG(FATAL) << "SingleShortestPath: Weight needs to have the path"
                 << " property and be right distributive: " << Weight::Type();


    for (Label i = 0; i < parens.size(); ++i) {
      const pair<Label, Label>  &p = parens[i];
      paren_id_map_[p.first] = i;
      paren_id_map_[p.second] = i;
    }
  };

  ~PdtShortestPath() {
    VLOG(1) << "# of input states: " << nstates_;
    VLOG(1) << "# of output states: " << sp_data_.Size();
    VLOG(1) << "opmm size: " << open_paren_multimap_.size();
    VLOG(1) << "opm size: " << open_paren_map_.size();
    VLOG(1) << "cpmm size: " << close_paren_multimap_.size();
    delete ifst_;
  }

  void ShortestPath(MutableFst<Arc> *ofst) {
    Init(ofst);
    GetDistance(ofst);
    GetPath(ofst);
  }

 private:
  static const Arc kNoArc;
  static const size_t kPrime0;
  static const size_t kPrime1;
  static const uint8 kEnqueued;
  static const uint8 kExpanded;

  // Open-paren shortest-path instance info.
  struct OpenParenData {
    OpenParenData()
        : state(kNoStateId, kNoStateId),
          arc(kNoArc),
          distance(Weight::Zero()) {}

    SearchState state;     // source state of paren
    Arc arc;               // paren transition
    Weight distance;       // shortest distance to open-paren arc.nextstate
  };

  // Close-paren shortest-path instance info.
  struct CloseParenData {
    CloseParenData() : state(kNoStateId, kNoStateId), arc(kNoArc) {}

    CloseParenData(SearchState s, const Arc &a) : state(s), arc(a) {}

    SearchState state;     // source state of paren
    Arc arc;               // paren transition
  };

  // Hash-table key specifying source and dest 'start' states of a paren.
  // These are the 'start' states of the respective sub-graphs.
  struct ParenKey {
    ParenKey()
        : paren_id(kNoLabel), src_start(kNoStateId), dest_start(kNoStateId) {}

    ParenKey(Label id, StateId s, StateId d)
        : paren_id(id), src_start(s), dest_start(d) {}

    Label paren_id;        // Id of paren.
    StateId src_start;     // sub-graph 'start' state for paren source.
    StateId dest_start;    // sub-graph 'start' state for paren dest.

    bool operator==(const ParenKey &x) const {
      if (&x == this)
        return true;
      return x.paren_id == this->paren_id &&
          x.src_start == this->src_start &&
          x.dest_start == this->dest_start;
    }
  };

  // Hash multimap key specifying source or dest 'start' state of a paren.
  // This is the 'start' state of the respective sub-graph.
  struct ParenMultiKey {
    ParenMultiKey() : paren_id(kNoLabel), start(kNoStateId) {}

    ParenMultiKey(Label id, StateId s) : paren_id(id), start(s) {}

    Label paren_id;      // Id of paren.
    StateId start;       // sub-graph 'start' state for the
                         // open (close) paren dest (source).

    bool operator==(const ParenMultiKey &x) const {
      if (&x == this)
        return true;
      return x.paren_id == this->paren_id && x.start == this->start;
    }
  };

  // Hash for paren map
  struct ParenHash {
    size_t operator()(const ParenKey &key) const {
      return key.paren_id + key.src_start * kPrime0 + key.dest_start * kPrime1;
    }
  };

  // Hash for paren multimap
  struct ParenMultiHash {
    size_t operator()(const ParenMultiKey &key) const {
      return key.paren_id + key.start * kPrime0;
    }
  };

  // Hash multimap from open paren multikey to open paren sub-graph
  // 'start' state for paren source
  typedef unordered_multimap<ParenMultiKey, StateId,
                        ParenMultiHash> OpenParenMultiMap;

  // Hash map from open paren key to open paren data
  typedef unordered_map<ParenKey, OpenParenData, ParenHash> OpenParenMap;

  // Hash multimap from close paren multikey to close paren data
  typedef unordered_multimap<ParenMultiKey, CloseParenData,
                        ParenMultiHash> CloseParenMultiMap;

  void Init(MutableFst<Arc> *ofst);
  void GetDistance(MutableFst<Arc> *ofst);
  bool ProcFinal(MutableFst<Arc> *ofst);
  void ProcArcs(MutableFst<Arc> *ofst);
  void ProcOpenParen(Label paren_id, SearchState s, Arc arc, Weight w);
  void ProcCloseParen(Label paren_id, SearchState s, const Arc &arc, Weight w);
  void ProcNonParen(SearchState s, const Arc &arc, Weight w);
  void Relax(SearchState s, SearchState t, Arc arc, Weight w);
  void GetPath(MutableFst<Arc> *ofst);

  Fst<Arc> *ifst_;
  const vector<pair<Label, Label> > &parens_;
  StateId nstates_;
  Queue *state_queue_;
  vector<list<StateId> > start_queue_;
  StateId current_state_;
  list<StateId> current_starts_;
  StateId source_;
  bool first_path_;
  Weight f_distance_;
  SearchState f_parent_;
  PdtShortestPathData<Arc> sp_data_;
  unordered_map<Label, Label> paren_id_map_;
  vector<bool> enqueued_;
  OpenParenMultiMap open_paren_multimap_;
  OpenParenMap open_paren_map_;
  CloseParenMultiMap close_paren_multimap_;

  DISALLOW_COPY_AND_ASSIGN(PdtShortestPath);
};

template<class Arc, class Queue, class ArcFilter>
void PdtShortestPath<Arc, Queue, ArcFilter>::Init(MutableFst<Arc> *ofst) {
  ofst->DeleteStates();
  ofst->SetInputSymbols(ifst_->InputSymbols());
  ofst->SetOutputSymbols(ifst_->OutputSymbols());

  if (ifst_->Start() == kNoStateId)
    return;

  state_queue_->Clear();
  start_queue_.clear();
  start_queue_.resize(nstates_);
  enqueued_.clear();
  enqueued_.resize(nstates_, false);
  f_distance_ = Weight::Zero();
  f_parent_ = SearchState(kNoStateId, kNoStateId);

  open_paren_multimap_.clear();
  open_paren_map_.clear();
  close_paren_multimap_.clear();

  current_state_ = kNoStateId;
  current_starts_.clear();
  state_queue_->Enqueue(source_);
  start_queue_[source_].push_back(source_);
  enqueued_[source_] = true;
  sp_data_.Clear();
  SearchState s(source_, source_);
  sp_data_.SetDistance(s, Weight::One());
  sp_data_.SetFlags(s, kEnqueued, kEnqueued);
}

// Computes the shortest distance stored in a recursive way. Each
// sub-graph (i.e. different paren 'start' state) begins with weight One().
template<class Arc, class Queue, class ArcFilter>
void PdtShortestPath<Arc, Queue, ArcFilter>::GetDistance(
    MutableFst<Arc> *ofst) {
  while (!state_queue_->Empty()) {
    current_state_ = state_queue_->Head();
    state_queue_->Dequeue();
    enqueued_[current_state_] = false;
    swap(current_starts_, start_queue_[current_state_]);
    if (ProcFinal(ofst))
      return;
    ProcArcs(ofst);
    current_starts_.clear();
  }
}

template<class Arc, class Queue, class ArcFilter>
bool PdtShortestPath<Arc, Queue, ArcFilter>::ProcFinal(MutableFst<Arc> *ofst) {
  for(typename list<StateId>::const_iterator lit = current_starts_.begin();
      lit != current_starts_.end(); ++lit) {
    StateId current_start = *lit;
    SearchState s(current_state_, current_start);
    if (ifst_->Final(current_state_) != Weight::Zero() &&
        current_start == source_) {
      Weight w = Times(sp_data_.Distance(s),
                       ifst_->Final(current_state_));
      if (f_distance_ != Plus(f_distance_, w)) {
        f_distance_ = Plus(f_distance_, w);
        f_parent_ = s;
      }
      if (first_path_)
        return true;
    }
  }
  return false;
}

template<class Arc, class Queue, class ArcFilter>
void PdtShortestPath<Arc, Queue, ArcFilter>::ProcArcs(MutableFst<Arc> *ofst) {
  for(typename list<StateId>::const_iterator lit = current_starts_.begin();
      lit != current_starts_.end(); ++lit) {
    StateId current_start = *lit;
    SearchState s(current_state_, current_start);
    sp_data_.SetFlags(s, 0, kEnqueued);

    for (ArcIterator< Fst<Arc> > aiter(*ifst_, current_state_);
         !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      Weight w = Times(sp_data_.Distance(s), arc.weight);

      typename unordered_map<Label, Label>::const_iterator pit
          = paren_id_map_.find(arc.ilabel);
      if (pit != paren_id_map_.end()) {  // Is a paren?
        Label paren_id = pit->second;
        if (arc.ilabel == parens_[paren_id].first)
          ProcOpenParen(paren_id, s, arc, w);
        else
          ProcCloseParen(paren_id, s, arc, w);
      } else {
        ProcNonParen(s, arc, w);
      }
    }
    sp_data_.SetFlags(s, kExpanded, kExpanded);
  }
}

// Saves the shortest path info for reaching this parenthesis
// and starts a new SSSP in the sub-graph pointed to by the parenthesis
// if previously unvisited. Otherwise it finds any previously encountered
// closing parentheses and relaxes them using the recursively stored
// shortest distance to them.
template<class Arc, class Queue, class ArcFilter> inline
void PdtShortestPath<Arc, Queue, ArcFilter>::ProcOpenParen(
    Label paren_id, SearchState s, Arc arc, Weight w) {
  SearchState d(arc.nextstate, arc.nextstate);
  ParenKey key(paren_id, s.start, d.start);
  OpenParenData &opdata = open_paren_map_[key];
  Weight nd = sp_data_.Distance(d);
  if (opdata.distance != Plus(opdata.distance, w)) {
    opdata.state = s;
    opdata.arc = arc;
    opdata.distance = w;
    ParenMultiKey mkey(key.paren_id, key.dest_start);
    open_paren_multimap_.insert(make_pair(mkey, s.start));
    if (nd == Weight::Zero()) {
      if (!enqueued_[d.state]) {
        state_queue_->Enqueue(d.state);
        enqueued_[d.state] = true;
      }
      start_queue_[d.state].push_back(d.state);
      sp_data_.SetFlags(d, kEnqueued, kEnqueued);
      sp_data_.SetDistance(d, Weight::One());
    } else {
      for (typename CloseParenMultiMap::iterator cpit =
               close_paren_multimap_.find(mkey);
           cpit != close_paren_multimap_.end() && cpit->first == mkey;
           ++cpit) {
        CloseParenData &cpdata = cpit->second;
        Weight cpw = Times(w, Times(sp_data_.Distance(cpdata.state),
                                    cpdata.arc.weight));
        Relax(cpdata.state, opdata.state, cpdata.arc, cpw);
      }
    }
  }
}

// Saves the correspondence between each closing parenthesis and its
// balancing open parenthesis info. Relaxes any close parenthesis
// destination state that has a balancing previously encountered open
// parenthesis.
template<class Arc, class Queue, class ArcFilter> inline
void PdtShortestPath<Arc, Queue, ArcFilter>::ProcCloseParen(
    Label paren_id, SearchState s, const Arc &arc, Weight w) {
  ParenMultiKey opmkey(paren_id, s.start);
  for (typename OpenParenMultiMap::iterator opit =
           open_paren_multimap_.find(opmkey);
       opit != open_paren_multimap_.end() && opit->first == opmkey;
       ++opit) {
    ParenKey key(paren_id, opit->second, s.start);
    OpenParenData &opdata = open_paren_map_[key];
    Weight cpw = Times(opdata.distance, w);
    Relax(s, opdata.state, arc, cpw);
  }

  if (!(sp_data_.Flags(s) & kExpanded)) {
    ParenMultiKey cpmkey(paren_id, s.start);
    CloseParenData cpdata(s, arc);
    close_paren_multimap_.insert(make_pair(cpmkey, cpdata));
  }
}

// For non-parentheses, classical relaxation.
template<class Arc, class Queue, class ArcFilter> inline
void PdtShortestPath<Arc, Queue, ArcFilter>::ProcNonParen(
    SearchState s, const Arc &arc, Weight w) {
  Relax(s, s, arc, w);
}

// Classical relaxation on the search graph for 'arc' from state 's'.
// State 't' is in the same sub-graph as the nextstate should be (i.e.
// has the same paren 'start'.
template<class Arc, class Queue, class ArcFilter> inline
void PdtShortestPath<Arc, Queue, ArcFilter>::Relax(
    SearchState s, SearchState t, Arc arc, Weight w) {
  SearchState d(arc.nextstate, t.start);
  Weight nd = sp_data_.Distance(d);

  if (nd != Plus(nd, w)) {
    sp_data_.SetParent(d, s);
    sp_data_.SetArcParent(d, arc);
    if (!(sp_data_.Flags(d) & kEnqueued)) {
      if (!enqueued_[d.state]) {
        state_queue_->Enqueue(d.state);
        enqueued_[d.state] = true;
      }
      start_queue_[d.state].push_back(d.start);
      sp_data_.SetFlags(d, kEnqueued, kEnqueued);
    } else if (enqueued_[d.state]) {
      state_queue_->Update(d.state);
    }
    sp_data_.SetDistance(d, Plus(nd, w));
  }
}

// Follows parent pointers to find the shortest path. Uses a stack
// since the shortest distance is stored recursively.
template<class Arc, class Queue, class ArcFilter>
void PdtShortestPath<Arc, Queue, ArcFilter>::GetPath(
    MutableFst<Arc> *ofst) {
  SearchState s = f_parent_, d = SearchState(kNoStateId, kNoStateId);
  StateId s_p = kNoStateId, d_p = kNoStateId;
  Arc arc(kNoArc);
  stack<ParenKey> paren_stack;
  while (s.state != kNoStateId) {
    d_p = s_p;
    s_p = ofst->AddState();
    if (d.state == kNoStateId) {
      ofst->SetFinal(s_p, ifst_->Final(f_parent_.state));
    } else {
      typename unordered_map<Label, Label>::const_iterator pit
          = paren_id_map_.find(arc.ilabel);
      if (pit != paren_id_map_.end()) {               // is a paren
        Label paren_id = pit->second;
        if (arc.ilabel == parens_[paren_id].first) {  // open paren
          paren_stack.pop();
        } else {                                      // close paren
          ParenKey key(paren_id, d.start, s.start);
          paren_stack.push(key);
        }
        arc.ilabel = arc.olabel = 0;
      }
      arc.nextstate = d_p;
      ofst->AddArc(s_p, arc);
    }
    d = s;
    s = sp_data_.Parent(d);
    arc = sp_data_.ArcParent(d);
    if (s.state == kNoStateId && !paren_stack.empty()) {
      ParenKey key = paren_stack.top();
      OpenParenData &opdata = open_paren_map_[key];
      s = opdata.state;
      arc = opdata.arc;
    }
  }
  ofst->SetStart(s_p);
  ofst->SetProperties(
      ShortestPathProperties(ofst->Properties(kFstProperties, false)),
      kFstProperties);
}

template<class Arc, class Queue, class ArcFilter>
const Arc PdtShortestPath<Arc, Queue, ArcFilter>::kNoArc
    = Arc(kNoLabel, kNoLabel, Weight::Zero(), kNoStateId);

template<class Arc, class Queue, class ArcFilter>
const size_t PdtShortestPath<Arc, Queue, ArcFilter>::kPrime0 = 7853;

template<class Arc, class Queue, class ArcFilter>
const size_t PdtShortestPath<Arc, Queue, ArcFilter>::kPrime1 = 7867;

template<class Arc, class Queue, class ArcFilter>
const uint8 PdtShortestPath<Arc, Queue, ArcFilter>::kEnqueued = 0x01;

template<class Arc, class Queue, class ArcFilter>
const uint8 PdtShortestPath<Arc, Queue, ArcFilter>::kExpanded = 0x02;


template<class Arc, class Queue, class ArcFilter>
void ShortestPath(const Fst<Arc> &ifst,
                  const vector<pair<typename Arc::Label,
                                    typename Arc::Label> > &parens,
                  MutableFst<Arc> *ofst,
                  ShortestPathOptions<Arc, Queue, ArcFilter> &opts) {
  size_t n = opts.nshortest;
  if (n == 1) {
    PdtShortestPath<Arc, Queue, ArcFilter> psp(ifst, parens, opts);
    psp.ShortestPath(ofst);
    return;
  } else {
    LOG(FATAL) << "ShortestPath: only single-source shortest path implemented "
               << "for PDTs";
  }
}

template<class Arc>
void ShortestPath(const Fst<Arc> &ifst,
                  const vector<pair<typename Arc::Label,
                                    typename Arc::Label> > &parens,
                  MutableFst<Arc> *ofst) {
  typedef typename Arc::StateId StateId;

  // FIFO queue for the default case for now.
  FifoQueue<StateId> queue;
  AnyArcFilter<Arc> arc_filter;
  ShortestPathOptions< Arc, FifoQueue<StateId>, AnyArcFilter<Arc> >
      opts(&queue, arc_filter);
  ShortestPath(ifst, parens, ofst, opts);
}


}  // namespace fst

#endif  // FST_EXTENSIONS_PDT_SHORTEST_PATH_H__
