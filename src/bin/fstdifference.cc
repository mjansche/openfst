// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Subtracts an unweighted DFA from an FSA.

#include <fst/script/difference.h>
#include <fst/script/connect.h>

DEFINE_string(compose_filter, "auto",
              "Composition filter, one of: \"alt_sequence\", \"auto\","
              " \"match\", \"sequence\"");
DEFINE_bool(connect, true, "Trim output");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;
  using fst::script::VectorFstClass;

  string usage = "Subtracts an unweighted DFA from an FSA.\n\n  Usage: ";
  usage += argv[0];
  usage += " in1.fst in2.fst [out.fst]\n";

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

  FstClass *ifst1 = FstClass::Read(in1_name);
  if (!ifst1) return 1;
  FstClass *ifst2 = FstClass::Read(in2_name);
  if (!ifst2) return 1;

  VectorFstClass ofst(ifst1->ArcType());

  fst::ComposeFilter cf;

  if (FLAGS_compose_filter == "auto") {
    cf = fst::AUTO_FILTER;
  } else if (FLAGS_compose_filter == "sequence") {
    cf = fst::SEQUENCE_FILTER;
  } else if (FLAGS_compose_filter == "alt_sequence") {
    cf = fst::ALT_SEQUENCE_FILTER;
  } else if (FLAGS_compose_filter == "match") {
    cf = fst::MATCH_FILTER;
  } else {
    LOG(ERROR) << argv[0] << ": Bad filter type \"" << FLAGS_compose_filter
               << "\"";
    return 1;
  }

  fst::DifferenceOptions opts(FLAGS_connect, cf);

  s::Difference(*ifst1, *ifst2, &ofst, opts);

  ofst.Write(out_name);

  return 0;
}
