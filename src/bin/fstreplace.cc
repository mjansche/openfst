// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Performs the dynamic replacement of arcs in one FST with another FST,
// allowing for the definition of FSTs analogous to RTNs.

#include <string>
#include <vector>

#include <fst/script/replace.h>

DEFINE_string(call_arc_labeling, "input",
              "Which labels to make non-epsilon on the call arc. "
              "One of: \"input\" (default), \"output\", \"both\", \"neither\"");
DEFINE_string(return_arc_labeling, "neither",
              "Which labels to make non-epsilon on the return arc. "
              "One of: \"input\", \"output\", \"both\", \"neither\" (default)");
DEFINE_int64(return_label, 0, "Label to put on return arc");
DEFINE_bool(epsilon_on_replace, false,
            "For backwards compatibility: call/return arcs are epsilon arcs");

// Assigns replace_label_type from enum values based on command line switches.
fst::ReplaceLabelType replace_type(string *arc_labeling, char *binname,
                                       string errmsg, bool epsilon_on_replace) {
  fst::ReplaceLabelType replace_label_type;
  if ((*arc_labeling) == "neither" || epsilon_on_replace) {
    replace_label_type = fst::REPLACE_LABEL_NEITHER;
  } else if ((*arc_labeling) == "input") {
    replace_label_type = fst::REPLACE_LABEL_INPUT;
  } else if ((*arc_labeling) == "output") {
    replace_label_type = fst::REPLACE_LABEL_OUTPUT;
  } else if ((*arc_labeling) == "both") {
    replace_label_type = fst::REPLACE_LABEL_BOTH;
  } else {
    LOG(ERROR) << binname << errmsg
               << "arc labeling option: " << (*arc_labeling);
    exit(1);
  }
  return replace_label_type;
}

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  string usage =
      "Recursively replaces FST arcs with other FST(s).\n\n"
      "  Usage: ";
  usage += argv[0];
  usage += " root.fst rootlabel [rule1.fst label1 ...] [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 4) {
    ShowUsage();
    return 1;
  }

  string in_fname = argv[1];
  string out_fname = argc % 2 == 0 ? argv[argc - 1] : "";

  FstClass *ifst = FstClass::Read(in_fname);
  if (!ifst) return 1;

  std::vector<s::LabelFstClassPair> pairs;
  // Warning: if the root label is beyond the range of the underlying FST's
  // labels, truncation will occur.
  int64 root = atoll(argv[2]);
  pairs.emplace_back(root, ifst);

  for (size_t i = 3; i < argc - 1; i += 2) {
    ifst = s::FstClass::Read(argv[i]);
    if (!ifst) return 1;
    // Warning: if the root label is beyond the range of the underlying FST's
    // labels, truncation will occur.
    int64 lab = atoll(argv[i + 1]);
    pairs.emplace_back(lab, ifst);
  }

  fst::ReplaceLabelType call_label_type =
      replace_type(&FLAGS_call_arc_labeling, argv[0], ": bad call ",
                   FLAGS_epsilon_on_replace);
  fst::ReplaceLabelType return_label_type =
      replace_type(&FLAGS_return_arc_labeling, argv[0], ": bad return ",
                   FLAGS_epsilon_on_replace);

  s::ReplaceOptions opts(root, call_label_type, return_label_type,
                         FLAGS_return_label);

  VectorFstClass ofst(ifst->ArcType());
  s::Replace(pairs, &ofst, opts);

  ofst.Write(out_fname);

  return 0;
}
