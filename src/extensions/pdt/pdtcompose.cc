// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Composes a PDT and an FST.

#include <string>
#include <vector>

#include <fst/extensions/pdt/getters.h>
#include <fst/extensions/pdt/pdtscript.h>
#include <fst/util.h>
#include <fst/script/connect.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs.");
DEFINE_bool(left_pdt, true, "1st arg is PDT (o.w. 2nd arg).");
DEFINE_bool(connect, true, "Trim output");
DEFINE_string(compose_filter, "paren",
              "Composition filter, one of: \"expand\", \"expand_paren\", "
              "\"paren\"");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Compose a PDT and an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt in.fst [out.pdt]\n";
  usage += " in.fst in.pdt [out.pdt]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  string in1_name = strcmp(argv[1], "-") == 0 ? "" : argv[1];
  string in2_name = strcmp(argv[2], "-") == 0 ? "" : argv[2];
  string out_name = argc > 3 ? argv[3] : "";

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input.";
    return 1;
  }

  s::FstClass *ifst1 = s::FstClass::Read(in1_name);
  if (!ifst1) return 1;
  s::FstClass *ifst2 = s::FstClass::Read(in2_name);
  if (!ifst2) return 1;

  if (FLAGS_pdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<s::LabelPair> parens;
  fst::ReadLabelPairs(FLAGS_pdt_parentheses, &parens, false);

  s::VectorFstClass ofst(ifst1->ArcType());

  fst::PdtComposeFilter compose_filter;
  if (!s::GetPdtComposeFilter(FLAGS_compose_filter, &compose_filter)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported compose filter type: "
               << FLAGS_compose_filter;
    return 1;
  }

  fst::PdtComposeOptions copts(FLAGS_connect, compose_filter);

  s::PdtCompose(*ifst1, *ifst2, parens, &ofst, copts, FLAGS_left_pdt);

  ofst.Write(out_name);

  return 0;
}
