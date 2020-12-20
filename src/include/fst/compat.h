// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_LIB_COMPAT_H_
#define FST_LIB_COMPAT_H_

#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// Makes copy constructor and operator= private
#define DISALLOW_COPY_AND_ASSIGN(type)    \
  type(const type&);                      \
  void operator=(const type&)

#include <fst/config.h>
#include <fst/types.h>
#include <fst/lock.h>
#include <fst/flags.h>
#include <fst/log.h>
#include <fst/icu.h>

using std::string;

void FailedNewHandler();

namespace fst {

// Downcasting
template<typename To, typename From>
inline To down_cast(From* f) {
  return static_cast<To>(f);
}

// Bitcasting
template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  // Compile time assertion: sizeof(Dest) == sizeof(Source)
  // A compile error here means your Dest and Source have different sizes.
  typedef char VerifySizesAreEqual [sizeof(Dest) == sizeof(Source) ? 1 :
                                    -1];
  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}

// Check sums
class CheckSummer {
 public:
  CheckSummer() : count_(0) {
    check_sum_.resize(kCheckSumLength, '\0');
  }

  void Reset() {
    count_ = 0;
    for (int i = 0; i < kCheckSumLength; ++i)
      check_sum_[i] = '\0';
  }

  void Update(void const *data, int size) {
    const char *p = reinterpret_cast<const char *>(data);
    for (int i = 0; i < size; ++i)
      check_sum_[(count_++) % kCheckSumLength] ^= p[i];
  }

  void Update(string const &data) {
    for (int i = 0; i < data.size(); ++i)
      check_sum_[(count_++) % kCheckSumLength] ^= data[i];
  }

  string Digest() {
    return check_sum_;
  }

 private:
  static const int kCheckSumLength = 32;
  int count_;
  string check_sum_;

  DISALLOW_COPY_AND_ASSIGN(CheckSummer);
};

}  // namespace fst

#endif  // FST_LIB_COMPAT_H_
