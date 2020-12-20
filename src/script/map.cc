// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/fst-class.h>
#include <fst/script/map.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {

FstClass *Map(const FstClass &ifst, MapType map_type, float delta,
              const WeightClass &weight_param) {
  if (!ifst.WeightTypesMatch(weight_param, "Map")) return nullptr;
  MapInnerArgs iargs(ifst, map_type, delta, weight_param);
  MapArgs args(iargs);
  Apply<Operation<MapArgs>>("Map", ifst.ArcType(), &args);
  return args.retval;
}

REGISTER_FST_OPERATION(Map, StdArc, MapArgs);
REGISTER_FST_OPERATION(Map, LogArc, MapArgs);
REGISTER_FST_OPERATION(Map, Log64Arc, MapArgs);

}  // namespace script
}  // namespace fst
