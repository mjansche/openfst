// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Classes for registering derived FST for generic reading.

#ifndef FST_LIB_REGISTER_H_
#define FST_LIB_REGISTER_H_

#include <string>


#include <fst/compat.h>
#include <fst/generic-register.h>
#include <fst/util.h>


#include <fst/types.h>

namespace fst {

template <class A>
class Fst;
struct FstReadOptions;

// This class represents a single entry in a FstRegister
template <class A>
struct FstRegisterEntry {
  typedef Fst<A> *(*Reader)(std::istream &strm, const FstReadOptions &opts);
  typedef Fst<A> *(*Converter)(const Fst<A> &fst);

  Reader reader;
  Converter converter;
  FstRegisterEntry() : reader(nullptr), converter(nullptr) {}
  FstRegisterEntry(Reader r, Converter c) : reader(r), converter(c) {}
};

// This class maintains the correspondence between a string describing
// an FST type, and its reader and converter.
template <class A>
class FstRegister
    : public GenericRegister<string, FstRegisterEntry<A>, FstRegister<A>> {
 public:
  typedef typename FstRegisterEntry<A>::Reader Reader;
  typedef typename FstRegisterEntry<A>::Converter Converter;

  const Reader GetReader(const string &type) const {
    return this->GetEntry(type).reader;
  }

  const Converter GetConverter(const string &type) const {
    return this->GetEntry(type).converter;
  }

 protected:
  string ConvertKeyToSoFilename(const string &key) const override {
    string legal_type(key);

    ConvertToLegalCSymbol(&legal_type);

    return legal_type + "-fst.so";
  }
};

// This class registers an Fst type for generic reading and creating.
// The Fst type must have a default constructor and a copy constructor
// from 'Fst<Arc>' for this to work.
template <class F>
class FstRegisterer : public GenericRegisterer<FstRegister<typename F::Arc>> {
 public:
  typedef typename F::Arc Arc;
  typedef typename FstRegister<Arc>::Entry Entry;
  typedef typename FstRegister<Arc>::Reader Reader;

  FstRegisterer()
      : GenericRegisterer<FstRegister<typename F::Arc>>(F().Type(),
                                                         BuildEntry()) {}

 private:
  static Entry BuildEntry() {
    F *(*reader)(std::istream &strm, const FstReadOptions &opts) = &F::Read;

    return Entry(reinterpret_cast<Reader>(reader), &FstRegisterer<F>::Convert);
  }

  static Fst<Arc> *Convert(const Fst<Arc> &fst) { return new F(fst); }
};

// Convenience macro to generate static FstRegisterer instance.
#define REGISTER_FST(F, A) \
  static fst::FstRegisterer<F<A>> F##_##A##_registerer

// Converts an fst to type 'type'.
template <class A>
Fst<A> *Convert(const Fst<A> &fst, const string &ftype) {
  FstRegister<A> *registr = FstRegister<A>::GetRegister();
  const typename FstRegister<A>::Converter converter =
      registr->GetConverter(ftype);
  if (!converter) {
    string atype = A::Type();
    FSTERROR() << "Fst::Convert: Unknown FST type " << ftype
               << " (arc type " << atype << ")";
    return nullptr;
  }
  return converter(fst);
}

}  // namespace fst

#endif  // FST_LIB_REGISTER_H_
