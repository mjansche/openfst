// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Pair weight templated base class for weight classes that contain two weights
// (e.g. Product, Lexicographic).

#ifndef FST_LIB_PAIR_WEIGHT_H_
#define FST_LIB_PAIR_WEIGHT_H_

#include <climits>
#include <stack>
#include <string>
#include <utility>

#include <fst/weight.h>


namespace fst {

template <class W1, class W2>
class PairWeight {
 public:
  typedef W1 Weight1;
  typedef W2 Weight2;
  typedef PairWeight<typename W1::ReverseWeight, typename W2::ReverseWeight>
      ReverseWeight;

  PairWeight() {}

  PairWeight(const PairWeight &w) : value1_(w.value1_), value2_(w.value2_) {}

  PairWeight(W1 w1, W2 w2) : value1_(std::move(w1)), value2_(std::move(w2)) {}

  static const PairWeight<W1, W2> &Zero() {
    static const PairWeight<W1, W2> zero(W1::Zero(), W2::Zero());
    return zero;
  }

  static const PairWeight<W1, W2> &One() {
    static const PairWeight<W1, W2> one(W1::One(), W2::One());
    return one;
  }

  static const PairWeight<W1, W2> &NoWeight() {
    static const PairWeight<W1, W2> no_weight(W1::NoWeight(), W2::NoWeight());
    return no_weight;
  }

  std::istream &Read(std::istream &strm) {
    value1_.Read(strm);
    return value2_.Read(strm);
  }

  std::ostream &Write(std::ostream &strm) const {
    value1_.Write(strm);
    return value2_.Write(strm);
  }

  PairWeight<W1, W2> &operator=(const PairWeight<W1, W2> &w) {
    value1_ = w.Value1();
    value2_ = w.Value2();
    return *this;
  }

  bool Member() const { return value1_.Member() && value2_.Member(); }

  size_t Hash() const {
    size_t h1 = value1_.Hash();
    size_t h2 = value2_.Hash();
    const int lshift = 5;
    const int rshift = CHAR_BIT * sizeof(size_t) - 5;
    return h1 << lshift ^ h1 >> rshift ^ h2;
  }

  PairWeight<W1, W2> Quantize(float delta = kDelta) const {
    return PairWeight<W1, W2>(value1_.Quantize(delta), value2_.Quantize(delta));
  }

  ReverseWeight Reverse() const {
    return ReverseWeight(value1_.Reverse(), value2_.Reverse());
  }

  const W1 &Value1() const { return value1_; }

  const W2 &Value2() const { return value2_; }

  void SetValue1(const W1 &w) { value1_ = w; }
  void SetValue2(const W2 &w) { value2_ = w; }

 private:
  W1 value1_;
  W2 value2_;
};

template <class W1, class W2>
inline bool operator==(const PairWeight<W1, W2> &w,
                       const PairWeight<W1, W2> &v) {
  return w.Value1() == v.Value1() && w.Value2() == v.Value2();
}

template <class W1, class W2>
inline bool operator!=(const PairWeight<W1, W2> &w1,
                       const PairWeight<W1, W2> &w2) {
  return w1.Value1() != w2.Value1() || w1.Value2() != w2.Value2();
}

template <class W1, class W2>
inline bool ApproxEqual(const PairWeight<W1, W2> &w1,
                        const PairWeight<W1, W2> &w2, float delta = kDelta) {
  return ApproxEqual(w1.Value1(), w2.Value1(), delta) &&
         ApproxEqual(w1.Value2(), w2.Value2(), delta);
}

template <class W1, class W2>
inline std::ostream &operator<<(std::ostream &strm,
                                const PairWeight<W1, W2> &w) {
  CompositeWeightWriter writer(strm);
  writer.WriteBegin();
  writer.WriteElement(w.Value1());
  writer.WriteElement(w.Value2());
  writer.WriteEnd();
  return strm;
}

template <class W1, class W2>
inline std::istream &operator>>(std::istream &strm, PairWeight<W1, W2> &w) {
  CompositeWeightReader reader(strm);
  reader.ReadBegin();
  W1 w1;
  reader.ReadElement(&w1);
  w.SetValue1(w1);

  W2 w2;
  reader.ReadElement(&w2, true);
  w.SetValue2(w2);
  reader.ReadEnd();

  return strm;
}

// This function object returns weights by calling the underlying generators
// and forming a pair. This is intended primarily for testing.
template <class W1, class W2>
class WeightGenerate<PairWeight<W1, W2>> {
 public:
  using Weight = PairWeight<W1, W2>;
  using Generate1 = WeightGenerate<W1>;
  using Generate2 = WeightGenerate<W2>;

  explicit WeightGenerate(bool allow_zero = true)
      : generate1_(allow_zero), generate2_(allow_zero) {}

  Weight operator()() const { return Weight(generate1_(), generate2_()); }

 private:
  Generate1 generate1_;
  Generate2 generate2_;
};

}  // namespace fst

#endif  // FST_LIB_PAIR_WEIGHT_H_
