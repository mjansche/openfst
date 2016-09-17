// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// LogWeight along with sign information that represents the value X in the
// linear domain as <sign(X), -ln(|X|)>
//
// The sign is a TropicalWeight:
//  positive, TropicalWeight.Value() > 0.0, recommended value 1.0
//  negative, TropicalWeight.Value() <= 0.0, recommended value -1.0

#ifndef FST_LIB_SIGNED_LOG_WEIGHT_H_
#define FST_LIB_SIGNED_LOG_WEIGHT_H_

#include <cstdlib>

#include <fst/float-weight.h>
#include <fst/pair-weight.h>
#include <fst/product-weight.h>


namespace fst {
template <class T>
class SignedLogWeightTpl : public PairWeight<TropicalWeight, LogWeightTpl<T>> {
 public:
  using X1 = TropicalWeight;
  using X2 = LogWeightTpl<T>;
  using ReverseWeight = SignedLogWeightTpl;

  using PairWeight<X1, X2>::Value1;
  using PairWeight<X1, X2>::Value2;

  SignedLogWeightTpl() : PairWeight<X1, X2>() {}

  SignedLogWeightTpl(const SignedLogWeightTpl &w) : PairWeight<X1, X2>(w) {}

  explicit SignedLogWeightTpl(const PairWeight<X1, X2> &w)
      : PairWeight<X1, X2>(w) {}

  SignedLogWeightTpl(const X1 &x1, const X2 &x2) : PairWeight<X1, X2>(x1, x2) {}

  static const SignedLogWeightTpl &Zero() {
    static const SignedLogWeightTpl zero(X1(1.0), X2::Zero());
    return zero;
  }

  static const SignedLogWeightTpl &One() {
    static const SignedLogWeightTpl one(X1(1.0), X2::One());
    return one;
  }

  static const SignedLogWeightTpl &NoWeight() {
    static const SignedLogWeightTpl no_weight(X1(1.0), X2::NoWeight());
    return no_weight;
  }

  static const string &Type() {
    static const string type = "signed_log_" + X1::Type() + "_" + X2::Type();
    return type;
  }

  SignedLogWeightTpl Quantize(float delta = kDelta) const {
    return SignedLogWeightTpl(PairWeight<X1, X2>::Quantize(delta));
  }

  ReverseWeight Reverse() const {
    return SignedLogWeightTpl(PairWeight<X1, X2>::Reverse());
  }

  bool Member() const { return PairWeight<X1, X2>::Member(); }

  // Neither idempotent nor path.
  static constexpr uint64 Properties() {
    return kLeftSemiring | kRightSemiring | kCommutative;
  }

  size_t Hash() const {
    size_t h1;
    if (Value2() == X2::Zero() || Value1().Value() > 0.0) {
      h1 = TropicalWeight(1.0).Hash();
    } else {
      h1 = TropicalWeight(-1.0).Hash();
    }
    size_t h2 = Value2().Hash();
    const int lshift = 5;
    const int rshift = CHAR_BIT * sizeof(size_t) - 5;
    return h1 << lshift ^ h1 >> rshift ^ h2;
  }
};

template <class T>
inline SignedLogWeightTpl<T> Plus(const SignedLogWeightTpl<T> &w1,
                                  const SignedLogWeightTpl<T> &w2) {
  typedef TropicalWeight X1;
  typedef LogWeightTpl<T> X2;
  if (!w1.Member() || !w2.Member()) return SignedLogWeightTpl<T>::NoWeight();
  bool s1 = w1.Value1().Value() > 0.0;
  bool s2 = w2.Value1().Value() > 0.0;
  T f1 = w1.Value2().Value();
  T f2 = w2.Value2().Value();
  if (f1 == FloatLimits<T>::PosInfinity()) {
    return w2;
  } else if (f2 == FloatLimits<T>::PosInfinity()) {
    return w1;
  } else if (f1 == f2) {
    if (s1 == s2) {
      return SignedLogWeightTpl<T>(X1(w1.Value1()), X2(f2 - log(2.0F)));
    } else {
      return SignedLogWeightTpl<T>::Zero();
    }
  } else if (f1 > f2) {
    if (s1 == s2) {
      return SignedLogWeightTpl<T>(X1(w1.Value1()),
                                   X2(f2 - log(1.0F + exp(f2 - f1))));
    } else {
      return SignedLogWeightTpl<T>(X1(w2.Value1()),
                                   X2((f2 - log(1.0F - exp(f2 - f1)))));
    }
  } else {
    if (s2 == s1) {
      return SignedLogWeightTpl<T>(X1(w2.Value1()),
                                   X2((f1 - log(1.0F + exp(f1 - f2)))));
    } else {
      return SignedLogWeightTpl<T>(X1(w1.Value1()),
                                   X2((f1 - log(1.0F - exp(f1 - f2)))));
    }
  }
}

template <class T>
inline SignedLogWeightTpl<T> Minus(const SignedLogWeightTpl<T> &w1,
                                   const SignedLogWeightTpl<T> &w2) {
  SignedLogWeightTpl<T> minus_w2(-w2.Value1().Value(), w2.Value2());
  return Plus(w1, minus_w2);
}

template <class T>
inline SignedLogWeightTpl<T> Times(const SignedLogWeightTpl<T> &w1,
                                   const SignedLogWeightTpl<T> &w2) {
  typedef LogWeightTpl<T> X2;
  if (!w1.Member() || !w2.Member()) return SignedLogWeightTpl<T>::NoWeight();
  bool s1 = w1.Value1().Value() > 0.0;
  bool s2 = w2.Value1().Value() > 0.0;
  T f1 = w1.Value2().Value();
  T f2 = w2.Value2().Value();
  if (s1 == s2) {
    return SignedLogWeightTpl<T>(TropicalWeight(1.0), X2(f1 + f2));
  } else {
    return SignedLogWeightTpl<T>(TropicalWeight(-1.0), X2(f1 + f2));
  }
}

template <class T>
inline SignedLogWeightTpl<T> Divide(const SignedLogWeightTpl<T> &w1,
                                    const SignedLogWeightTpl<T> &w2,
                                    DivideType typ = DIVIDE_ANY) {
  typedef LogWeightTpl<T> X2;
  if (!w1.Member() || !w2.Member()) return SignedLogWeightTpl<T>::NoWeight();
  bool s1 = w1.Value1().Value() > 0.0;
  bool s2 = w2.Value1().Value() > 0.0;
  T f1 = w1.Value2().Value();
  T f2 = w2.Value2().Value();
  if (f2 == FloatLimits<T>::PosInfinity()) {
    return SignedLogWeightTpl<T>(TropicalWeight(1.0),
                                 X2(FloatLimits<T>::NumberBad()));
  } else if (f1 == FloatLimits<T>::PosInfinity()) {
    return SignedLogWeightTpl<T>(TropicalWeight(1.0),
                                 X2(FloatLimits<T>::PosInfinity()));
  } else if (s1 == s2) {
    return SignedLogWeightTpl<T>(TropicalWeight(1.0), X2(f1 - f2));
  } else {
    return SignedLogWeightTpl<T>(TropicalWeight(-1.0), X2(f1 - f2));
  }
}

template <class T>
inline bool ApproxEqual(const SignedLogWeightTpl<T> &w1,
                        const SignedLogWeightTpl<T> &w2, float delta = kDelta) {
  bool s1 = w1.Value1().Value() > 0.0;
  bool s2 = w2.Value1().Value() > 0.0;
  if (s1 == s2) {
    return ApproxEqual(w1.Value2(), w2.Value2(), delta);
  } else {
    return w1.Value2() == LogWeightTpl<T>::Zero() &&
           w2.Value2() == LogWeightTpl<T>::Zero();
  }
}

template <class T>
inline bool operator==(const SignedLogWeightTpl<T> &w1,
                       const SignedLogWeightTpl<T> &w2) {
  bool s1 = w1.Value1().Value() > 0.0;
  bool s2 = w2.Value1().Value() > 0.0;
  if (s1 == s2) {
    return w1.Value2() == w2.Value2();
  } else {
    return (w1.Value2() == LogWeightTpl<T>::Zero()) &&
           (w2.Value2() == LogWeightTpl<T>::Zero());
  }
}

// Single-precision signed-log weight
typedef SignedLogWeightTpl<float> SignedLogWeight;
// Double-precision signed-log weight
typedef SignedLogWeightTpl<double> SignedLog64Weight;

template <class W1, class W2>
bool SignedLogConvertCheck(W1 w) {
  if (w.Value1().Value() < 0.0) {
    FSTERROR() << "WeightConvert: Can't convert weight from " << W1::Type()
               << " to " << W2::Type();
    return false;
  }
  return true;
}

// Converts to tropical.
template <>
struct WeightConvert<SignedLogWeight, TropicalWeight> {
  TropicalWeight operator()(const SignedLogWeight &w) const {
    if (!SignedLogConvertCheck<SignedLogWeight, TropicalWeight>(w)) {
      return TropicalWeight::NoWeight();
    }
    return TropicalWeight(w.Value2().Value());
  }
};

template <>
struct WeightConvert<SignedLog64Weight, TropicalWeight> {
  TropicalWeight operator()(const SignedLog64Weight &w) const {
    if (!SignedLogConvertCheck<SignedLog64Weight, TropicalWeight>(w)) {
      return TropicalWeight::NoWeight();
    }
    return TropicalWeight(w.Value2().Value());
  }
};

// Converts to log.
template <>
struct WeightConvert<SignedLogWeight, LogWeight> {
  LogWeight operator()(const SignedLogWeight &w) const {
    if (!SignedLogConvertCheck<SignedLogWeight, LogWeight>(w)) {
      return LogWeight::NoWeight();
    }
    return LogWeight(w.Value2().Value());
  }
};

template <>
struct WeightConvert<SignedLog64Weight, LogWeight> {
  LogWeight operator()(const SignedLog64Weight &w) const {
    if (!SignedLogConvertCheck<SignedLog64Weight, LogWeight>(w)) {
      return LogWeight::NoWeight();
    }
    return LogWeight(w.Value2().Value());
  }
};

// Converts to log64.
template <>
struct WeightConvert<SignedLogWeight, Log64Weight> {
  Log64Weight operator()(const SignedLogWeight &w) const {
    if (!SignedLogConvertCheck<SignedLogWeight, Log64Weight>(w)) {
      return Log64Weight::NoWeight();
    }
    return Log64Weight(w.Value2().Value());
  }
};

template <>
struct WeightConvert<SignedLog64Weight, Log64Weight> {
  Log64Weight operator()(const SignedLog64Weight &w) const {
    if (!SignedLogConvertCheck<SignedLog64Weight, Log64Weight>(w)) {
      return Log64Weight::NoWeight();
    }
    return Log64Weight(w.Value2().Value());
  }
};

// Converts to signed log.
template <>
struct WeightConvert<TropicalWeight, SignedLogWeight> {
  SignedLogWeight operator()(const TropicalWeight &w) const {
    const TropicalWeight x1(1.0);
    const LogWeight x2(w.Value());
    return SignedLogWeight(x1, x2);
  }
};

template <>
struct WeightConvert<LogWeight, SignedLogWeight> {
  SignedLogWeight operator()(const LogWeight &w) const {
    const TropicalWeight x1(1.0);
    const LogWeight x2(w.Value());
    return SignedLogWeight(x1, x2);
  }
};

template <>
struct WeightConvert<Log64Weight, SignedLogWeight> {
  SignedLogWeight operator()(const Log64Weight &w) const {
    const TropicalWeight x1(1.0);
    const LogWeight x2(w.Value());
    return SignedLogWeight(x1, x2);
  }
};

template <>
struct WeightConvert<SignedLog64Weight, SignedLogWeight> {
  SignedLogWeight operator()(const SignedLog64Weight &w) const {
    const LogWeight x2(w.Value2().Value());
    return SignedLogWeight(w.Value1(), x2);
  }
};

// Converts to signed log64.
template <>
struct WeightConvert<TropicalWeight, SignedLog64Weight> {
  SignedLog64Weight operator()(const TropicalWeight &w) const {
    const TropicalWeight x1(1.0);
    const Log64Weight x2(w.Value());
    return SignedLog64Weight(x1, x2);
  }
};

template <>
struct WeightConvert<LogWeight, SignedLog64Weight> {
  SignedLog64Weight operator()(const LogWeight &w) const {
    const TropicalWeight x1(1.0);
    const Log64Weight x2(w.Value());
    return SignedLog64Weight(x1, x2);
  }
};

template <>
struct WeightConvert<Log64Weight, SignedLog64Weight> {
  SignedLog64Weight operator()(const Log64Weight &w) const {
    const TropicalWeight x1(1.0);
    const Log64Weight x2(w.Value());
    return SignedLog64Weight(x1, x2);
  }
};

template <>
struct WeightConvert<SignedLogWeight, SignedLog64Weight> {
  SignedLog64Weight operator()(const SignedLogWeight &w) const {
    const Log64Weight x2(w.Value2().Value());
    return SignedLog64Weight(w.Value1(), x2);
  }
};

// This function object returns SignedLogWeightTpl<T>'s that are random integers
// chosen from [0, num_random_weights) times a random sign. This is intended
// primarily for testing.
template <class T>
class WeightGenerate<SignedLogWeightTpl<T>> {
 public:
  using Weight = SignedLogWeightTpl<T>;
  using X1 = typename Weight::X1;
  using X2 = typename Weight::X2;

  explicit WeightGenerate(bool allow_zero = true,
                          size_t num_random_weights = kNumRandomWeights)
    : allow_zero_(allow_zero), num_random_weights_(num_random_weights) {}

  Weight operator()() const {
    static const X1 negative_one(-1.0);
    static const X1 positive_one(+1.0);
    int m = rand() % 2;                                    // NOLINT
    int n = rand() % (num_random_weights_ + allow_zero_);  // NOLINT
    return Weight((m == 0) ? negative_one : positive_one,
                  (allow_zero_ && n == num_random_weights_) ?
                   X2::Zero() : X2(n));
  }

 private:
  // Permits Zero() and zero divisors.
  bool allow_zero_;
  // Number of alternative random weights.
  const size_t num_random_weights_;
};

/*
template <class T>
const typename WeightGenerate<SignedLogWeightTpl<T>>::X1 negative_one;

template <class T>
const typename WeightGenerate<SignedLogWeightTpl<T>>::X1 positive_one;
*/

}  // namespace fst

#endif  // FST_LIB_SIGNED_LOG_WEIGHT_H_
