// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Creates the Kleene closure of an FST.

#include <fst/script/closure.h>

DEFINE_bool(closure_plus, false,
            "Do not add the empty path (T+ instead of T*)");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;

  string usage = "Creates the Kleene closure of an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_fname = (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  string out_fname = argc > 2 ? argv[2] : "";

  MutableFstClass *fst = MutableFstClass::Read(in_fname, true);
  if (!fst) return 1;

  fst::ClosureType closure_type =
      FLAGS_closure_plus ? fst::CLOSURE_PLUS : fst::CLOSURE_STAR;

  s::Closure(fst, closure_type);

  fst->Write(out_fname);

  return 0;
}
