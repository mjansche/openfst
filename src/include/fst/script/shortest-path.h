
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
// Author: jpr@google.com (Jake Ratkiewicz)

#ifndef FST_SCRIPT_SHORTEST_PATH_H_
#define FST_SCRIPT_SHORTEST_PATH_H_

#include <vector>
using std::vector;

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/script/weight-class.h>
#include <fst/shortest-path.h>
#include <fst/script/shortest-distance.h>  // for ShortestDistanceOptions

namespace fst {
namespace script {

struct ShortestPathOptions
    : public fst::script::ShortestDistanceOptions {
  const size_t nshortest;
  const bool unique;
  const bool has_distance;
  const bool first_path;
  const WeightClass weight_threshold;
  const int64 state_threshold;

  ShortestPathOptions(QueueType qt, ArcFilterType aft, size_t n = 1,
                      bool u = false, bool hasdist = false,
                      float d = fst::kDelta, bool fp = false,
                      WeightClass w = fst::script::WeightClass::Zero(),
                      int64 s = fst::kNoStateId)
      : ShortestDistanceOptions(qt, aft, kNoStateId, d),
        nshortest(n), unique(u), has_distance(hasdist), first_path(fp),
        weight_threshold(w), state_threshold(s) { }
};

// typedef args::Package<const FstClass&, MutableFstClass*,
//                       vector<WeightClass>*, int64, bool, double,
//                       WeightClass, int64> ShortestPathArgs;

// template<class Arc>
// void ShortestPath(ShortestPathArgs *args) {
//   typedef typename Arc::Weight Weight;
//   typedef typename Arc::StateId StateId;

//   const Fst<Arc> &ifst = *(args->arg1.GetFst<Arc>());
//   MutableFst<Arc> *ofst = args->arg2->GetMutableFst<Arc>();
//   Weight weight_threshold = *(args->arg7.GetWeight<Weight>());

//   vector<Weight> distance;

//   AnyArcFilter<Arc> arc_filter;
//   AutoQueue<StateId> state_queue(ifst, &distance, arc_filter);

//   ShortestPathOptions<Arc, AutoQueue<StateId>, AnyArcFilter<Arc> >
//       sopts(&state_queue, arc_filter, args->arg4, args->arg5,
//       false, args->arg6, false, weight_threshold, args->arg8);
//   ShortestPath(ifst, ofst, &distance, sopts);

//   // Copy the weights back into the vector of WeightClass
//   vector<WeightClass> *retval = args->arg3;
//   retval->resize(distance.size());

//   for (unsigned i = 0; i < distance.size(); ++i) {
//     (*retval)[i] = WeightClass(distance[i]);
//   }
// }


typedef args::Package<const FstClass &, MutableFstClass *,
                      vector<WeightClass> *, const ShortestPathOptions &>
  ShortestPathArgs1;


template<class Arc, class Queue>
void ShortestPathHelper(ShortestPathArgs1 *args) {
  const Fst<Arc> &ifst = *(args->arg1.GetFst<Arc>());
  MutableFst<Arc> *ofst = args->arg2->GetMutableFst<Arc>();
  const ShortestPathOptions &opts = args->arg4;

  vector<typename Arc::Weight> weights;
  typename Arc::Weight weight_threshold =
      *(opts.weight_threshold.GetWeight<typename Arc::Weight>());

  switch (opts.arc_filter_type) {
    case ANY_ARC_FILTER: {
      Queue queue =
          QueueConstructor<Queue, Arc, AnyArcFilter<Arc> >::Construct(
              ifst, &weights);
      fst::ShortestPathOptions<Arc, Queue, AnyArcFilter<Arc> > spopts(
          &queue, AnyArcFilter<Arc>(), opts.nshortest, opts.unique,
          opts.has_distance, opts.delta, opts.first_path,
          weight_threshold, opts.state_threshold);

      ShortestPath(ifst, ofst, &weights, spopts);
      break;
    }
    case EPSILON_ARC_FILTER: {
      Queue queue =
          QueueConstructor<Queue, Arc, EpsilonArcFilter<Arc> >::Construct(
              ifst, &weights);

      fst::ShortestPathOptions<Arc, Queue, EpsilonArcFilter<Arc> > spopts(
          &queue, EpsilonArcFilter<Arc>(), opts.nshortest, opts.unique,
          opts.has_distance, opts.delta, opts.first_path,
          weight_threshold, opts.state_threshold);

      ShortestPath(ifst, ofst, &weights, spopts);
      break;
    }
    case INPUT_EPSILON_ARC_FILTER: {
      Queue queue =
          QueueConstructor<Queue, Arc, InputEpsilonArcFilter<Arc> >::Construct(
              ifst, &weights);

      fst::ShortestPathOptions<Arc, Queue,
          InputEpsilonArcFilter<Arc> > spopts(
              &queue, InputEpsilonArcFilter<Arc>(), opts.nshortest,
              opts.unique, opts.has_distance, opts.delta, opts.first_path,
              weight_threshold, opts.state_threshold);

      ShortestPath(ifst, ofst, &weights, spopts);
      break;
    }
    case OUTPUT_EPSILON_ARC_FILTER: {
      Queue queue =
          QueueConstructor<Queue, Arc, OutputEpsilonArcFilter<Arc> >::Construct(
              ifst, &weights);

      fst::ShortestPathOptions<Arc, Queue,
          OutputEpsilonArcFilter<Arc> > spopts(
              &queue, OutputEpsilonArcFilter<Arc>(), opts.nshortest,
              opts.unique, opts.has_distance, opts.delta, opts.first_path,
              weight_threshold, opts.state_threshold);

      ShortestPath(ifst, ofst, &weights, spopts);
      break;
    }
  }

  // Copy the weights back
  CHECK(args->arg3);

  args->arg3->resize(weights.size());
  for (unsigned i = 0; i < weights.size(); ++i) {
    (*args->arg3)[i] = WeightClass(weights[i]);
  }
}


template<class Arc>
void ShortestPath(ShortestPathArgs1 *args) {
  const ShortestPathOptions &opts = args->arg4;
  typedef typename Arc::StateId StateId;

  // Must consider (opts.queue_type x opts.filter_type) options
  switch (opts.queue_type) {
    case TRIVIAL_QUEUE:
      ShortestPathHelper<Arc, TrivialQueue<StateId> >(args);
      return;

    case FIFO_QUEUE:
       ShortestPathHelper<Arc, FifoQueue<StateId> >(args);
      return;

    case LIFO_QUEUE:
       ShortestPathHelper<Arc, LifoQueue<StateId> >(args);
      return;

//     case SHORTEST_FIRST_QUEUE:
//        ShortestPathHelper<Arc, ShortestFirstQueue<StateId> >(args);
//       return;

    case TOP_ORDER_QUEUE:
       ShortestPathHelper<Arc, TopOrderQueue<StateId> >(args);
      return;

    case STATE_ORDER_QUEUE:
       ShortestPathHelper<Arc, StateOrderQueue<StateId> >(args);
      return;

    case AUTO_QUEUE:
      ShortestPathHelper<Arc, AutoQueue<StateId> >(args);
      return;

    default:
      LOG(FATAL) << "Unknown queue type: " << opts.queue_type;
  }
}

// 2
typedef args::Package<const FstClass &, MutableFstClass *,
                      size_t, bool, bool, WeightClass,
                      int64> ShortestPathArgs2;

template<class Arc>
void ShortestPath(ShortestPathArgs2 *args) {
  const Fst<Arc> &ifst = *(args->arg1.GetFst<Arc>());
  MutableFst<Arc> *ofst = args->arg2->GetMutableFst<Arc>();
  typename Arc::Weight weight_threshold =
      *(args->arg6.GetWeight<typename Arc::Weight>());

  ShortestPath(ifst, ofst, args->arg3, args->arg4, args->arg5,
               weight_threshold, args->arg7);
}


// 1
void ShortestPath(const FstClass &ifst, MutableFstClass *ofst,
                  vector<WeightClass> *distance,
                  const ShortestPathOptions &opts);


// 2
void ShortestPath(const FstClass &ifst, MutableFstClass *ofst,
                  size_t n = 1, bool unique = false,
                  bool first_path = false,
                  WeightClass weight_threshold =
                    fst::script::WeightClass::Zero(),
                  int64 state_threshold = fst::kNoStateId);

}  // namespace script
}  // namespace fst



#endif  // FST_SCRIPT_SHORTEST_PATH_H_
