// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Generates random paths through an FST.

#include <fst/script/randgen.h>

DEFINE_int32(max_length, INT_MAX, "Maximum path length");
DEFINE_int32(npath, 1, "Number of paths to generate");
DEFINE_int32(seed, time(0), "Random seed");
DEFINE_string(select, "uniform",
              "Selection type: one of: "
              " \"uniform\", \"log_prob\" (when appropriate),"
              " \"fast_log_prob\" (when appropriate)");
DEFINE_bool(weighted, false,
            "Output tree weighted by path count vs. unweighted paths");
DEFINE_bool(remove_total_weight, false,
            "Remove total weight when output weighted");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  string usage = "Generates random paths through an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  VLOG(1) << argv[0] << ": Seed = " << FLAGS_seed;

  string in_name = (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  FstClass *ifst = FstClass::Read(in_name);
  if (!ifst) return 1;

  VectorFstClass ofst(ifst->ArcType());

  s::RandArcSelection ras;

  if (FLAGS_select == "uniform") {
    ras = s::UNIFORM_ARC_SELECTOR;
  } else if (FLAGS_select == "log_prob") {
    ras = s::LOG_PROB_ARC_SELECTOR;
  } else if (FLAGS_select == "fast_log_prob") {
    ras = s::FAST_LOG_PROB_ARC_SELECTOR;
  } else {
    LOG(ERROR) << argv[0] << ": Unknown selection type \"" << FLAGS_select
               << "\"\n";
    return 1;
  }

  s::RandGen(*ifst, &ofst, FLAGS_seed,
             fst::RandGenOptions<s::RandArcSelection>(
                 ras, FLAGS_max_length, FLAGS_npath, FLAGS_weighted,
                 FLAGS_remove_total_weight));

  ofst.Write(out_name);
  return 0;
}
