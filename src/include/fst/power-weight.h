// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Cartesian power weight semiring operation definitions.

#ifndef FST_LIB_POWER_WEIGHT_H__
#define FST_LIB_POWER_WEIGHT_H__

#include <fst/tuple-weight.h>
#include <fst/weight.h>


namespace fst {

// Cartesian power semiring: W ^ n
// Forms:
//  - a left semimodule when W is a left semiring,
//  - a right semimodule when W is a right semiring,
//  - a bisemimodule when W is a semiring,
//    the free semimodule of rank n over W
// The Times operation is overloaded to provide the
// left and right scalar products.
template <class W, unsigned int n>
class PowerWeight : public TupleWeight<W, n> {
 public:
  using TupleWeight<W, n>::Zero;
  using TupleWeight<W, n>::One;
  using TupleWeight<W, n>::NoWeight;
  using TupleWeight<W, n>::Quantize;
  using TupleWeight<W, n>::Reverse;

  typedef PowerWeight<typename W::ReverseWeight, n> ReverseWeight;

  PowerWeight() {}

  PowerWeight(const TupleWeight<W, n> &w) : TupleWeight<W, n>(w) {}

  template <class Iterator>
  PowerWeight(Iterator begin, Iterator end) : TupleWeight<W, n>(begin, end) {}

  static const PowerWeight<W, n> &Zero() {
    static const PowerWeight<W, n> zero(TupleWeight<W, n>::Zero());
    return zero;
  }

  static const PowerWeight<W, n> &One() {
    static const PowerWeight<W, n> one(TupleWeight<W, n>::One());
    return one;
  }

  static const PowerWeight<W, n> &NoWeight() {
    static const PowerWeight<W, n> no_weight(TupleWeight<W, n>::NoWeight());
    return no_weight;
  }

  static const string &Type() {
    static string type;
    if (type.empty()) {
      string power;
      Int64ToStr(n, &power);
      type = W::Type() + "_^" + power;
    }
    return type;
  }

  static uint64 Properties() {
    uint64 props = W::Properties();
    return props &
           (kLeftSemiring | kRightSemiring | kCommutative | kIdempotent);
  }

  PowerWeight<W, n> Quantize(float delta = kDelta) const {
    return TupleWeight<W, n>::Quantize(delta);
  }

  ReverseWeight Reverse() const { return TupleWeight<W, n>::Reverse(); }
};

// Semiring plus operation
template <class W, unsigned int n>
inline PowerWeight<W, n> Plus(const PowerWeight<W, n> &w1,
                              const PowerWeight<W, n> &w2) {
  PowerWeight<W, n> w;
  for (size_t i = 0; i < n; ++i) w.SetValue(i, Plus(w1.Value(i), w2.Value(i)));
  return w;
}

// Semiring times operation
template <class W, unsigned int n>
inline PowerWeight<W, n> Times(const PowerWeight<W, n> &w1,
                               const PowerWeight<W, n> &w2) {
  PowerWeight<W, n> w;
  for (size_t i = 0; i < n; ++i) w.SetValue(i, Times(w1.Value(i), w2.Value(i)));
  return w;
}

// Semiring divide operation
template <class W, unsigned int n>
inline PowerWeight<W, n> Divide(const PowerWeight<W, n> &w1,
                                const PowerWeight<W, n> &w2,
                                DivideType type = DIVIDE_ANY) {
  PowerWeight<W, n> w;
  for (size_t i = 0; i < n; ++i)
    w.SetValue(i, Divide(w1.Value(i), w2.Value(i), type));
  return w;
}

// Semimodule left scalar product
template <class W, unsigned int n>
inline PowerWeight<W, n> Times(const W &s, const PowerWeight<W, n> &w) {
  PowerWeight<W, n> sw;
  for (size_t i = 0; i < n; ++i) sw.SetValue(i, Times(s, w.Value(i)));
  return sw;
}

// Semimodule right scalar product
template <class W, unsigned int n>
inline PowerWeight<W, n> Times(const PowerWeight<W, n> &w, const W &s) {
  PowerWeight<W, n> ws;
  for (size_t i = 0; i < n; ++i) ws.SetValue(i, Times(w.Value(i), s));
  return ws;
}

// Semimodule dot product
template <class W, unsigned int n>
inline W DotProduct(const PowerWeight<W, n> &w1, const PowerWeight<W, n> &w2) {
  W w = W::Zero();
  for (size_t i = 0; i < n; ++i) w = Plus(w, Times(w1.Value(i), w2.Value(i)));
  return w;
}

}  // namespace fst

#endif  // FST_LIB_POWER_WEIGHT_H__
