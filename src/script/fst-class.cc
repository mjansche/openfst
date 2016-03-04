// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// These classes are only recommended for use in high-level scripting
// applications. Most users should use the lower-level templated versions
// corresponding to these classes.

#include <istream>

#include <fst/equal.h>
#include <fst/fst-decl.h>
#include <fst/reverse.h>
#include <fst/union.h>
#include <fst/script/fst-class.h>
#include <fst/script/register.h>

namespace fst {
namespace script {

// REGISTRATION

REGISTER_FST_CLASSES(StdArc);
REGISTER_FST_CLASSES(LogArc);
REGISTER_FST_CLASSES(Log64Arc);

// FST CLASS METHODS

template <class FstT>
FstT *ReadFst(std::istream &in, const string &fname) {
  if (!in) {
    LOG(ERROR) << "ReadFst: Can't open file: " << fname;
    return nullptr;
  }

  FstHeader hdr;
  if (!hdr.Read(in, fname)) {
    return nullptr;
  }

  FstReadOptions read_options(fname, &hdr);

  typename IORegistration<FstT>::Register *reg =
      IORegistration<FstT>::Register::GetRegister();

  const typename IORegistration<FstT>::Reader reader =
      reg->GetReader(hdr.ArcType());

  if (!reader) {
    LOG(ERROR) << "ReadFst: Unknown arc type: " << hdr.ArcType();
    return nullptr;
  }

  return reader(in, read_options);
}

FstClass *FstClass::Read(const string &fname) {
  if (!fname.empty()) {
    std::ifstream in(fname.c_str(),
                          std::ios_base::in | std::ios_base::binary);
    return ReadFst<FstClass>(in, fname);
  } else {
    return ReadFst<FstClass>(std::cin, "standard input");
  }
}

FstClass *FstClass::Read(std::istream &istr, const string &source) {
  return ReadFst<FstClass>(istr, source);
}

bool FstClass::WeightTypesMatch(const WeightClass &w,
                                const string &op_name) const {
  if (WeightType() != w.Type()) {
    FSTERROR() << "FST and weight with non-matching weight types passed to "
               << op_name << ": " << WeightType() << " and " << w.Type();
    return false;
  }
  return true;
}

// MUTABLE FST CLASS METHODS

MutableFstClass *MutableFstClass::Read(const string &fname, bool convert) {
  if (convert == false) {
    if (!fname.empty()) {
      std::ifstream in(fname.c_str(),
                            std::ios_base::in | std::ios_base::binary);
      return ReadFst<MutableFstClass>(in, fname);
    } else {
      return ReadFst<MutableFstClass>(std::cin, "standard input");
    }
  } else {  // Converts to VectorFstClass if not mutable.
    FstClass *ifst = FstClass::Read(fname);
    if (!ifst) return nullptr;
    if (ifst->Properties(fst::kMutable, false)) {
      return static_cast<MutableFstClass *>(ifst);
    } else {
      MutableFstClass *ofst = new VectorFstClass(*ifst);
      delete ifst;
      return ofst;
    }
  }
}

// VECTOR FST CLASS METHODS

VectorFstClass *VectorFstClass::Read(const string &fname) {
  if (!fname.empty()) {
    std::ifstream in(fname.c_str(),
                          std::ios_base::in | std::ios_base::binary);
    return ReadFst<VectorFstClass>(in, fname);
  } else {
    return ReadFst<VectorFstClass>(std::cin, "standard input");
  }
}

IORegistration<VectorFstClass>::Entry GetVFSTRegisterEntry(
    const string &arc_type) {
  IORegistration<VectorFstClass>::Register *reg =
      IORegistration<VectorFstClass>::Register::GetRegister();
  const IORegistration<VectorFstClass>::Entry &entry = reg->GetEntry(arc_type);
  return entry;
}

VectorFstClass::VectorFstClass(const string &arc_type)
    : MutableFstClass(GetVFSTRegisterEntry(arc_type).creator()) {
  if (Properties(kError, true) & kError)
    FSTERROR() << "VectorFstClass: Unknown arc type: " << arc_type;
}

VectorFstClass::VectorFstClass(const FstClass &other)
    : MutableFstClass(GetVFSTRegisterEntry(other.ArcType()).converter(other)) {}

}  // namespace script
}  // namespace fst
