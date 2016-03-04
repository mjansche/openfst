// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Reweights an FST.

#include <fst/script/reweight.h>
#include <fst/script/text-io.h>

DEFINE_bool(to_final, false, "Push/reweight to final (vs. to initial) states");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;

  string usage = "Reweights an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.fst potential.txt [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  string in_fname = argv[1];
  string potentials_fname = argv[2];
  string out_fname = argc > 3 ? argv[3] : "";

  MutableFstClass *fst = MutableFstClass::Read(in_fname, true);
  if (!fst) return 1;

  std::vector<s::WeightClass> potential;
  if (!s::ReadPotentials(fst->WeightType(), potentials_fname, &potential))
    return 1;

  fst::ReweightType reweight_type = FLAGS_to_final
                                            ? fst::REWEIGHT_TO_FINAL
                                            : fst::REWEIGHT_TO_INITIAL;

  s::Reweight(fst, potential, reweight_type);

  fst->Write(out_fname);

  return 0;
}
