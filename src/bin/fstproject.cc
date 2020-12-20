// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Projects a transduction onto its input or output language.

#include <fst/script/project.h>

DEFINE_bool(project_output, false, "Project on output (vs. input)");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;

  string usage =
      "Projects a transduction onto its input"
      " or output language.\n\n  Usage: ";
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

  MutableFstClass *fst = MutableFstClass::Read(in_name, true);
  if (!fst) return 1;

  fst::ProjectType project_type =
      FLAGS_project_output ? fst::PROJECT_OUTPUT : fst::PROJECT_INPUT;

  s::Project(fst, project_type);

  fst->Write(out_name);

  return 0;
}
