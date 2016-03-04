// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Outputs as strings the string FSTs in a finite-state archive.

#ifndef FST_EXTENSIONS_FAR_PRINT_STRINGS_H__
#define FST_EXTENSIONS_FAR_PRINT_STRINGS_H__

#include <string>
#include <vector>

#include <fst/extensions/far/far.h>
#include <fstream>
#include <fst/shortest-distance.h>
#include <fst/string.h>

DECLARE_string(far_field_separator);

namespace fst {

template <class Arc>
void FarPrintStrings(const std::vector<string> &ifilenames,
                     const FarEntryType entry_type,
                     const FarTokenType far_token_type, const string &begin_key,
                     const string &end_key, const bool print_key,
                     const bool print_weight, const string &symbols_fname,
                     const bool initial_symbols, const int32 generate_filenames,
                     const string &filename_prefix,
                     const string &filename_suffix) {
  typename StringPrinter<Arc>::TokenType token_type;
  if (far_token_type == FTT_SYMBOL) {
    token_type = StringPrinter<Arc>::SYMBOL;
  } else if (far_token_type == FTT_BYTE) {
    token_type = StringPrinter<Arc>::BYTE;
  } else if (far_token_type == FTT_UTF8) {
    token_type = StringPrinter<Arc>::UTF8;
  } else {
    FSTERROR() << "FarPrintStrings: Unknown token type";
    return;
  }

  const SymbolTable *syms = nullptr;
  if (!symbols_fname.empty()) {
    // allow negative flag?
    SymbolTableTextOptions opts;
    opts.allow_negative = true;
    syms = SymbolTable::ReadText(symbols_fname, opts);
    if (!syms) {
      LOG(ERROR) << "FarPrintStrings: Error reading symbol table: "
                 << symbols_fname;
      return;
    }
  }

  FarReader<Arc> *far_reader = FarReader<Arc>::Open(ifilenames);
  if (!far_reader) return;

  if (!begin_key.empty()) far_reader->Find(begin_key);

  string okey;
  int nrep = 0;
  for (int i = 1; !far_reader->Done(); far_reader->Next(), ++i) {
    string key = far_reader->GetKey();
    if (!end_key.empty() && end_key < key) break;
    if (okey == key)
      ++nrep;
    else
      nrep = 0;
    okey = key;

    const Fst<Arc> &fst = far_reader->GetFst();
    if (i == 1 && initial_symbols && syms == nullptr && fst.InputSymbols())
      syms = fst.InputSymbols()->Copy();
    string str;
    VLOG(2) << "Handling key: " << key;
    StringPrinter<Arc> string_printer(token_type,
                                      syms ? syms : fst.InputSymbols());
    string_printer(fst, &str);

    if (entry_type == FET_LINE) {
      if (print_key) std::cout << key << FLAGS_far_field_separator[0];
      std::cout << str;
      if (print_weight)
        std::cout << FLAGS_far_field_separator[0] << ShortestDistance(fst);
      std::cout << std::endl;
    } else if (entry_type == FET_FILE) {
      stringstream sstrm;
      if (generate_filenames) {
        sstrm.fill('0');
        sstrm << std::right << std::setw(generate_filenames) << i;
      } else {
        sstrm << key;
        if (nrep > 0) sstrm << "." << nrep;
      }

      string filename;
      filename = filename_prefix + sstrm.str() + filename_suffix;

      std::ofstream ostrm(filename.c_str());
      if (!ostrm) {
        LOG(ERROR) << "FarPrintStrings: Can't open file: " << filename;
        delete syms;
        delete far_reader;
        return;
      }
      ostrm << str;
      if (token_type == StringPrinter<Arc>::SYMBOL) ostrm << "\n";
    }
  }
  delete syms;
}

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_PRINT_STRINGS_H__
