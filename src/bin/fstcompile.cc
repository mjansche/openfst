// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Creates binary FSTs from simple text format used by AT&T.

#include <istream>

#include <fstream>
#include <fst/script/compile.h>

DEFINE_bool(acceptor, false, "Input in acceptor format");
DEFINE_string(arc_type, "standard", "Output arc type");
DEFINE_string(fst_type, "vector", "Output FST type");
DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_string(ssymbols, "", "State label symbol table");
DEFINE_bool(keep_isymbols, false, "Store input label symbol table with FST");
DEFINE_bool(keep_osymbols, false, "Store output label symbol table with FST");
DEFINE_bool(keep_state_numbering, false, "Do not renumber input states");
DEFINE_bool(allow_negative_labels, false,
            "Allow negative labels (not recommended; may cause conflicts)");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::SymbolTable;

  string usage = "Creates binary FSTs from simple text format.\n\n  Usage: ";
  usage += argv[0];
  usage += " [text.fst [binary.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  const char *source = "standard input";
  std::istream *istrm = &std::cin;
  if (argc > 1 && strcmp(argv[1], "-") != 0) {
    source = argv[1];
    istrm = new std::ifstream(argv[1]);
    if (!*istrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[1];
      return 1;
    }
  }
  const SymbolTable *isyms = nullptr;
  const SymbolTable *osyms = nullptr;
  const SymbolTable *ssyms = nullptr;

  fst::SymbolTableTextOptions opts;
  opts.allow_negative = FLAGS_allow_negative_labels;

  if (!FLAGS_isymbols.empty()) {
    isyms = SymbolTable::ReadText(FLAGS_isymbols, opts);
    if (!isyms) return 1;
  }

  if (!FLAGS_osymbols.empty()) {
    osyms = SymbolTable::ReadText(FLAGS_osymbols, opts);
    if (!osyms) return 1;
  }

  if (!FLAGS_ssymbols.empty()) {
    ssyms = SymbolTable::ReadText(FLAGS_ssymbols);
    if (!ssyms) return 1;
  }

  string dest = argc > 2 ? argv[2] : "";

  s::CompileFst(*istrm, source, dest, FLAGS_fst_type, FLAGS_arc_type, isyms,
                osyms, ssyms, FLAGS_acceptor, FLAGS_keep_isymbols,
                FLAGS_keep_osymbols, FLAGS_keep_state_numbering,
                FLAGS_allow_negative_labels);

  if (istrm != &std::cin) delete istrm;

  return 0;
}
