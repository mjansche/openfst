// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <ostream>
#include <string>

#include <fst/arc.h>
#include <fst/script/weight-class.h>

namespace fst {
namespace script {

const char *WeightClass::__ZERO__ = "__ZERO__";
const char *WeightClass::__ONE__ = "__ONE__";
const char *WeightClass::__NOWEIGHT__ = "__NOWEIGHT__";

REGISTER_FST_WEIGHT(StdArc::Weight);
REGISTER_FST_WEIGHT(LogArc::Weight);
REGISTER_FST_WEIGHT(Log64Arc::Weight);

WeightClass::WeightClass(const string &weight_type, const string &weight_str) {
  WeightClassRegister *reg = WeightClassRegister::GetRegister();
  StrToWeightImplBaseT stw = reg->GetEntry(weight_type);
  if (!stw) {
    FSTERROR() << "Unknown weight type: " << weight_type;
    impl_.reset();
    return;
  }
  impl_.reset(stw(weight_str, "WeightClass", 0));
}

const WeightClass WeightClass::Zero(const string &weight_type) {
  const WeightClass zero(weight_type, __ZERO__);
  return zero;
}

const WeightClass WeightClass::One(const string &weight_type) {
  const WeightClass one(weight_type, __ONE__);
  return one;
}

const WeightClass WeightClass::NoWeight(const string &weight_type) {
  const WeightClass one(weight_type, __NOWEIGHT__);
  return one;
}

std::ostream &operator<<(std::ostream &o, const WeightClass &c) {
  c.impl_->Print(&o);
  return o;
}

}  // namespace script
}  // namespace fst
