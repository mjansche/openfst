// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Prints out various information about a PDT such as number of states, arcs,
// and parentheses.

#include <vector>

#include <fst/extensions/pdt/pdtscript.h>
#include <fst/util.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs.");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Prints out information about a PDT.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 2) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  s::FstClass *ifst = s::FstClass::Read(in_name);
  if (!ifst) return 1;

  if (FLAGS_pdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<s::LabelPair> parens;
  fst::ReadLabelPairs(FLAGS_pdt_parentheses, &parens, false);

  s::PrintPdtInfo(*ifst, parens);

  return 0;
}
