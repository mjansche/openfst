// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Converts an RTN represented by FSTs and non-terminal labels into a PDT.

#include <string>
#include <vector>

#include <fst/extensions/pdt/getters.h>
#include <fst/extensions/pdt/pdtscript.h>
#include <fst/util.h>
#include <fst/vector-fst.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs.");
DEFINE_string(pdt_parser_type, "left",
              "Construction method, one of: \"left\", \"left_sr\"");
DEFINE_int64(start_paren_labels, fst::kNoLabel,
             "Index to use for the first inserted parentheses; if not "
             "specified, the next available label beyond the highest output "
             "label is used");
DEFINE_string(left_paren_prefix, "(_", "Prefix to attach to SymbolTable "
              "labels for inserted left parentheses");
DEFINE_string(right_paren_prefix, ")_", "Prefix to attach to SymbolTable "
              "labels for inserted right parentheses");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Converts an RTN represented by FSTs";
  usage += " and non-terminal labels into PDT.\n\n  Usage: ";
  usage += argv[0];
  usage += " root.fst rootlabel [rule1.fst label1 ...] [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 4) {
    ShowUsage();
    return 1;
  }

  string in_fname = argv[1];
  string out_fname = argc % 2 == 0 ? argv[argc - 1] : "";

  s::FstClass *ifst = s::FstClass::Read(in_fname);
  if (!ifst) return 1;

  fst::PdtParserType parser_type;
  if (!s::GetPdtParserType(FLAGS_pdt_parser_type, &parser_type)) {
    LOG(ERROR) << argv[0] << ": Unknown PDT parser type: "
               << FLAGS_pdt_parser_type;
    return 1;
  }

  std::vector<s::LabelFstClassPair> pairs;
  int64 root = atoll(argv[2]);
  pairs.push_back(std::make_pair(root, ifst));

  for (size_t i = 3; i < argc - 1; i += 2) {
    ifst = s::FstClass::Read(argv[i]);
    if (!ifst) return 1;
    int64 lab = atoll(argv[i + 1]);
    pairs.push_back(std::make_pair(lab, ifst));
  }

  s::VectorFstClass ofst(ifst->ArcType());
  std::vector<s::LabelPair> parens;
  s::PdtReplace(pairs, &ofst, &parens, root, parser_type,
                FLAGS_start_paren_labels, FLAGS_left_paren_prefix,
                FLAGS_right_paren_prefix);

  if (!FLAGS_pdt_parentheses.empty())
    fst::WriteLabelPairs(FLAGS_pdt_parentheses, parens);

  ofst.Write(out_fname);

  return 0;
}
