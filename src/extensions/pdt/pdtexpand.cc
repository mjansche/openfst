// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Expands a (bounded-stack) PDT as an FST.

#include <vector>

#include <fst/extensions/pdt/pdtscript.h>
#include <fst/util.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs.");
DEFINE_bool(connect, true, "Trim output");
DEFINE_bool(keep_parentheses, false, "Keep PDT parentheses in result.");
DEFINE_string(weight, "", "Weight threshold");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Expand a (bounded-stack) PDT as an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  s::FstClass *ifst = s::FstClass::Read(in_name);
  if (!ifst) return 1;

  if (FLAGS_pdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<s::LabelPair> parens;
  fst::ReadLabelPairs(FLAGS_pdt_parentheses, &parens, false);

  s::WeightClass weight_threshold =
      FLAGS_weight.empty() ? s::WeightClass::Zero(ifst->WeightType())
                           : s::WeightClass(ifst->WeightType(), FLAGS_weight);

  s::VectorFstClass ofst(ifst->ArcType());
  s::PdtExpand(*ifst, parens, &ofst,
               s::PdtExpandOptions(FLAGS_connect, FLAGS_keep_parentheses,
                                   weight_threshold));

  ofst.Write(out_name);

  return 0;
}
