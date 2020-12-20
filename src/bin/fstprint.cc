// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Prints out binary FSTs in simple text format used by AT&T.

#include <ostream>

#include <fstream>
#include <fst/script/print.h>

DEFINE_bool(acceptor, false, "Input in acceptor format");
DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_string(ssymbols, "", "State label symbol table");
DEFINE_bool(numeric, false, "Print numeric labels");
DEFINE_string(save_isymbols, "", "Save input symbol table to file");
DEFINE_string(save_osymbols, "", "Save output symbol table to file");
DEFINE_bool(show_weight_one, false,
            "Print/draw arc weights and final weights equal to semiring One");
DEFINE_bool(allow_negative_labels, false,
            "Allow negative labels (not recommended; may cause conflicts)");
DEFINE_string(missing_symbol, "",
              "symbol to print when lookup fails (default raises error)");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::SymbolTable;

  string usage = "Prints out binary FSTs in simple text format.\n\n  Usage: ";
  usage += argv[0];
  usage += " [binary.fst [text.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  s::FstClass *fst = s::FstClass::Read(in_name);
  if (!fst) return 1;

  std::ostream *ostrm = &std::cout;
  string dest = "standard output";
  if (argc == 3) {
    dest = argv[2];
    ostrm = new std::ofstream(argv[2]);
    if (!*ostrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[2];
      return 1;
    }
  }
  ostrm->precision(9);

  const SymbolTable *isyms = nullptr;
  const SymbolTable *osyms = nullptr;
  const SymbolTable *ssyms = nullptr;

  fst::SymbolTableTextOptions opts;
  opts.allow_negative = FLAGS_allow_negative_labels;

  if (!FLAGS_isymbols.empty() && !FLAGS_numeric) {
    isyms = SymbolTable::ReadText(FLAGS_isymbols, opts);
    if (!isyms) return 1;
  }

  if (!FLAGS_osymbols.empty() && !FLAGS_numeric) {
    osyms = SymbolTable::ReadText(FLAGS_osymbols, opts);
    if (!osyms) return 1;
  }

  if (!FLAGS_ssymbols.empty() && !FLAGS_numeric) {
    ssyms = SymbolTable::ReadText(FLAGS_ssymbols);
    if (!ssyms) return 1;
  }

  if (!isyms && !FLAGS_numeric) isyms = fst->InputSymbols();
  if (!osyms && !FLAGS_numeric) osyms = fst->OutputSymbols();

  s::PrintFst(*fst, *ostrm, dest, isyms, osyms, ssyms, FLAGS_acceptor,
              FLAGS_show_weight_one, FLAGS_missing_symbol);

  if (isyms && !FLAGS_save_isymbols.empty())
    isyms->WriteText(FLAGS_save_isymbols);

  if (osyms && !FLAGS_save_osymbols.empty())
    osyms->WriteText(FLAGS_save_osymbols);

  if (ostrm != &std::cout) delete ostrm;

  return 0;
}
