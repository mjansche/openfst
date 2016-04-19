// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Product weight set and associated semiring operation definitions.

#ifndef FST_LIB_PRODUCT_WEIGHT_H_
#define FST_LIB_PRODUCT_WEIGHT_H_

#include <stack>
#include <string>

#include <fst/pair-weight.h>
#include <fst/weight.h>


namespace fst {

// Product semiring: W1 * W2
template <class W1, class W2>
class ProductWeight : public PairWeight<W1, W2> {
 public:
  using PairWeight<W1, W2>::Zero;
  using PairWeight<W1, W2>::One;
  using PairWeight<W1, W2>::NoWeight;
  using PairWeight<W1, W2>::Quantize;
  using PairWeight<W1, W2>::Reverse;

  typedef ProductWeight<typename W1::ReverseWeight, typename W2::ReverseWeight>
      ReverseWeight;

  ProductWeight() {}

  ProductWeight(const PairWeight<W1, W2> &w) : PairWeight<W1, W2>(w) {}

  ProductWeight(W1 w1, W2 w2) : PairWeight<W1, W2>(w1, w2) {}

  static const ProductWeight<W1, W2> &Zero() {
    static const ProductWeight<W1, W2> zero(PairWeight<W1, W2>::Zero());
    return zero;
  }

  static const ProductWeight<W1, W2> &One() {
    static const ProductWeight<W1, W2> one(PairWeight<W1, W2>::One());
    return one;
  }

  static const ProductWeight<W1, W2> &NoWeight() {
    static const ProductWeight<W1, W2> no_weight(
        PairWeight<W1, W2>::NoWeight());
    return no_weight;
  }

  static const string &Type() {
    static const string type = W1::Type() + "_X_" + W2::Type();
    return type;
  }

  static uint64 Properties() {
    uint64 props1 = W1::Properties();
    uint64 props2 = W2::Properties();
    return props1 & props2 &
           (kLeftSemiring | kRightSemiring | kCommutative | kIdempotent);
  }

  ProductWeight<W1, W2> Quantize(float delta = kDelta) const {
    return PairWeight<W1, W2>::Quantize(delta);
  }

  ReverseWeight Reverse() const { return PairWeight<W1, W2>::Reverse(); }
};

template <class W1, class W2>
inline ProductWeight<W1, W2> Plus(const ProductWeight<W1, W2> &w,
                                  const ProductWeight<W1, W2> &v) {
  return ProductWeight<W1, W2>(Plus(w.Value1(), v.Value1()),
                               Plus(w.Value2(), v.Value2()));
}

template <class W1, class W2>
inline ProductWeight<W1, W2> Times(const ProductWeight<W1, W2> &w,
                                   const ProductWeight<W1, W2> &v) {
  return ProductWeight<W1, W2>(Times(w.Value1(), v.Value1()),
                               Times(w.Value2(), v.Value2()));
}

template <class W1, class W2>
inline ProductWeight<W1, W2> Divide(const ProductWeight<W1, W2> &w,
                                    const ProductWeight<W1, W2> &v,
                                    DivideType typ = DIVIDE_ANY) {
  return ProductWeight<W1, W2>(Divide(w.Value1(), v.Value1(), typ),
                               Divide(w.Value2(), v.Value2(), typ));
}

}  // namespace fst

#endif  // FST_LIB_PRODUCT_WEIGHT_H_
