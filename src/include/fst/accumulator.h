// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Classes to accumulate arc weights. Useful for weight lookahead.

#ifndef FST_LIB_ACCUMULATOR_H_
#define FST_LIB_ACCUMULATOR_H_

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <vector>

#include <fst/arcfilter.h>
#include <fst/arcsort.h>
#include <fst/dfs-visit.h>
#include <fst/expanded-fst.h>
#include <fst/replace.h>

namespace fst {

// This class accumulates arc weights using the semiring Plus().
template <class A>
class DefaultAccumulator {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  DefaultAccumulator() {}

  DefaultAccumulator(const DefaultAccumulator<A> &acc, bool safe = false) {}

  void Init(const Fst<A> &fst, bool copy = false) {}

  void SetState(StateId) {}

  Weight Sum(Weight w, Weight v) { return Plus(w, v); }

  template <class ArcIterator>
  Weight Sum(Weight w, ArcIterator *aiter, ssize_t begin, ssize_t end) {
    Weight sum = w;
    aiter->Seek(begin);
    for (ssize_t pos = begin; pos < end; aiter->Next(), ++pos)
      sum = Plus(sum, aiter->Value().weight);
    return sum;
  }

  bool Error() const { return false; }

 private:
  void operator=(const DefaultAccumulator<A> &);  // Disallow
};

// This class accumulates arc weights using the log semiring Plus()
// assuming an arc weight has a WeightConvert specialization to
// and from log64 weights.
template <class A>
class LogAccumulator {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  LogAccumulator() {}

  LogAccumulator(const LogAccumulator<A> &acc, bool safe = false) {}

  void Init(const Fst<A> &fst, bool copy = false) {}

  void SetState(StateId) {}

  Weight Sum(Weight w, Weight v) { return LogPlus(w, v); }

  template <class ArcIterator>
  Weight Sum(Weight w, ArcIterator *aiter, ssize_t begin, ssize_t end) {
    Weight sum = w;
    aiter->Seek(begin);
    for (ssize_t pos = begin; pos < end; aiter->Next(), ++pos)
      sum = LogPlus(sum, aiter->Value().weight);
    return sum;
  }

  bool Error() const { return false; }

 private:
  double LogPosExp(double x) { return log(1.0F + exp(-x)); }

  Weight LogPlus(Weight w, Weight v) {
    double f1 = to_log_weight_(w).Value();
    double f2 = to_log_weight_(v).Value();
    if (f1 > f2)
      return to_weight_(f2 - LogPosExp(f1 - f2));
    else
      return to_weight_(f1 - LogPosExp(f2 - f1));
  }

  WeightConvert<Weight, Log64Weight> to_log_weight_;
  WeightConvert<Log64Weight, Weight> to_weight_;

  void operator=(const LogAccumulator<A> &);  // Disallow
};

// Interface for shareable data for fast log accumulator copies. Holds pointers
// to data only, storage is provided by derived classes.
class FastLogAccumulatorData {
 public:
  FastLogAccumulatorData(int arc_limit, int arc_period)
      : arc_limit_(arc_limit),
        arc_period_(arc_period),
        weights_ptr_(nullptr),
        num_weights_(0),
        weight_positions_ptr_(nullptr),
        num_positions_(0) {}

  virtual ~FastLogAccumulatorData() {}

  // Cummulative weight per state for all states s.t. # of arcs >
  // arc_limit_ with arcs in order. Special first element per state
  // being Log64Weight::Zero();
  const double *Weights() const { return weights_ptr_; }
  int NumWeights() const { return num_weights_; }

  // Maps from state to corresponding beginning weight position in
  // weights_. Position -1 means no pre-computed weights for that
  // state.
  const int *WeightPositions() const { return weight_positions_ptr_; }
  int NumPositions() const { return num_positions_; }

  int ArcLimit() const { return arc_limit_; }
  int ArcPeriod() const { return arc_period_; }

  // Returns true if the data object is mutable and supports SetData().
  virtual bool IsMutable() const = 0;

  // Does not take ownership but may invalidate the contents of weights and
  // weight_positions.
  virtual void SetData(vector<double> *weights,
                       vector<int> *weight_positions) = 0;

 protected:
  void Init(int num_weights, const double *weights, int num_positions,
            const int *weight_positions) {
    weights_ptr_ = weights;
    num_weights_ = num_weights;
    weight_positions_ptr_ = weight_positions;
    num_positions_ = num_positions;
  }

 private:
  const int arc_limit_;
  const int arc_period_;
  const double *weights_ptr_;
  int num_weights_;
  const int *weight_positions_ptr_;
  int num_positions_;

  DISALLOW_COPY_AND_ASSIGN(FastLogAccumulatorData);
};

// FastLogAccumulatorData with mutable storage.
// Filled by FastLogAccumulator::Init.
class MutableFastLogAccumulatorData : public FastLogAccumulatorData {
 public:
  MutableFastLogAccumulatorData(int arc_limit, int arc_period)
      : FastLogAccumulatorData(arc_limit, arc_period) {}

  bool IsMutable() const override { return true; }

  void SetData(vector<double> *weights,
               vector<int> *weight_positions) override {
    weights_.swap(*weights);
    weight_positions_.swap(*weight_positions);
    Init(weights_.size(), weights_.data(), weight_positions_.size(),
         weight_positions_.data());
  }

 private:
  vector<double> weights_;
  vector<int> weight_positions_;

  DISALLOW_COPY_AND_ASSIGN(MutableFastLogAccumulatorData);
};

// This class accumulates arc weights using the log semiring Plus()
// assuming an arc weight has a WeightConvert specialization to and
// from log64 weights. The member function Init(fst) has to be called
// to setup pre-computed weight information.
template <class A>
class FastLogAccumulator {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  explicit FastLogAccumulator(ssize_t arc_limit = 20, ssize_t arc_period = 10)

      : to_log_weight_(),
        to_weight_(),
        arc_limit_(arc_limit),
        arc_period_(arc_period),
        data_(std::make_shared<MutableFastLogAccumulatorData>(arc_limit,
                                                              arc_period)),
        state_weights_(nullptr),
        error_(false) {}

  explicit FastLogAccumulator(std::shared_ptr<FastLogAccumulatorData> data)
      : to_log_weight_(),
        to_weight_(),
        arc_limit_(data->ArcLimit()),
        arc_period_(data->ArcPeriod()),
        data_(data),
        state_weights_(nullptr),
        error_(false) {}

  FastLogAccumulator(const FastLogAccumulator<A> &acc, bool safe = false)
      : to_log_weight_(),
        to_weight_(),
        arc_limit_(acc.arc_limit_),
        arc_period_(acc.arc_period_),
        data_(acc.data_),
        state_weights_(nullptr),
        error_(acc.error_) {}

  void SetState(StateId s) {
    const double *weights = data_->Weights();
    const int *weight_positions = data_->WeightPositions();

    state_weights_ = nullptr;
    if (s < data_->NumPositions()) {
      const int pos = weight_positions[s];
      if (pos >= 0) state_weights_ = &(weights[pos]);
    }
  }

  Weight Sum(Weight w, Weight v) const { return LogPlus(w, v); }

  template <class ArcIterator>
  Weight Sum(Weight w, ArcIterator *aiter, ssize_t begin, ssize_t end) const {
    if (error_) return Weight::NoWeight();
    Weight sum = w;
    // Finds begin and end of pre-stored weights
    ssize_t index_begin = -1, index_end = -1;
    ssize_t stored_begin = end, stored_end = end;
    if (state_weights_ != nullptr) {
      index_begin = begin > 0 ? (begin - 1) / arc_period_ + 1 : 0;
      index_end = end / arc_period_;
      stored_begin = index_begin * arc_period_;
      stored_end = index_end * arc_period_;
    }
    // Computes sum before pre-stored weights
    if (begin < stored_begin) {
      ssize_t pos_end = std::min(stored_begin, end);
      aiter->Seek(begin);
      for (ssize_t pos = begin; pos < pos_end; aiter->Next(), ++pos)
        sum = LogPlus(sum, aiter->Value().weight);
    }
    // Computes sum between pre-stored weights
    if (stored_begin < stored_end) {
      double f1 = state_weights_[index_end];
      double f2 = state_weights_[index_begin];
      if (f1 < f2) {
        sum = LogPlus(sum, LogMinus(f1, f2));
      }
      // commented out for efficiency; adds Zero()
      /* else {
        // explicitly computes if cumulative sum lacks precision
        aiter->Seek(stored_begin);
        for (ssize_t pos = stored_begin; pos < stored_end; aiter->Next(), ++pos)
          sum = LogPlus(sum, aiter->Value().weight);
      } */
    }
    // Computes sum after pre-stored weights
    if (stored_end < end) {
      ssize_t pos_start = std::max(stored_begin, stored_end);
      aiter->Seek(pos_start);
      for (ssize_t pos = pos_start; pos < end; aiter->Next(), ++pos)
        sum = LogPlus(sum, aiter->Value().weight);
    }
    return sum;
  }

  template <class F>
  void Init(const F &fst, bool copy = false) {
    if (copy || !data_->IsMutable()) {
      return;
    }
    if (data_->NumPositions() != 0 || arc_limit_ < arc_period_) {
      FSTERROR() << "FastLogAccumulator: Initialization error";
      error_ = true;
      return;
    }
    vector<double> weights;
    vector<int> weight_positions;
    weight_positions.reserve(CountStates(fst));

    int weight_position = 0;
    for (StateIterator<F> siter(fst); !siter.Done(); siter.Next()) {
      StateId s = siter.Value();
      if (fst.NumArcs(s) >= arc_limit_) {
        double sum = FloatLimits<double>::PosInfinity();
        weight_positions.push_back(weight_position);
        weights.push_back(sum);
        ++weight_position;
        ssize_t narcs = 0;
        for (ArcIterator<F> aiter(fst, s); !aiter.Done(); aiter.Next()) {
          const A &arc = aiter.Value();
          sum = LogPlus(sum, arc.weight);
          // Stores cumulative weight distribution per arc_period_.
          if (++narcs % arc_period_ == 0) {
            weights.push_back(sum);
            ++weight_position;
          }
        }
      } else {
        weight_positions.push_back(-1);
      }
    }
    data_->SetData(&weights, &weight_positions);
  }

  bool Error() const { return error_; }

  std::shared_ptr<FastLogAccumulatorData> GetData() const { return data_; }

 private:
  static double LogPosExp(double x) {
    return x == FloatLimits<double>::PosInfinity() ? 0.0
                                                   : log(1.0F + exp(-x));
  }

  static double LogMinusExp(double x) {
    return x == FloatLimits<double>::PosInfinity() ? 0.0
                                                   : log(1.0F - exp(-x));
  }

  Weight LogPlus(Weight w, Weight v) const {
    double f1 = to_log_weight_(w).Value();
    double f2 = to_log_weight_(v).Value();
    if (f1 > f2)
      return to_weight_(f2 - LogPosExp(f1 - f2));
    else
      return to_weight_(f1 - LogPosExp(f2 - f1));
  }

  double LogPlus(double f1, Weight v) const {
    double f2 = to_log_weight_(v).Value();
    if (f1 == FloatLimits<double>::PosInfinity())
      return f2;
    else if (f1 > f2)
      return f2 - LogPosExp(f1 - f2);
    else
      return f1 - LogPosExp(f2 - f1);
  }

  // Assumes f1 < f2
  Weight LogMinus(double f1, double f2) const {
    if (f2 == FloatLimits<double>::PosInfinity())
      return to_weight_(f1);
    else
      return to_weight_(f1 - LogMinusExp(f2 - f1));
  }

  const WeightConvert<Weight, Log64Weight> to_log_weight_;
  const WeightConvert<Log64Weight, Weight> to_weight_;

  const ssize_t arc_limit_;   // Minimum # of arcs to pre-compute state.
  const ssize_t arc_period_;  // Save cumulative weights per 'arc_period_'.
  std::shared_ptr<FastLogAccumulatorData> data_;
  const double *state_weights_;
  bool error_;

  void operator=(const FastLogAccumulator<A> &);  // Disallow
};

// Stores shareable data for cache log accumulator copies.
// All copies share the same cache.
template <class A>
class CacheLogAccumulatorData {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  CacheLogAccumulatorData(bool gc, size_t gc_limit)
      : cache_gc_(gc), cache_limit_(gc_limit), cache_size_(0) {}

  CacheLogAccumulatorData(const CacheLogAccumulatorData<A> &data)
      : cache_gc_(data.cache_gc_),
        cache_limit_(data.cache_limit_),
        cache_size_(0) {}

  ~CacheLogAccumulatorData() {
    for (auto it = cache_.begin(); it != cache_.end(); ++it)
      delete it->second.weights;
  }

  bool CacheDisabled() const { return cache_gc_ && cache_limit_ == 0; }

  std::vector<double> *GetWeights(StateId s) {
    auto it = cache_.find(s);
    if (it != cache_.end()) {
      it->second.recent = true;
      return it->second.weights;
    } else {
      return 0;
    }
  }

  void AddWeights(StateId s, std::vector<double> *weights) {
    if (cache_gc_ && cache_size_ >= cache_limit_) GC(false);
    cache_.insert(std::make_pair(s, CacheState(weights, true)));
    if (cache_gc_) cache_size_ += weights->capacity() * sizeof(double);
  }

 private:
  // Cached information for a given state.
  struct CacheState {
    std::vector<double> *weights;  // Accumulated weights for this state.
    bool recent;              // Has this state been accessed since last GC?

    CacheState(std::vector<double> *w, bool r) : weights(w), recent(r) {}
  };

  // Garbage collect: Delete from cache states that have not been
  // accessed since the last GC ('free_recent = false') until
  // 'cache_size_' is 2/3 of 'cache_limit_'. If it does not free enough
  // memory, start deleting recently accessed states.
  void GC(bool free_recent) {
    size_t cache_target = (2 * cache_limit_) / 3 + 1;
    auto it = cache_.begin();
    while (it != cache_.end() && cache_size_ > cache_target) {
      CacheState &cs = it->second;
      if (free_recent || !cs.recent) {
        cache_size_ -= cs.weights->capacity() * sizeof(double);
        delete cs.weights;
        cache_.erase(it++);
      } else {
        cs.recent = false;
        ++it;
      }
    }
    if (!free_recent && cache_size_ > cache_target) GC(true);
  }

  std::unordered_map<StateId, CacheState> cache_;  // Cache
  bool cache_gc_;                        // Enable garbage collection
  size_t cache_limit_;                   // # of bytes cached
  size_t cache_size_;                    // # of bytes allowed before GC

  void operator=(const CacheLogAccumulatorData<A> &);  // Disallow
};

// This class accumulates arc weights using the log semiring Plus()
//  has a WeightConvert specialization to and from log64 weights.  It
//  is similar to the FastLogAccumator. However here, the accumulated
//  weights are pre-computed and stored only for the states that are
//  visited. The member function Init(fst) has to be called to setup
//  this accumulator.
template <class A>
class CacheLogAccumulator {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  explicit CacheLogAccumulator(ssize_t arc_limit = 10, bool gc = false,
                               size_t gc_limit = 10 * 1024 * 1024)
      : arc_limit_(arc_limit),
        fst_(0),
        data_(std::make_shared<CacheLogAccumulatorData<A>>(gc, gc_limit)),
        s_(kNoStateId),
        error_(false) {}

  CacheLogAccumulator(const CacheLogAccumulator<A> &acc, bool safe = false)
      : arc_limit_(acc.arc_limit_),
        fst_(acc.fst_ ? acc.fst_->Copy() : 0),
        data_(safe ? std::make_shared<CacheLogAccumulatorData<A>>(*acc.data_)
                   : acc.data_),
        s_(kNoStateId),
        error_(acc.error_) {}

  ~CacheLogAccumulator() {
    if (fst_) delete fst_;
  }

  // Arg 'arc_limit' specifies minimum # of arcs to pre-compute state.
  void Init(const Fst<A> &fst, bool copy = false) {
    if (copy) {
      delete fst_;
    } else if (fst_) {
      FSTERROR() << "CacheLogAccumulator: Initialization error";
      error_ = true;
      return;
    }
    fst_ = fst.Copy();
  }

  void SetState(StateId s, int depth = 0) {
    if (s == s_) return;
    s_ = s;

    if (data_->CacheDisabled() || error_) {
      weights_ = 0;
      return;
    }

    if (!fst_) {
      FSTERROR() << "CacheLogAccumulator::SetState: Incorrectly initialized";
      error_ = true;
      weights_ = 0;
      return;
    }

    weights_ = data_->GetWeights(s);
    if ((weights_ == 0) && (fst_->NumArcs(s) >= arc_limit_)) {
      weights_ = new std::vector<double>;
      weights_->reserve(fst_->NumArcs(s) + 1);
      weights_->push_back(FloatLimits<double>::PosInfinity());
      data_->AddWeights(s, weights_);
    }
  }

  Weight Sum(Weight w, Weight v) { return LogPlus(w, v); }

  template <class Iterator>
  Weight Sum(Weight w, Iterator *aiter, ssize_t begin, ssize_t end) {
    if (weights_ == 0) {
      Weight sum = w;
      aiter->Seek(begin);
      for (ssize_t pos = begin; pos < end; aiter->Next(), ++pos)
        sum = LogPlus(sum, aiter->Value().weight);
      return sum;
    } else {
      if (weights_->size() <= end) {
        for (aiter->Seek(weights_->size() - 1); weights_->size() <= end;
             aiter->Next()) {
          weights_->push_back(LogPlus(weights_->back(), aiter->Value().weight));
        }
      }
      double f1 = (*weights_)[end];
      double f2 = (*weights_)[begin];
      if (f1 < f2) {
        return LogPlus(w, LogMinus(f1, f2));
      } else {
        Weight sum = w;
        // commented out for efficiency; adds Zero()
        /*
        // explicitly computes if cumulative sum lacks precision
        aiter->Seek(begin);
        for (ssize_t pos = begin; pos < end; aiter->Next(), ++pos)
          sum = LogPlus(sum, aiter->Value().weight);
        */
        return sum;
      }
    }
  }

  // Arc iterator position determines beginning point of lower bound.
  template <class Iterator>
  size_t LowerBound(double w, Iterator *aiter) {
    if (weights_ != 0) {
      ssize_t pos = aiter->Position();
      return std::lower_bound(weights_->begin() + pos + 1,
                              weights_->end(), w, std::greater<double>()) -
             weights_->begin() - 1;
    } else {
      size_t n = 0;
      double x = FloatLimits<double>::PosInfinity();
      for (; !aiter->Done(); aiter->Next(), ++n) {
        x = LogPlus(x, aiter->Value().weight);
        if (x < w) break;
      }
      return n;
    }
  }

  bool Error() const { return error_; }

 private:
  double LogPosExp(double x) {
    return x == FloatLimits<double>::PosInfinity() ? 0.0
                                                   : log(1.0F + exp(-x));
  }

  double LogMinusExp(double x) {
    return x == FloatLimits<double>::PosInfinity() ? 0.0
                                                   : log(1.0F - exp(-x));
  }

  Weight LogPlus(Weight w, Weight v) {
    double f1 = to_log_weight_(w).Value();
    double f2 = to_log_weight_(v).Value();
    if (f1 > f2)
      return to_weight_(f2 - LogPosExp(f1 - f2));
    else
      return to_weight_(f1 - LogPosExp(f2 - f1));
  }

  double LogPlus(double f1, Weight v) {
    double f2 = to_log_weight_(v).Value();
    if (f1 == FloatLimits<double>::PosInfinity())
      return f2;
    else if (f1 > f2)
      return f2 - LogPosExp(f1 - f2);
    else
      return f1 - LogPosExp(f2 - f1);
  }

  // Assumes f1 < f2
  Weight LogMinus(double f1, double f2) {
    if (f2 == FloatLimits<double>::PosInfinity())
      return to_weight_(f1);
    else
      return to_weight_(f1 - LogMinusExp(f2 - f1));
  }

  WeightConvert<Weight, Log64Weight> to_log_weight_;
  WeightConvert<Log64Weight, Weight> to_weight_;

  ssize_t arc_limit_;                 // Minimum # of arcs to cache a state
  std::vector<double> *weights_;      // Accumulated weights for cur. state
  const Fst<A> *fst_;                 // Input fst
  std::shared_ptr<CacheLogAccumulatorData<A>> data_;  // Cache data
  StateId s_;                         // Current state
  bool error_;

  void operator=(const CacheLogAccumulator<A> &);  // Disallow
};

// Stores shareable data for replace accumulator copies.
template <class Accumulator, class T>
class ReplaceAccumulatorData {
 public:
  typedef typename Accumulator::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef T StateTable;
  typedef typename T::StateTuple StateTuple;

  ReplaceAccumulatorData() : state_table_(0) {}

  explicit ReplaceAccumulatorData(
      const std::vector<Accumulator *> &accumulators)
      : state_table_(0), accumulators_(accumulators) {}

  ~ReplaceAccumulatorData() {
    for (size_t i = 0; i < fst_array_.size(); ++i) delete fst_array_[i];
    for (size_t i = 0; i < accumulators_.size(); ++i) delete accumulators_[i];
  }

  void Init(const std::vector<std::pair<Label, const Fst<Arc> *>> &fst_tuples,
            const StateTable *state_table) {
    state_table_ = state_table;
    accumulators_.resize(fst_tuples.size());
    for (size_t i = 0; i < accumulators_.size(); ++i) {
      if (!accumulators_[i]) {
        accumulators_[i] = new Accumulator;
        accumulators_[i]->Init(*(fst_tuples[i].second));
      }
      fst_array_.push_back(fst_tuples[i].second->Copy());
    }
  }

  const StateTuple &GetTuple(StateId s) const { return state_table_->Tuple(s); }

  Accumulator *GetAccumulator(size_t i) { return accumulators_[i]; }

  const Fst<Arc> *GetFst(size_t i) const { return fst_array_[i]; }

 private:
  const T *state_table_;
  std::vector<Accumulator *> accumulators_;
  std::vector<const Fst<Arc> *> fst_array_;

  DISALLOW_COPY_AND_ASSIGN(ReplaceAccumulatorData);
};

// This class accumulates weights in a ReplaceFst.  The 'Init' method
// takes as input the argument used to build the ReplaceFst and the
// ReplaceFst state table. It uses accumulators of type 'Accumulator'
// in the underlying FSTs.
template <class Accumulator,
          class T = DefaultReplaceStateTable<typename Accumulator::Arc>>
class ReplaceAccumulator {
 public:
  typedef typename Accumulator::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;
  typedef T StateTable;
  typedef typename T::StateTuple StateTuple;

  ReplaceAccumulator()
      : init_(false),
        data_(std::make_shared<ReplaceAccumulatorData<Accumulator, T>>()),
        aiter_(0),
        error_(false) {}

  explicit ReplaceAccumulator(const std::vector<Accumulator *> &accumulators)
      : init_(false),
        data_(std::make_shared<ReplaceAccumulatorData<Accumulator, T>>(
            accumulators)),
        aiter_(0),
        error_(false) {}

  ReplaceAccumulator(const ReplaceAccumulator<Accumulator, T> &acc,
                     bool safe = false)
      : init_(acc.init_), data_(acc.data_), aiter_(0), error_(acc.error_) {
    if (!init_) {
      FSTERROR() << "ReplaceAccumulator: Can't copy unintialized accumulator";
    }
    if (safe) {
      FSTERROR() << "ReplaceAccumulator: Safe copy not supported";
    }
  }

  ~ReplaceAccumulator() {
    if (aiter_) delete aiter_;
  }

  // Does not take ownership of the state table, the state table
  // is own by the ReplaceFst
  void Init(const std::vector<std::pair<Label, const Fst<Arc> *>> &fst_tuples,
            const StateTable *state_table) {
    init_ = true;
    data_->Init(fst_tuples, state_table);
  }

  // Method required by LookAheadMatcher. However, ReplaceAccumulator
  // needs to be initialized by calling the Init method above before
  // being passed to LookAheadMatcher.
  // TODO(allauzen): Revisit this. Consider creating an
  // Init(const ReplaceFst<A, T, C>&, bool) method and using friendship to
  // get access to the innards of ReplaceFst.
  void Init(const Fst<Arc> &fst, bool copy = false) {
    if (!init_) {
      FSTERROR() << "ReplaceAccumulator::Init: Accumulator needs to be"
                 << " initialized before being passed to LookAheadMatcher";
      error_ = true;
    }
  }

  void SetState(StateId s) {
    if (!init_) {
      FSTERROR() << "ReplaceAccumulator::SetState: Incorrectly initialized";
      error_ = true;
      return;
    }
    StateTuple tuple = data_->GetTuple(s);
    fst_id_ = tuple.fst_id - 1;  // Replace FST ID is 1-based
    data_->GetAccumulator(fst_id_)->SetState(tuple.fst_state);
    if ((tuple.prefix_id != 0) &&
        (data_->GetFst(fst_id_)->Final(tuple.fst_state) != Weight::Zero())) {
      offset_ = 1;
      offset_weight_ = data_->GetFst(fst_id_)->Final(tuple.fst_state);
    } else {
      offset_ = 0;
      offset_weight_ = Weight::Zero();
    }
    if (aiter_) delete aiter_;
    aiter_ =
        new ArcIterator<Fst<Arc>>(*data_->GetFst(fst_id_), tuple.fst_state);
  }

  Weight Sum(Weight w, Weight v) {
    if (error_) return Weight::NoWeight();
    return data_->GetAccumulator(fst_id_)->Sum(w, v);
  }

  template <class ArcIterator>
  Weight Sum(Weight w, ArcIterator *aiter, ssize_t begin, ssize_t end) {
    if (error_) return Weight::NoWeight();
    Weight sum = begin == end ? Weight::Zero()
                              : data_->GetAccumulator(fst_id_)->Sum(
                                    w, aiter_, begin ? begin - offset_ : 0,
                                    end - offset_);
    if (begin == 0 && end != 0 && offset_ > 0) sum = Sum(offset_weight_, sum);
    return sum;
  }

  bool Error() const { return error_; }

 private:
  bool init_;
  std::shared_ptr<ReplaceAccumulatorData<Accumulator, T>> data_;
  Label fst_id_;
  size_t offset_;
  Weight offset_weight_;
  ArcIterator<Fst<Arc>> *aiter_;
  bool error_;

  void operator=(const ReplaceAccumulator<Accumulator, T> &);  // Disallow
};

}  // namespace fst

#endif  // FST_LIB_ACCUMULATOR_H_
