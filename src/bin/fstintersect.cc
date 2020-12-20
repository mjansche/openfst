// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Intersects two FSTs.

#include <memory>

#include <fst/script/connect.h>
#include <fst/script/intersect.h>

DEFINE_string(compose_filter, "auto",
              "Composition filter, one of: \"alt_sequence\", \"auto\", "
              "\"match\", \"sequence\"");
DEFINE_bool(connect, true, "Trim output");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  string usage = "Intersects two FSAs.\n\n  Usage: ";
  usage += argv[0];
  usage += " in1.fst in2.fst [out.fst]\n";
  usage += "  Flags: connect\n";

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

  std::unique_ptr<FstClass> ifst1(FstClass::Read(in1_name));
  if (!ifst1) return 1;
  std::unique_ptr<FstClass> ifst2(FstClass::Read(in2_name));
  if (!ifst2) return 1;

  VectorFstClass ofst(ifst1->ArcType());

  fst::ComposeFilter compose_filter;

  if (FLAGS_compose_filter == "alt_sequence") {
    compose_filter = fst::ALT_SEQUENCE_FILTER;
  } else if (FLAGS_compose_filter == "auto") {
    compose_filter = fst::AUTO_FILTER;
  } else if (FLAGS_compose_filter == "match") {
    compose_filter = fst::MATCH_FILTER;
  } else if (FLAGS_compose_filter == "sequence") {
    compose_filter = fst::SEQUENCE_FILTER;
  } else {
    LOG(ERROR) << argv[0]
               << "Unknown compose filter type: " << FLAGS_compose_filter;
    return 1;
  }

  fst::IntersectOptions opts(FLAGS_connect, compose_filter);

  s::Intersect(*ifst1, *ifst2, &ofst, opts);

  ofst.Write(out_name);

  return 0;
}
