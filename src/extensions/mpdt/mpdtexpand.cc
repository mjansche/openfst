// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Expands a (bounded-stack) MPDT as an FST.

#include <string>
#include <vector>

#include <fst/extensions/mpdt/mpdtscript.h>
#include <fst/extensions/mpdt/read_write_utils.h>
#include <fst/util.h>

DEFINE_string(mpdt_parentheses, "",
              "MPDT parenthesis label pairs with assignments.");
DEFINE_bool(connect, true, "Trim output");
DEFINE_bool(keep_parentheses, false, "Keep PDT parentheses in result.");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Expand a (bounded-stack) MPDT as an FST.\n\n  Usage: ";
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

  if (FLAGS_mpdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No MPDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<s::LabelPair> parens;
  std::vector<int64> assignments;
  fst::ReadLabelTriples(FLAGS_mpdt_parentheses, &parens, &assignments,
                            false);

  s::VectorFstClass ofst(ifst->ArcType());
  s::MPdtExpand(*ifst, parens, assignments, &ofst,
      fst::MPdtExpandOptions(FLAGS_connect, FLAGS_keep_parentheses));

  ofst.Write(out_name);

  return 0;
}
