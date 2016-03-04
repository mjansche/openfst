// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Function to test equality of two FSTs.

#ifndef FST_LIB_EQUAL_H__
#define FST_LIB_EQUAL_H__

#include <fst/fst.h>
#include <fst/test-properties.h>


namespace fst {

const uint32 kEqualFsts = 0x0001;
const uint32 kEqualFstTypes = 0x0002;
const uint32 kEqualCompatProperties = 0x0004;
const uint32 kEqualCompatSymbols = 0x0008;
const uint32 kEqualAll = kEqualFsts | kEqualFstTypes | kEqualCompatProperties |
    kEqualCompatSymbols;

// Tests if two Fsts have the same states and arcs in the same order (when
// etype & kEqualFst).
// Also optional checks equality of Fst types (etype & kEqualFstTypes) and
// compatibility of stored properties (etype & kEqualCompatProperties) and
// of symbol tables (etype & kEqualCompatSymbols).
template <class Arc>
bool Equal(const Fst<Arc> &fst1, const Fst<Arc> &fst2, float delta = kDelta,
           uint32 etype = kEqualFsts) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  if ((etype & kEqualFstTypes) && (fst1.Type() != fst2.Type())) {
    VLOG(1) << "Equal: mismatched Fst types";
    return false;
  }

  if ((etype & kEqualCompatProperties) &&
      !CompatProperties(fst1.Properties(kCopyProperties, false),
                        fst2.Properties(kCopyProperties, false))) {
    VLOG(1) << "Equal: properties not compatible";
    return false;
  }

  if (etype & kEqualCompatSymbols) {
    if (!CompatSymbols(fst1.InputSymbols(), fst2.InputSymbols(), false)) {
      VLOG(1) << "Equal: input symbols not compatible";
      return false;
    }
    if (!CompatSymbols(fst1.OutputSymbols(), fst2.OutputSymbols(), false)) {
      VLOG(1) << "Equal: output symbols not compatible";
      return false;
    }
  }

  if (!(etype & kEqualFsts)) return true;

  if (fst1.Start() != fst2.Start()) {
    VLOG(1) << "Equal: Mismatched start states";
    return false;
  }

  StateIterator<Fst<Arc>> siter1(fst1);
  StateIterator<Fst<Arc>> siter2(fst2);

  while (!siter1.Done() || !siter2.Done()) {
    if (siter1.Done() || siter2.Done()) {
      VLOG(1) << "Equal: Mismatched # of states";
      return false;
    }
    StateId s1 = siter1.Value();
    StateId s2 = siter2.Value();
    if (s1 != s2) {
      VLOG(1) << "Equal: Mismatched states:"
              << ", state1 = " << s1 << ", state2 = " << s2;
      return false;
    }
    Weight final1 = fst1.Final(s1);
    Weight final2 = fst2.Final(s2);
    if (!ApproxEqual(final1, final2, delta)) {
      VLOG(1) << "Equal: Mismatched final weights:"
              << " state = " << s1 << ", final1 = " << final1
              << ", final2 = " << final2;
      return false;
    }
    ArcIterator<Fst<Arc>> aiter1(fst1, s1);
    ArcIterator<Fst<Arc>> aiter2(fst2, s2);
    for (size_t a = 0; !aiter1.Done() || !aiter2.Done(); ++a) {
      if (aiter1.Done() || aiter2.Done()) {
        VLOG(1) << "Equal: Mismatched # of arcs"
                << " state = " << s1;
        return false;
      }
      Arc arc1 = aiter1.Value();
      Arc arc2 = aiter2.Value();
      if (arc1.ilabel != arc2.ilabel) {
        VLOG(1) << "Equal: Mismatched arc input labels:"
                << " state = " << s1 << ", arc = " << a
                << ", ilabel1 = " << arc1.ilabel
                << ", ilabel2 = " << arc2.ilabel;
        return false;
      } else if (arc1.olabel != arc2.olabel) {
        VLOG(1) << "Equal: Mismatched arc output labels:"
                << " state = " << s1 << ", arc = " << a
                << ", olabel1 = " << arc1.olabel
                << ", olabel2 = " << arc2.olabel;
        return false;
      } else if (!ApproxEqual(arc1.weight, arc2.weight, delta)) {
        VLOG(1) << "Equal: Mismatched arc weights:"
                << " state = " << s1 << ", arc = " << a
                << ", weight1 = " << arc1.weight
                << ", weight2 = " << arc2.weight;
        return false;
      } else if (arc1.nextstate != arc2.nextstate) {
        VLOG(1) << "Equal: Mismatched next states:"
                << " state = " << s1 << ", arc = " << a
                << ", nextstate1 = " << arc1.nextstate
                << ", nextstate2 = " << arc2.nextstate;
        return false;
      }
      aiter1.Next();
      aiter2.Next();
    }
    // Sanity checks: should never fail
    if (fst1.NumArcs(s1) != fst2.NumArcs(s2) ||
        fst1.NumInputEpsilons(s1) != fst2.NumInputEpsilons(s2) ||
        fst1.NumOutputEpsilons(s1) != fst2.NumOutputEpsilons(s2)) {
      FSTERROR() << "Equal: Inconsistent arc/epsilon counts";
    }

    siter1.Next();
    siter2.Next();
  }
  return true;
}

}  // namespace fst

#endif  // FST_LIB_EQUAL_H__
