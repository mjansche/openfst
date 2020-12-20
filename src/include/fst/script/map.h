// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_MAP_H_
#define FST_SCRIPT_MAP_H_

#include <fst/arc-map.h>
#include <fst/state-map.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/script/weight-class.h>

namespace fst {
namespace script {

template <class M>
Fst<typename M::ToArc> *ArcMap(const Fst<typename M::FromArc> &fst,
                               const M &mapper) {
  typedef typename M::ToArc ToArc;
  VectorFst<ToArc> *ofst = new VectorFst<ToArc>;
  ArcMap(fst, ofst, mapper);
  return ofst;
}

template <class M>
Fst<typename M::ToArc> *StateMap(const Fst<typename M::FromArc> &fst,
                                 const M &mapper) {
  typedef typename M::ToArc ToArc;
  VectorFst<ToArc> *ofst = new VectorFst<ToArc>;
  StateMap(fst, ofst, mapper);
  return ofst;
}

enum MapType {
  ARC_SUM_MAPPER,
  IDENTITY_MAPPER,
  INPUT_EPSILON_MAPPER,
  INVERT_MAPPER,
  OUTPUT_EPSILON_MAPPER,
  PLUS_MAPPER,
  QUANTIZE_MAPPER,
  RMWEIGHT_MAPPER,
  SUPERFINAL_MAPPER,
  TIMES_MAPPER,
  TO_LOG_MAPPER,
  TO_LOG64_MAPPER,
  TO_STD_MAPPER
};

typedef args::Package<const FstClass &, MapType, float, const WeightClass &>
    MapInnerArgs;
typedef args::WithReturnValue<FstClass *, MapInnerArgs> MapArgs;

template <class Arc>
void Map(MapArgs *args) {
  const Fst<Arc> &ifst = *(args->args.arg1.GetFst<Arc>());
  MapType map_type = args->args.arg2;
  float delta = args->args.arg3;
  typename Arc::Weight weight_param =
      *(args->args.arg4.GetWeight<typename Arc::Weight>());
  Fst<Arc> *fst = nullptr;
  Fst<LogArc> *lfst = nullptr;
  Fst<Log64Arc> *l64fst = nullptr;
  Fst<StdArc> *sfst = nullptr;
  if (map_type == ARC_SUM_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::StateMap(ifst, ArcSumMapper<Arc>(ifst))));
  } else if (map_type == IDENTITY_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::ArcMap(ifst, IdentityArcMapper<Arc>())));
  } else if (map_type == INPUT_EPSILON_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::ArcMap(ifst, InputEpsilonMapper<Arc>())));
  } else if (map_type == INVERT_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::ArcMap(ifst, InvertWeightMapper<Arc>())));
  } else if (map_type == OUTPUT_EPSILON_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::ArcMap(ifst, OutputEpsilonMapper<Arc>())));
  } else if (map_type == PLUS_MAPPER) {
    args->retval = new FstClass(
        *(fst = script::ArcMap(ifst, PlusMapper<Arc>(weight_param))));
  } else if (map_type == QUANTIZE_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::ArcMap(ifst, QuantizeMapper<Arc>(delta))));
  } else if (map_type == RMWEIGHT_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::ArcMap(ifst, RmWeightMapper<Arc>())));
  } else if (map_type == SUPERFINAL_MAPPER) {
    args->retval =
        new FstClass(*(fst = script::ArcMap(ifst, SuperFinalMapper<Arc>())));
  } else if (map_type == TIMES_MAPPER) {
    args->retval = new FstClass(
        *(fst = script::ArcMap(ifst, TimesMapper<Arc>(weight_param))));
  } else if (map_type == TO_LOG_MAPPER) {
    args->retval = new FstClass(
        *(lfst = script::ArcMap(ifst, WeightConvertMapper<Arc, LogArc>())));
  } else if (map_type == TO_LOG64_MAPPER) {
    args->retval = new FstClass(
        *(l64fst = script::ArcMap(ifst, WeightConvertMapper<Arc, Log64Arc>())));
  } else if (map_type == TO_STD_MAPPER) {
    args->retval = new FstClass(
        *(sfst = script::ArcMap(ifst, WeightConvertMapper<Arc, StdArc>())));
  } else {
    FSTERROR() << "Unknown mapper type: " << map_type;
    VectorFst<Arc> *ofst = new VectorFst<Arc>;
    ofst->SetProperties(kError, kError);
    args->retval = new FstClass(*(fst = ofst));
  }
  delete sfst;
  delete l64fst;
  delete lfst;
  delete fst;
}

FstClass *Map(const FstClass &fst, MapType map_type, float delta,
              const WeightClass &weight_param);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_MAP_H_
