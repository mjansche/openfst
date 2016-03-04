// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Represents a generic weight in an FST; that is, represents a specific type
// of weight underneath while hiding that type from a client.

#ifndef FST_SCRIPT_WEIGHT_CLASS_H_
#define FST_SCRIPT_WEIGHT_CLASS_H_

#include <memory>
#include <ostream>
#include <string>

#include <fst/generic-register.h>
#include <fst/util.h>

namespace fst {
namespace script {

class WeightImplBase {
 public:
  virtual WeightImplBase *Copy() const = 0;
  virtual void Print(std::ostream *o) const = 0;
  virtual const string &Type() const = 0;
  virtual string ToString() const = 0;
  virtual bool operator==(const WeightImplBase &other) const = 0;
  virtual ~WeightImplBase() {}
};

template <class W>
struct WeightClassImpl : public WeightImplBase {
  W weight;

  explicit WeightClassImpl(const W &weight) : weight(weight) {}

  WeightClassImpl<W> *Copy() const override {
    return new WeightClassImpl<W>(weight);
  }

  const string &Type() const override { return W::Type(); }

  void Print(std::ostream *o) const override { *o << weight; }

  string ToString() const override {
    string str;
    WeightToStr(weight, &str);
    return str;
  }

  bool operator==(const WeightImplBase &other) const override {
    if (Type() != other.Type()) {
      return false;
    } else {
      const WeightClassImpl<W> *typed_other =
          static_cast<const WeightClassImpl<W> *>(&other);
      return typed_other->weight == weight;
    }
  }
};

class WeightClass {
 public:
  WeightClass() = default;

  template <class W>
  explicit WeightClass(const W &weight)
      : impl_(new WeightClassImpl<W>(weight)) {}

  WeightClass(const string &weight_type, const string &weight_str);

  WeightClass(const WeightClass &other)
      : impl_(other.impl_ ? other.impl_->Copy() : nullptr) {}

  WeightClass &operator=(const WeightClass &other) {
    impl_.reset(other.impl_ ? other.impl_->Copy() : nullptr);
    return *this;
  }

  // The statics are defined in the .cc.

  static const char *__ZERO__;  // NOLINT

  static const WeightClass Zero(const string &weight_type);

  static const char *__ONE__;  // NOLINT

  static const WeightClass One(const string &weight_type);

  static const char *__NOWEIGHT__;  // NOLINT

  static const WeightClass NoWeight(const string &weight_type);

  template <class W>
  const W *GetWeight() const;

  string ToString() const { return (impl_) ? impl_->ToString() : "none"; }

  bool operator==(const WeightClass &other) const {
    return ((impl_ && other.impl_ && (*impl_ == *other.impl_)) ||
            (impl_ == nullptr && other.impl_ == nullptr));
  }

  const string &Type() const {
    if (impl_) return impl_->Type();
    static const string no_type = "none";
    return no_type;
  }

 private:
  std::unique_ptr<WeightImplBase> impl_;

  friend std::ostream &operator<<(std::ostream &o, const WeightClass &c);
};

template <class W>
const W *WeightClass::GetWeight() const {
  if (W::Type() != impl_->Type()) {
     return nullptr;
  } else {
    auto *typed_impl = static_cast<WeightClassImpl<W> *>(impl_.get());
    return &typed_impl->weight;
  }
}

// Registration for generic weight types.

typedef WeightImplBase *(*StrToWeightImplBaseT)(const string &str,
                                                const string &src,
                                                size_t nline);

template <class W>
WeightImplBase *StrToWeightImplBase(const string &str, const string &src,
                                    size_t nline) {
  if (str == WeightClass::__ZERO__)
    return new WeightClassImpl<W>(W::Zero());
  else if (str == WeightClass::__ONE__)
    return new WeightClassImpl<W>(W::One());
  else if (str == WeightClass::__NOWEIGHT__)
    return new WeightClassImpl<W>(W::NoWeight());
  return new WeightClassImpl<W>(StrToWeight<W>(str, src, nline));
}

std::ostream &operator<<(std::ostream &o, const WeightClass &c);

class WeightClassRegister : public GenericRegister<string, StrToWeightImplBaseT,
                                                   WeightClassRegister> {
 protected:
  string ConvertKeyToSoFilename(const string &key) const override {
    return key + ".so";
  }
};

typedef GenericRegisterer<WeightClassRegister> WeightClassRegisterer;

// Internal version; needs to be called by wrapper in order for macro args to
// expand.
#define REGISTER_FST_WEIGHT__(Weight, line)                \
  static WeightClassRegisterer weight_registerer##_##line( \
      Weight::Type(), StrToWeightImplBase<Weight>)

// This layer is where __FILE__ and __LINE__ are expanded.
#define REGISTER_FST_WEIGHT_EXPANDER(Weight, line) \
  REGISTER_FST_WEIGHT__(Weight, line)

// Macro for registering new weight types. Clients call this.
#define REGISTER_FST_WEIGHT(Weight) \
  REGISTER_FST_WEIGHT_EXPANDER(Weight, __LINE__)

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_WEIGHT_CLASS_H_
