// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Generic FST augmented with state count-interface class definition.

#ifndef FST_LIB_EXPANDED_FST_H_
#define FST_LIB_EXPANDED_FST_H_

#include <sys/types.h>
#include <istream>
#include <string>

#include <fstream>

#include <fst/fst.h>


namespace fst {

// A generic FST plus state count.
template <class A>
class ExpandedFst : public Fst<A> {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;

  virtual StateId NumStates() const = 0;  // State count

  // Get a copy of this ExpandedFst. See Fst<>::Copy() for further doc.
  ExpandedFst<A> *Copy(bool safe = false) const override = 0;

  // Read an ExpandedFst from an input stream; return NULL on error.
  static ExpandedFst<A> *Read(std::istream &strm, const FstReadOptions &opts) {
    FstReadOptions ropts(opts);
    FstHeader hdr;
    if (ropts.header)
      hdr = *opts.header;
    else {
      if (!hdr.Read(strm, opts.source)) return 0;
      ropts.header = &hdr;
    }
    if (!(hdr.Properties() & kExpanded)) {
      LOG(ERROR) << "ExpandedFst::Read: Not an ExpandedFst: " << ropts.source;
      return 0;
    }
    FstRegister<A> *registr = FstRegister<A>::GetRegister();
    const typename FstRegister<A>::Reader reader =
        registr->GetReader(hdr.FstType());
    if (!reader) {
      LOG(ERROR) << "ExpandedFst::Read: Unknown FST type \"" << hdr.FstType()
                 << "\" (arc type = \"" << A::Type() << "\"): " << ropts.source;
      return 0;
    }
    Fst<A> *fst = reader(strm, ropts);
    if (!fst) return 0;
    return static_cast<ExpandedFst<A> *>(fst);
  }

  // Read an ExpandedFst from a file; return NULL on error.
  // Empty filename reads from standard input.
  static ExpandedFst<A> *Read(const string &filename) {
    if (!filename.empty()) {
      std::ifstream strm(filename.c_str(),
                              std::ios_base::in | std::ios_base::binary);
      if (!strm) {
        LOG(ERROR) << "ExpandedFst::Read: Can't open file: " << filename;
        return 0;
      }
      return Read(strm, FstReadOptions(filename));
    } else {
      return Read(std::cin, FstReadOptions("standard input"));
    }
  }
};

namespace internal {

//  ExpandedFst<A> case - abstract methods.
template <class A>
inline typename A::Weight Final(const ExpandedFst<A> &fst,
                                typename A::StateId s) {
  return fst.Final(s);
}

template <class A>
inline ssize_t NumArcs(const ExpandedFst<A> &fst, typename A::StateId s) {
  return fst.NumArcs(s);
}

template <class A>
inline ssize_t NumInputEpsilons(const ExpandedFst<A> &fst,
                                typename A::StateId s) {
  return fst.NumInputEpsilons(s);
}

template <class A>
inline ssize_t NumOutputEpsilons(const ExpandedFst<A> &fst,
                                 typename A::StateId s) {
  return fst.NumOutputEpsilons(s);
}

}  // namespace internal

// A useful alias when using StdArc.
typedef ExpandedFst<StdArc> StdExpandedFst;

// This is a helper class template useful for attaching an ExpandedFst
// interface to its implementation, handling reference counting. It
// delegates to ImplToFst the handling of the Fst interface methods.
template <class I, class F = ExpandedFst<typename I::Arc>>
class ImplToExpandedFst : public ImplToFst<I, F> {
 public:
  typedef typename I::Arc Arc;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;

  StateId NumStates() const override { return GetImpl()->NumStates(); }

 protected:
  using ImplToFst<I, F>::GetImpl;

  explicit ImplToExpandedFst(std::shared_ptr<I> impl) : ImplToFst<I, F>(impl) {}

  ImplToExpandedFst(const ImplToExpandedFst<I, F> &fst)
      : ImplToFst<I, F>(fst) {}

  ImplToExpandedFst(const ImplToExpandedFst<I, F> &fst, bool safe)
      : ImplToFst<I, F>(fst, safe) {}

  static I *Read(std::istream &strm, const FstReadOptions &opts) {
    return I::Read(strm, opts);
  }

  // Read FST implementation from a file; return NULL on error.
  // Empty filename reads from standard input.
  static I *Read(const string &filename) {
    if (!filename.empty()) {
      std::ifstream strm(filename.c_str(),
                              std::ios_base::in | std::ios_base::binary);
      if (!strm) {
        LOG(ERROR) << "ExpandedFst::Read: Can't open file: " << filename;
        return 0;
      }
      return I::Read(strm, FstReadOptions(filename));
    } else {
      return I::Read(std::cin, FstReadOptions("standard input"));
    }
  }
};

// Function to return the number of states in an FST, counting them
// if necessary.
template <class Arc>
typename Arc::StateId CountStates(const Fst<Arc> &fst) {
  if (fst.Properties(kExpanded, false)) {
    const ExpandedFst<Arc> *efst = static_cast<const ExpandedFst<Arc> *>(&fst);
    return efst->NumStates();
  } else {
    typename Arc::StateId nstates = 0;
    for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next())
      ++nstates;
    return nstates;
  }
}

// Function to return the number of arcs in an FST.
template <class Arc>
typename Arc::StateId CountArcs(const Fst<Arc> &fst) {
  size_t narcs = 0;
  for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next())
    narcs += fst.NumArcs(siter.Value());
  return narcs;
}

}  // namespace fst

#endif  // FST_LIB_EXPANDED_FST_H_
