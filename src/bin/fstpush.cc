// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Pushes weights and/or output labels in an FST toward the initial or final
// states.

#include <fst/script/push.h>

DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_bool(push_weights, false, "Push weights");
DEFINE_bool(push_labels, false, "Push output labels");
DEFINE_bool(remove_total_weight, false,
            "Remove total weight when pushing weights");
DEFINE_bool(remove_common_affix, false,
            "Remove common prefix/suffix when pushing labels");
DEFINE_bool(to_final, false, "Push/reweight to final (vs. to initial) states");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  string usage = "Pushes weights and/or olabels in an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  FstClass *ifst = FstClass::Read(in_name);
  if (!ifst) return 1;

  uint32 flags = 0;
  if (FLAGS_push_weights) flags |= fst::kPushWeights;
  if (FLAGS_push_labels) flags |= fst::kPushLabels;
  if (FLAGS_remove_total_weight) flags |= fst::kPushRemoveTotalWeight;
  if (FLAGS_remove_common_affix) flags |= fst::kPushRemoveCommonAffix;

  VectorFstClass ofst(ifst->ArcType());

  if (FLAGS_to_final) {
    s::Push(*ifst, &ofst, flags, fst::REWEIGHT_TO_FINAL, FLAGS_delta);
  } else {
    s::Push(*ifst, &ofst, flags, fst::REWEIGHT_TO_INITIAL, FLAGS_delta);
  }

  ofst.Write(out_name);

  return 0;
}
