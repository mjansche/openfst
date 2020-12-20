// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

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
  const WeightClass noweight(weight_type, __NOWEIGHT__);
  return noweight;
}

bool WeightClass::WeightTypesMatch(const WeightClass &other,
                                   const string &op_name) const {
  if (Type() != other.Type()) {
    FSTERROR() << "Weights with non-matching types passed to " << op_name
               << ": " << Type() << " and " << other.Type();
    return false;
  }
  return true;
}

bool operator==(const WeightClass &lhs, const WeightClass &rhs) {
  if (!(lhs.GetImpl() && rhs.GetImpl() &&
        lhs.WeightTypesMatch(rhs, "operator==")))
    return false;
  return *lhs.GetImpl() == *rhs.GetImpl();
}

bool operator!=(const WeightClass &lhs, const WeightClass &rhs) {
  return !(lhs == rhs);
}

WeightClass Plus(const WeightClass &lhs, const WeightClass &rhs) {
  if (!(lhs.GetImpl() && rhs.GetImpl() && lhs.WeightTypesMatch(rhs, "Plus")))
    return WeightClass();
  WeightClass result(lhs);
  result.GetImpl()->PlusEq(*rhs.GetImpl());
  return result;
}

WeightClass Times(const WeightClass &lhs, const WeightClass &rhs) {
  if (!(lhs.GetImpl() && rhs.GetImpl() && lhs.WeightTypesMatch(rhs, "Times")))
    return WeightClass();
  WeightClass result(lhs);
  result.GetImpl()->TimesEq(*rhs.GetImpl());
  return result;
}

WeightClass Divide(const WeightClass &lhs, const WeightClass &rhs) {
  if (!(lhs.GetImpl() && rhs.GetImpl() && lhs.WeightTypesMatch(rhs, "Divide")))
    return WeightClass();
  WeightClass result(lhs);
  result.GetImpl()->DivideEq(*rhs.GetImpl());
  return result;
}

WeightClass Power(const WeightClass &w, size_t n) {
  if (!w.GetImpl()) return WeightClass();
  WeightClass result(w);
  result.GetImpl()->PowerEq(n);
  return result;
}

std::ostream &operator<<(std::ostream &o, const WeightClass &c) {
  c.impl_->Print(&o);
  return o;
}

}  // namespace script
}  // namespace fst
