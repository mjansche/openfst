// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Utilities to convert strings into FSTs.

#ifndef FST_LIB_STRING_H_
#define FST_LIB_STRING_H_

#include <sstream>

#include <fst/compact-fst.h>
#include <fst/icu.h>
#include <fst/mutable-fst.h>
#include <fst/util.h>


DECLARE_string(fst_field_separator);

namespace fst {

// This will eventually replace StringCompiler<Arc>::TokenType and
// StringPrinter<Arc>::TokenType.
enum StringTokenType { SYMBOL = 1, BYTE = 2, UTF8 = 3 };

// Functor compiling a string in an FST
template <class A>
class StringCompiler {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  enum TokenType { SYMBOL = 1, BYTE = 2, UTF8 = 3 };

  StringCompiler(TokenType type, const SymbolTable *syms = 0,
                 Label unknown_label = kNoLabel, bool allow_negative = false)
      : token_type_(type),
        syms_(syms),
        unknown_label_(unknown_label),
        allow_negative_(allow_negative) {}

  StringCompiler(StringTokenType type, const SymbolTable *syms = 0,
                 Label unknown_label = kNoLabel, bool allow_negative = false)
      : StringCompiler(static_cast<TokenType>(type), syms, unknown_label,
                       allow_negative) {}

  // Compile string 's' into FST 'fst'.
  template <class F>
  bool operator()(const string &s, F *fst) const {
    std::vector<Label> labels;
    if (!ConvertStringToLabels(s, &labels)) return false;
    Compile(labels, fst);
    return true;
  }

  template <class F>
  bool operator()(const string &s, F *fst, Weight w) const {
    std::vector<Label> labels;
    if (!ConvertStringToLabels(s, &labels)) return false;
    Compile(labels, fst, w);
    return true;
  }

 private:
  bool ConvertStringToLabels(const string &str,
                             std::vector<Label> *labels) const {
    labels->clear();
    if (token_type_ == BYTE) {
      for (size_t i = 0; i < str.size(); ++i)
        labels->push_back(static_cast<unsigned char>(str[i]));
    } else if (token_type_ == UTF8) {
      return UTF8StringToLabels(str, labels);
    } else {
      char *c_str = new char[str.size() + 1];
      str.copy(c_str, str.size());
      c_str[str.size()] = 0;
      std::vector<char *> vec;
      string separator = "\n" + FLAGS_fst_field_separator;
      SplitToVector(c_str, separator.c_str(), &vec, true);
      for (size_t i = 0; i < vec.size(); ++i) {
        Label label;
        if (!ConvertSymbolToLabel(vec[i], &label)) {
          delete[] c_str;
          return false;
        }
        labels->push_back(label);
      }
      delete[] c_str;
    }
    return true;
  }

  void Compile(const std::vector<Label> &labels, MutableFst<A> *fst,
               const Weight &weight = Weight::One()) const {
    fst->DeleteStates();
    while (fst->NumStates() <= labels.size()) fst->AddState();
    for (size_t i = 0; i < labels.size(); ++i)
      fst->AddArc(i, Arc(labels[i], labels[i], Weight::One(), i + 1));
    fst->SetStart(0);
    fst->SetFinal(labels.size(), weight);
  }

  template <class Unsigned>
  void Compile(const std::vector<Label> &labels,
               CompactStringFst<A, Unsigned> *fst) const {
    fst->SetCompactElements(labels.begin(), labels.end());
  }

  template <class Unsigned>
  void Compile(const std::vector<Label> &labels,
               CompactWeightedStringFst<A, Unsigned> *fst,
               const Weight &weight = Weight::One()) const {
    std::vector<std::pair<Label, Weight>> compacts;
    compacts.reserve(labels.size() + 1);
    for (int i = 0; i < static_cast<int>(labels.size()) - 1; ++i)
      compacts.push_back(std::make_pair(labels[i], Weight::One()));
    compacts.push_back(
        std::make_pair(labels.size() > 0 ? labels.back() : kNoLabel, weight));
    fst->SetCompactElements(compacts.begin(), compacts.end());
  }

  bool ConvertSymbolToLabel(const char *s, Label *output) const {
    int64 n;
    if (syms_) {
      n = syms_->Find(s);
      if ((n == -1) && (unknown_label_ != kNoLabel)) n = unknown_label_;
      if (n == -1 || (!allow_negative_ && n < 0)) {
        VLOG(1) << "StringCompiler::ConvertSymbolToLabel: Symbol \"" << s
                << "\" is not mapped to any integer label, symbol table = "
                << syms_->Name();
        return false;
      }
    } else {
      char *p;
      n = strtoll(s, &p, 10);
      if (p < s + strlen(s) || (!allow_negative_ && n < 0)) {
        VLOG(1) << "StringCompiler::ConvertSymbolToLabel: Bad label integer "
                << "= \"" << s << "\"";
        return false;
      }
    }
    *output = n;
    return true;
  }

  TokenType token_type_;     // Token type: symbol, byte or utf8 encoded
  const SymbolTable *syms_;  // Symbol table used when token type is symbol
  Label unknown_label_;      // Label for token missing from symbol table
  bool allow_negative_;      // Negative labels allowed?

  DISALLOW_COPY_AND_ASSIGN(StringCompiler);
};

// Functor to print a string FST as a string.
template <class A>
class StringPrinter {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  enum TokenType { SYMBOL = 1, BYTE = 2, UTF8 = 3 };

  explicit StringPrinter(TokenType token_type, const SymbolTable *syms = 0)
      : token_type_(token_type), syms_(syms) {}

  explicit StringPrinter(StringTokenType token_type,
                         const SymbolTable *syms = 0)
      : StringPrinter(static_cast<TokenType>(token_type), syms) {}

  // Convert the FST 'fst' into the string 'output'
  bool operator()(const Fst<A> &fst, string *output) {
    bool is_a_string = FstToLabels(fst);
    if (!is_a_string) {
      VLOG(1) << "StringPrinter::operator(): Fst is not a string.";
      return false;
    }

    output->clear();

    if (token_type_ == SYMBOL) {
      std::stringstream sstrm;
      for (size_t i = 0; i < labels_.size(); ++i) {
        if (i) sstrm << *(FLAGS_fst_field_separator.rbegin());
        if (!PrintLabel(labels_[i], sstrm)) return false;
      }
      *output = sstrm.str();
    } else if (token_type_ == BYTE) {
      output->reserve(labels_.size());
      for (size_t i = 0; i < labels_.size(); ++i) {
        output->push_back(labels_[i]);
      }
    } else if (token_type_ == UTF8) {
      return LabelsToUTF8String(labels_, output);
    } else {
      VLOG(1) << "StringPrinter::operator(): Unknown token type: "
              << token_type_;
      return false;
    }
    return true;
  }

 private:
  bool FstToLabels(const Fst<A> &fst) {
    labels_.clear();

    StateId s = fst.Start();
    if (s == kNoStateId) {
      VLOG(2) << "StringPrinter::FstToLabels: Invalid starting state for "
              << "string fst.";
      return false;
    }

    while (fst.Final(s) == Weight::Zero()) {
      ArcIterator<Fst<A>> aiter(fst, s);
      if (aiter.Done()) {
        VLOG(2) << "StringPrinter::FstToLabels: String fst traversal does "
                << "not reach final state.";
        return false;
      }

      const A &arc = aiter.Value();
      labels_.push_back(arc.olabel);

      s = arc.nextstate;
      if (s == kNoStateId) {
        VLOG(2) << "StringPrinter::FstToLabels: Transition to invalid "
                << "state.";
        return false;
      }

      aiter.Next();
      if (!aiter.Done()) {
        VLOG(2) << "StringPrinter::FstToLabels: State with multiple "
                << "outgoing arcs found.";
        return false;
      }
    }

    return true;
  }

  bool PrintLabel(Label lab, ostream &ostrm) {
    if (syms_) {
      string symbol = syms_->Find(lab);
      if (symbol == "") {
        VLOG(2) << "StringPrinter::PrintLabel: Integer " << lab << " is not "
                << "mapped to any textual symbol, symbol table = "
                << syms_->Name();
        return false;
      }
      ostrm << symbol;
    } else {
      ostrm << lab;
    }
    return true;
  }

  TokenType token_type_;     // Token type: symbol, byte or utf8 encoded
  const SymbolTable *syms_;  // Symbol table used when token type is symbol
  std::vector<Label> labels_;  // Input FST labels.

  DISALLOW_COPY_AND_ASSIGN(StringPrinter);
};

}  // namespace fst

#endif  // FST_LIB_STRING_H_
