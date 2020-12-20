// pair-arc.h
// Author: riley@google.com (Michael Riley)
//
// \file
// An Fst arc type with weights over product of the tropical semiring
// with itself.

#ifndef FST_TEST_PAIR_ARC_H__
#define FST_TEST_PAIR_ARC_H__

#include "fst/lib/arc.h"

namespace fst {

// Arc with integer labels and state Ids and weights over the
// product of the tropical semiring with itself.
struct PairArc {
  typedef int Label;
  typedef ProductWeight<TropicalWeight, TropicalWeight> Weight;
  typedef int StateId;

  PairArc(Label i, Label o, Weight w, StateId s)
      : ilabel(i), olabel(o), weight(w), nextstate(s) {}

  PairArc() {}

  static const string &Type() {  // Arc type name
    static const string type = "pair";
    return type;
  }

  Label ilabel;       // Transition input label
  Label olabel;       // Transition output label
  Weight weight;      // Transition weight
  StateId nextstate;  // Transition destination state
};

}  // namespace fst;

#endif  // FST_TEST_PAIR_ARC_H__
