// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Expands a PDT to an FST.

#ifndef FST_EXTENSIONS_PDT_REVERSE_H__
#define FST_EXTENSIONS_PDT_REVERSE_H__

#include <unordered_map>
#include <vector>

#include <fst/mutable-fst.h>
#include <fst/relabel.h>
#include <fst/reverse.h>

namespace fst {

// Reverses a pushdown transducer (PDT) encoded as an FST.
template <class Arc, class RevArc>
void Reverse(const Fst<Arc> &ifst,
             const std::vector<
                 std::pair<typename Arc::Label, typename Arc::Label>> &parens,
             MutableFst<RevArc> *ofst) {
  typedef typename Arc::Label Label;

  // Reverses FST
  Reverse(ifst, ofst);

  // Exchanges open and close parenthesis pairs
  std::vector<std::pair<Label, Label>> relabel_pairs;
  for (size_t i = 0; i < parens.size(); ++i) {
    relabel_pairs.push_back(std::make_pair(parens[i].first, parens[i].second));
    relabel_pairs.push_back(std::make_pair(parens[i].second, parens[i].first));
  }
  Relabel(ofst, relabel_pairs, relabel_pairs);
}

}  // namespace fst

#endif  // FST_EXTENSIONS_PDT_REVERSE_H__
