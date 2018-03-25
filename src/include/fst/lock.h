// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Google-compatibility locking declarations and inline definitions.

#ifndef FST_LIB_LOCK_H_
#define FST_LIB_LOCK_H_

#include <mutex>

#include <fst/compat.h>  // for DISALLOW_COPY_AND_ASSIGN

namespace fst {

using namespace std;

class Mutex {
 public:
  Mutex() {}

  inline void Lock() { mu_.lock(); }

  inline void Unlock() { mu_.unlock(); }

 private:
  std::mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(Mutex);
};

class MutexLock {
 public:
  explicit MutexLock(Mutex *mu) : mu_(mu) { mu_->Lock(); }

  ~MutexLock() { mu_->Unlock(); }

 private:
  Mutex *mu_;

  DISALLOW_COPY_AND_ASSIGN(MutexLock);
};

// Currently, we don't use a separate reader lock.
// TODO(kbg): Implement this with std::shared_mutex once C++17 becomes widely
// available.
using ReaderMutexLock = MutexLock;

}  // namespace fst

#endif  // FST_LIB_LOCK_H_
