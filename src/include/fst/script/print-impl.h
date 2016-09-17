// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Stand-alone class to print out binary FSTs in the AT&T format, a helper
// class for fstprint.cc.

#ifndef FST_SCRIPT_PRINT_IMPL_H_
#define FST_SCRIPT_PRINT_IMPL_H_

#include <ostream>
#include <sstream>
#include <string>

#include <fst/fstlib.h>
#include <fst/util.h>

DECLARE_string(fst_field_separator);

namespace fst {

// Print a binary Fst in textual format, helper class for fstprint.cc
// WARNING: Stand-alone use of this class not recommended, most code should
// read/write using the binary format which is much more efficient.
template <class A>
class FstPrinter {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  FstPrinter(const Fst<A> &fst, const SymbolTable *isyms,
             const SymbolTable *osyms, const SymbolTable *ssyms, bool accep,
             bool show_weight_one, const string &field_separator,
             const string &missing_symbol = "")
      : fst_(fst),
        isyms_(isyms),
        osyms_(osyms),
        ssyms_(ssyms),
        accep_(accep && fst.Properties(kAcceptor, true)),
        ostrm_(nullptr),
        show_weight_one_(show_weight_one),
        sep_(field_separator),
        missing_symbol_(missing_symbol) {}

  // Print Fst to an output stream
  void Print(std::ostream *ostrm, const string &dest) {
    ostrm_ = ostrm;
    dest_ = dest;
    StateId start = fst_.Start();
    if (start == kNoStateId) return;
    // initial state first
    PrintState(start);
    for (StateIterator<Fst<A>> siter(fst_); !siter.Done(); siter.Next()) {
      StateId s = siter.Value();
      if (s != start) PrintState(s);
    }
  }

 private:
  // Maximum line length in text file.
  static const int kLineLen = 8096;

  void PrintId(StateId id, const SymbolTable *syms, const char *name) const {
    if (syms) {
      string symbol = syms->Find(id);
      if (symbol == "") {
        if (missing_symbol_ == "") {
          FSTERROR() << "FstPrinter: Integer " << id
                     << " is not mapped to any textual symbol"
                     << ", symbol table = " << syms->Name()
                     << ", destination = " << dest_;
          symbol = "?";
        } else {
          symbol = missing_symbol_;
        }
      }
      *ostrm_ << symbol;
    } else {
      *ostrm_ << id;
    }
  }

  void PrintStateId(StateId s) const { PrintId(s, ssyms_, "state ID"); }

  void PrintILabel(Label l) const { PrintId(l, isyms_, "arc input label"); }

  void PrintOLabel(Label l) const { PrintId(l, osyms_, "arc output label"); }

  void PrintState(StateId s) const {
    bool output = false;
    for (ArcIterator<Fst<A>> aiter(fst_, s); !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      PrintStateId(s);
      *ostrm_ << sep_;
      PrintStateId(arc.nextstate);
      *ostrm_ << sep_;
      PrintILabel(arc.ilabel);
      if (!accep_) {
        *ostrm_ << sep_;
        PrintOLabel(arc.olabel);
      }
      if (show_weight_one_ || arc.weight != Weight::One())
        *ostrm_ << sep_ << arc.weight;
      *ostrm_ << "\n";
      output = true;
    }
    Weight final = fst_.Final(s);
    if (final != Weight::Zero() || !output) {
      PrintStateId(s);
      if (show_weight_one_ || final != Weight::One()) {
        *ostrm_ << sep_ << final;
      }
      *ostrm_ << "\n";
    }
  }

  const Fst<A> &fst_;
  const SymbolTable *isyms_;  // ilabel symbol table
  const SymbolTable *osyms_;  // olabel symbol table
  const SymbolTable *ssyms_;  // slabel symbol table
  bool accep_;                // print as acceptor when possible
  std::ostream *ostrm_;       // text FST destination
  string dest_;               // text FST destination name
  bool show_weight_one_;      // print weights equal to Weight::One()
  string sep_;                // separator character between fields.
  string missing_symbol_;     // symbol to print when lookup fails (default
                              // "" means raise error)
  FstPrinter(const FstPrinter &) = delete;
  FstPrinter &operator=(const FstPrinter &) = delete;
};

}  // namespace fst

#endif  // FST_SCRIPT_PRINT_IMPL_H_
