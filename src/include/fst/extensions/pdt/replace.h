// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Recursively replaces FST arcs with other FSTs, returning a PDT.

#ifndef FST_EXTENSIONS_PDT_REPLACE_H__
#define FST_EXTENSIONS_PDT_REPLACE_H__

#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <fst/replace.h>
#include <fst/replace-util.h>
#include <fst/symbol-table-ops.h>

namespace fst {

// Hash to paren IDs
template <typename S>
struct ReplaceParenHash {
  size_t operator()(const std::pair<size_t, S> &p) const {
    return p.first + p.second * kPrime;
  }

 private:
  static const size_t kPrime = 7853;
};

template <typename S>
const size_t ReplaceParenHash<S>::kPrime;

// PARSER TYPE: These characterize the PDT construction method. When
// applied to a CFG, each non-terminal is encoded as a DFA that
// accepts precisely the RHS's of productions of that non-terminal.
// For parsing (rather than just recognition), production numbers can
// used as outputs (placed as early as possible) in the DFAs promoted
// to DFTs. The 'strongly-regular' construction is inspired by Mohri
// and Pereira, "Dynamic compilation of weighted context-free
// grammars."  Proc. of ACL, 1998.
enum PdtParserType {
  // 'Top-down' construction. Applied to a simple LL(1) grammar (among others),
  // gives a DPDA. If promoted to a DPDT, with outputs being production
  // numbers, gives a leftmost derivation.  Left recursive grammars
  // problematic in use.
  PDT_LEFT_PARSER,

  // 'Top-down' construction. Similar to PDT_LEFT_PARSE except
  // bounded-stack (expandable as an FST) result with regular or, more
  // generally, 'strongly regular' grammars. Epsilons may replace some
  // parentheses, which may introduce some non-determinism.
  PDT_LEFT_SR_PARSER,

  /* TODO(riley):
  // 'Bottom-up' construction. Applied to a LR(0) grammar, gives a DPDA.
  // If promoted to a DPDT, with outputs being the production nubmers,
  // gives the reverse of a rightmost derivation.
  PDT_RIGHT_PARSER,
  */
};

template <class Arc>
struct PdtReplaceOptions {
  using Label = typename Arc::Label;

  explicit PdtReplaceOptions(Label root,
                             PdtParserType type = PDT_LEFT_PARSER,
                             Label start_paren_labels = kNoLabel,
                             string left_paren_prefix = "(_",
                             string right_paren_prefix = ")_") :
      root(root), type(type), start_paren_labels(start_paren_labels),
      left_paren_prefix(std::move(left_paren_prefix)),
      right_paren_prefix(std::move(right_paren_prefix)) {}

  Label root;
  PdtParserType type;
  Label start_paren_labels;
  const string left_paren_prefix;
  const string right_paren_prefix;
};

//
// PdtParser: Base PDT parser class common to specific parsers.
//

template <class Arc>
class PdtParser {
 public:
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef std::pair<Label, const Fst<Arc> *> LabelFstPair;
  typedef std::pair<Label, Label> LabelPair;
  typedef std::pair<Label, StateId> LabelStatePair;
  typedef std::pair<StateId, Weight> StateWeightPair;
  typedef std::pair<size_t, StateId> ParenKey;
  typedef std::unordered_map<ParenKey, size_t,
                             ReplaceParenHash<StateId>> ParenMap;

  PdtParser(const std::vector<LabelFstPair> &fst_array,
            const PdtReplaceOptions<Arc> &opts) :
      root_(opts.root), start_paren_labels_(opts.start_paren_labels),
      left_paren_prefix_(std::move(opts.left_paren_prefix)),
      right_paren_prefix_(std::move(opts.right_paren_prefix)),
      error_(false) {
    for (auto i = 0; i < fst_array.size(); ++i) {
      if (!CompatSymbols(fst_array[0].second->InputSymbols(),
                         fst_array[i].second->InputSymbols())) {
        FSTERROR() << "PdtParser: Input symbol table of input FST " << i
                   << " does not match input symbol table of 0th input FST";
        error_ = true;
      }
      if (!CompatSymbols(fst_array[0].second->OutputSymbols(),
                         fst_array[i].second->OutputSymbols())) {
        FSTERROR() << "PdtParser: Output symbol table of input FST " << i
                   << " does not match input symbol table of 0th input FST";
        error_ = true;
      }
      fst_array_.emplace_back(fst_array[i].first, fst_array[i].second->Copy());
      // Builds map from non-terminal label to FST ID.
      label2id_[fst_array[i].first] = i;
    }
  }

  virtual ~PdtParser() {
    for (auto i = 0; i < fst_array_.size(); ++i)
      delete fst_array_[i].second;
  }

  // Constructs the output PDT, dependent on the derived parser type.
  virtual void GetParser(MutableFst<Arc> *ofst,
                         std::vector<LabelPair> *parens) = 0;

 protected:
  const std::vector<LabelFstPair> &FstArray() const { return fst_array_; }
  Label Root() const { return root_; }

  // Maps from non-terminal label to corresponding FST ID.
  // Returns kNoStateId o.w.
  StateId Label2Id(Label l) const {
    auto it = label2id_.find(l);
    return it == label2id_.end() ? kNoStateId : it->second;
  }

  // Maps from output state to input FST label, state pair Returns
  // (kNoLabel, kNoStateId) if output does not map to any input.
  LabelStatePair GetLabelStatePair(StateId os) const {
    if (os >= label_state_pairs_.size()) {
      return LabelStatePair(kNoLabel, kNoStateId);
    } else {
      return label_state_pairs_[os];
    }
  }

  // Maps to output state from input FST label, state pair
  // Returns kNoStateId if input does not map to any output.
  StateId GetState(const LabelStatePair &lsp) const {
    auto it = state_map_.find(lsp);
    if (it == state_map_.end()) {
      return kNoStateId;
    } else {
      return it->second;
    }
  }

  // Builds single Fst combining all referenced input Fsts. Leaves in the
  // non-termnals for now.  Tabulates the PDT states that correspond to
  // the start and final states of the input Fsts and stores info in
  // 'open_dest' and 'close_src' respectively.
  void CreateFst(MutableFst<Arc> *ofst, std::vector<StateId> *open_dest,
                 std::vector<std::vector<StateWeightPair>> *close_src);

  // Assigns parenthesis labels from total allocated paren IDs.
  void AssignParenLabels(size_t total_nparens, std::vector<LabelPair> *parens) {
    parens->clear();
    for (auto paren_id = 0; paren_id < total_nparens; ++paren_id) {
      Label open_paren = start_paren_labels_ + paren_id;
      Label close_paren = open_paren + total_nparens;
      parens->emplace_back(open_paren, close_paren);
    }
  }

  // Determines how non-terminal instances are assigned parentheses Ids.
  virtual size_t AssignParenIds(const Fst<Arc> &ofst,
                                ParenMap *paren_map) const = 0;

  // Changes a non-terminal transition to an open parenthesis
  // transition redirected to the PDT state specified in 'open_dest',
  // when indexed by the input FST ID for the non-terminal. Adds close
  // parenthesis transitions (with specified weights) from the PDT
  // states specified in 'close_src', when indexed by the input FST Id
  // for the non-terminal, to the former destination state of the
  // non-terminal transition. The 'paren_map' gives the parenthesis ID
  // for a given non-terminal FST ID and destination state pair.
  // The vector 'close_non_term_weight' specifies non-terminals for
  // which the non-terminal arc weight should be applied on the close
  // parenthesis (multiplying the 'close_src' weight above) rather
  // than on the open parenthesis.  If no paren ID is found, then an
  // epsilon replaces the parenthesis that would carry the
  // non-terminal arc weight and the other parenthesis is omitted
  // (appropriate for the strongly-regular case).
  void AddParensToFst(
      const std::vector<LabelPair> &parens,
      const ParenMap &paren_map,
      const std::vector<StateId> &open_dest,
      const std::vector<std::vector<StateWeightPair>> &close_src,
      const std::vector<bool> &close_non_term_weight,
      MutableFst<Arc> *ofst);

  // Ensures that parentheses arcs are added to the symbol table.
  void AddParensToSymbolTables(const std::vector<LabelPair> &parens,
                               MutableFst<Arc> *ofst);

 private:
  std::vector<LabelFstPair> fst_array_;
  Label root_;
  // Index to use for the first parenthesis.
  Label start_paren_labels_;
  const string left_paren_prefix_;
  const string right_paren_prefix_;
  // maps from non-terminal label to FST ID.
  std::unordered_map<Label, StateId> label2id_;
  // Given an output state, specifies the input FST label, state pair.
  std::vector<LabelStatePair> label_state_pairs_;
  // Given an Fst label, state pair, specifies the output FST state ID.
  std::map<LabelStatePair, StateId> state_map_;
  bool error_;
};

template <class Arc>
void PdtParser<Arc>::CreateFst(
    MutableFst<Arc> *ofst, std::vector<StateId> *open_dest,
    std::vector<std::vector<StateWeightPair>> *close_src) {
  ofst->DeleteStates();
  if (error_) {
    ofst->SetProperties(kError, kError);
    return;
  }

  open_dest->resize(fst_array_.size(), kNoStateId);
  close_src->resize(fst_array_.size());

  // Queue of non-terminals to replace
  std::deque<Label> non_term_queue;
  non_term_queue.push_back(root_);

  // Has a non-terminal been enqueued?
  std::vector<bool> enqueued(fst_array_.size(), false);
  enqueued[label2id_[root_]] = true;

  Label max_label = kNoLabel;
  for (StateId soff = 0; !non_term_queue.empty(); soff = ofst->NumStates()) {
    Label label = non_term_queue.front();
    non_term_queue.pop_front();
    StateId fst_id = Label2Id(label);

    const Fst<Arc> *ifst = fst_array_[fst_id].second;
    for (StateIterator<Fst<Arc>> siter(*ifst); !siter.Done(); siter.Next()) {
      StateId is = siter.Value();
      StateId os = ofst->AddState();
      LabelStatePair lsp(label, is);
      label_state_pairs_.push_back(lsp);
      state_map_[lsp] = os;
      if (is == ifst->Start()) {
        (*open_dest)[fst_id] = os;
        if (label == root_) ofst->SetStart(os);
      }
      if (ifst->Final(is) != Weight::Zero()) {
        if (label == root_) ofst->SetFinal(os, ifst->Final(is));
        (*close_src)[fst_id].emplace_back(os, ifst->Final(is));
      }
      for (ArcIterator<Fst<Arc>> aiter(*ifst, is); !aiter.Done();
           aiter.Next()) {
        Arc arc = aiter.Value();
        arc.nextstate += soff;
        if (max_label == kNoLabel || arc.olabel > max_label)
          max_label = arc.olabel;
        StateId nfst_id = Label2Id(arc.olabel);
        if (nfst_id != kNoStateId) {
          if (fst_array_[nfst_id].second->Start() == kNoStateId) continue;
          if (!enqueued[nfst_id]) {
            non_term_queue.push_back(arc.olabel);
            enqueued[nfst_id] = true;
          }
        }
        ofst->AddArc(os, arc);
      }
    }
  }
  if (start_paren_labels_ == kNoLabel)
    start_paren_labels_ = max_label + 1;
}

template <class Arc>
void PdtParser<Arc>::AddParensToFst(
    const std::vector<LabelPair> &parens,
    const ParenMap &paren_map,
    const std::vector<StateId> &open_dest,
    const std::vector<std::vector<StateWeightPair>> &close_src,
    const std::vector<bool> &close_non_term_weight,
    MutableFst<Arc> *ofst) {
  StateId dead_state = kNoStateId;
  typedef MutableArcIterator<MutableFst<Arc>> MIter;
  for (StateIterator<Fst<Arc>> siter(*ofst); !siter.Done(); siter.Next()) {
    StateId os = siter.Value();
    std::unique_ptr<MIter> aiter(new MIter(ofst, os));
    for (auto n = 0; !aiter->Done(); aiter->Next(), ++n) {
      Arc arc = aiter->Value();
      StateId nfst_id = Label2Id(arc.olabel);
      if (nfst_id != kNoStateId) {
        // Gets parentheses.
        ParenKey paren_key(nfst_id, arc.nextstate);
        auto it = paren_map.find(paren_key);
        Label open_paren = 0;
        Label close_paren = 0;
        if (it != paren_map.end()) {
          size_t paren_id = it->second;
          open_paren = parens[paren_id].first;
          close_paren = parens[paren_id].second;
        }
        // Sets open parenthesis.
        if (open_paren != 0 || !close_non_term_weight[nfst_id]) {
          Weight open_weight = close_non_term_weight[nfst_id] ?
              Weight::One() : arc.weight;
          Arc sarc(open_paren, open_paren, open_weight, open_dest[nfst_id]);
          aiter->SetValue(sarc);
        } else {
          if (dead_state == kNoStateId)
            dead_state = ofst->AddState();
          Arc sarc(0, 0, Weight::One(), dead_state);
          aiter->SetValue(sarc);
        }
        // Adds close parentheses.
        if (close_paren != 0 || close_non_term_weight[nfst_id]) {
          for (size_t i = 0; i < close_src[nfst_id].size(); ++i) {
            const StateWeightPair &p = close_src[nfst_id][i];
            Weight close_weight = close_non_term_weight[nfst_id] ?
                Times(arc.weight, p.second) : p.second;
            Arc farc(close_paren, close_paren, close_weight, arc.nextstate);

            ofst->AddArc(p.first, farc);
            if (os == p.first) {  // Invalidated iterator
              aiter.reset(new MIter(ofst, os));
              aiter->Seek(n);
            }
          }
        }
      }
    }
  }
}

template <class Arc>
void PdtParser<Arc>::AddParensToSymbolTables(
    const std::vector<LabelPair> &parens, MutableFst<Arc> *ofst) {
  auto size = parens.size();
  if (ofst->InputSymbols()) {
    if (!AddAuxiliarySymbols(left_paren_prefix_, start_paren_labels_, size,
                             ofst->MutableInputSymbols())) {
      ofst->SetProperties(kError, kError);
      return;
    }
    if (!AddAuxiliarySymbols(right_paren_prefix_, start_paren_labels_ + size,
                             size, ofst->MutableInputSymbols())) {
      ofst->SetProperties(kError, kError);
      return;
    }
  }
  if (ofst->OutputSymbols()) {
    if (!AddAuxiliarySymbols(left_paren_prefix_, start_paren_labels_, size,
                             ofst->MutableOutputSymbols())) {
      ofst->SetProperties(kError, kError);
      return;
    }
    if (!AddAuxiliarySymbols(right_paren_prefix_, start_paren_labels_ + size,
                             size, ofst->MutableOutputSymbols())) {
      ofst->SetProperties(kError, kError);
      return;
    }
  }
}

//
// PdtLeftParser: Builds a PDT by recursive replacement 'top-down'
// where the 'call' and 'return' are encoded in the parentheses.
//

template <class Arc>
class PdtLeftParser : public PdtParser<Arc> {
 public:
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename PdtParser<Arc>::LabelFstPair LabelFstPair;
  typedef typename PdtParser<Arc>::LabelPair LabelPair;
  typedef typename PdtParser<Arc>::LabelStatePair LabelStatePair;
  typedef typename PdtParser<Arc>::StateWeightPair StateWeightPair;
  typedef typename PdtParser<Arc>::ParenKey ParenKey;
  typedef typename PdtParser<Arc>::ParenMap ParenMap;

  using PdtParser<Arc>::AddParensToFst;
  using PdtParser<Arc>::AddParensToSymbolTables;
  using PdtParser<Arc>::AssignParenLabels;
  using PdtParser<Arc>::CreateFst;
  using PdtParser<Arc>::FstArray;
  using PdtParser<Arc>::GetLabelStatePair;
  using PdtParser<Arc>::GetState;
  using PdtParser<Arc>::Label2Id;
  using PdtParser<Arc>::Root;

  PdtLeftParser(const std::vector<LabelFstPair> &fst_array,
                const PdtReplaceOptions<Arc> &opts) :
      PdtParser<Arc>(fst_array, opts) { }

  void GetParser(MutableFst<Arc> *ofst,
                 std::vector<LabelPair> *parens) override;

 protected:
  // Assigns a unique parenthesis ID for each non-terminal, destination
  // state pair.
  size_t AssignParenIds(const Fst<Arc> &ofst,
                        ParenMap *paren_map) const override;
};

template <class Arc>
void PdtLeftParser<Arc>::GetParser(
    MutableFst<Arc> *ofst,
    std::vector<LabelPair> *parens) {
  ofst->DeleteStates();
  parens->clear();
  const std::vector<LabelFstPair> &fst_array = FstArray();

  // Map that gives the paren ID for a (non-terminal, dest. state) pair
  // (which can be unique).
  ParenMap paren_map;
  // Specifies the open parenthesis destination state for a given
  // non-terminal. The source is the non-terminal instance
  // source state.
  std::vector<StateId> open_dest(fst_array.size(), kNoStateId);
  // Specifies close parenthesis source states and weights for a given
  // non-terminal. The destination is the non-terminal instance
  // destination state.
  std::vector<std::vector<StateWeightPair>> close_src(fst_array.size());
  // Specifies non-terminals for which the non-terminal arc weight
  // should be applied on the close parenthesis (multiplying the
  // 'close_src' weight above) rather than on the open parenthesis.
  std::vector<bool> close_non_term_weight(fst_array.size(), false);

  CreateFst(ofst, &open_dest, &close_src);
  size_t total_nparens = AssignParenIds(*ofst, &paren_map);
  AssignParenLabels(total_nparens, parens);
  AddParensToFst(*parens, paren_map, open_dest, close_src,
                 close_non_term_weight, ofst);
  if (!fst_array.empty()) {
    ofst->SetInputSymbols(fst_array[0].second->InputSymbols());
    ofst->SetOutputSymbols(fst_array[0].second->OutputSymbols());
  }
  AddParensToSymbolTables(*parens, ofst);
}

template <class Arc>
size_t PdtLeftParser<Arc>::AssignParenIds(
    const Fst<Arc> &ofst,
    ParenMap *paren_map) const {
  // # of distinct parenthesis pairs per fst.
  std::vector<size_t> nparens(FstArray().size(), 0);
  // # of distinct parenthesis pairs overall.
  size_t total_nparens = 0;

  for (StateIterator<Fst<Arc>> siter(ofst); !siter.Done(); siter.Next()) {
    StateId os = siter.Value();
    for (ArcIterator<Fst<Arc>> aiter(ofst, os); !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      StateId nfst_id = Label2Id(arc.olabel);
      if (nfst_id != kNoStateId) {
        ParenKey paren_key(nfst_id, arc.nextstate);
        auto pit = paren_map->find(paren_key);
        if (pit == paren_map->end()) {
          // Assigns new paren ID for this fst, dest state pair.
          (*paren_map)[paren_key] = nparens[nfst_id]++;
          if (nparens[nfst_id] > total_nparens)
            total_nparens = nparens[nfst_id];
        }
      }
    }
  }
  return total_nparens;
}

//
// PdtLeftSRParser: Similar to PdtLeftParser but (a) uses epsilons
// rather than parenthesis labels for any non-terminal instances
// within a (left-/right-) linear dependency SCC and (b) allocates a paren
// ID uniquely for each such dependency SCC (rather than
// non-terminal = dependency state) and destination state.
//

template <class Arc>
class PdtLeftSRParser : public PdtParser<Arc> {
 public:
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename PdtParser<Arc>::LabelFstPair LabelFstPair;
  typedef typename PdtParser<Arc>::LabelPair LabelPair;
  typedef typename PdtParser<Arc>::LabelStatePair LabelStatePair;
  typedef typename PdtParser<Arc>::StateWeightPair StateWeightPair;
  typedef typename PdtParser<Arc>::ParenKey ParenKey;
  typedef typename PdtParser<Arc>::ParenMap ParenMap;

  using PdtParser<Arc>::AddParensToFst;
  using PdtParser<Arc>::AddParensToSymbolTables;
  using PdtParser<Arc>::AssignParenLabels;
  using PdtParser<Arc>::CreateFst;
  using PdtParser<Arc>::FstArray;
  using PdtParser<Arc>::GetLabelStatePair;
  using PdtParser<Arc>::GetState;
  using PdtParser<Arc>::Label2Id;
  using PdtParser<Arc>::Root;

  PdtLeftSRParser(const std::vector<LabelFstPair> &fst_array,
                  const PdtReplaceOptions<Arc> &opts) :
      PdtParser<Arc>(fst_array, opts),
      replace_util_(fst_array, ReplaceUtilOptions(opts.root)) { }

  void GetParser(MutableFst<Arc> *ofst,
                 std::vector<LabelPair> *parens) override;

 protected:
  // Assigns a unique parenthesis ID for each non-terminal, destination
  // state pair when the non-terminal refers to a 'non-linear' FST.
  // Otherwise, assigns a unique parenthesis ID for each dependency SCC,
  // destination state pair if the non-terminal instance is between
  // SCCs. Otherwise does nothing.
  size_t AssignParenIds(const Fst<Arc> &ofst,
                        ParenMap *paren_map) const override;

  // Returns dependency SCC for given label.
  size_t SCC(Label label) const { return replace_util_.SCC(label); }

  // Is a given dependency SCC left-linear?
  bool SCCLeftLinear(size_t scc_id) const {
    const uint32 ll_props = kReplaceSCCLeftLinear | kReplaceSCCNonTrivial;
    uint32 scc_props = replace_util_.SCCProperties(scc_id);
    return (scc_props & ll_props) == ll_props;
  }

  // Is a given dependency SCC right-linear?
  bool SCCRightLinear(size_t scc_id) const {
    const uint32 lr_props = kReplaceSCCRightLinear | kReplaceSCCNonTrivial;
    uint32 scc_props = replace_util_.SCCProperties(scc_id);
    return (scc_props & lr_props) == lr_props;
  }

  // Components of (left-/right-) linear dependency SCC; empty o.w.
  const std::vector<size_t> &SCCComps(size_t scc_id) const {
    if (scc_comps_.empty())
      GetSCCComps();
    return scc_comps_[scc_id];
  }

  // Returns the representative state of an SCC:
  //  For left-linear,  it is one of the initial states.
  //  For right-linear, it is one of the non-terminal destination states.
  //  O.w. it is kNoStateId.
  StateId RepState(size_t scc_id) const {
    if (SCCComps(scc_id).empty())
      return kNoStateId;
    StateId fst_id = SCCComps(scc_id).front();
    const std::vector<LabelFstPair> &fst_array = FstArray();
    Label label = fst_array[fst_id].first;
    const Fst<Arc> *ifst = fst_array[fst_id].second;
    if (SCCLeftLinear(scc_id)) {
      LabelStatePair lsp(label, ifst->Start());
      return GetState(lsp);
    } else {  // right-linear
      LabelStatePair lsp(label, *NonTermDests(fst_id).begin());
      return GetState(lsp);
    }
    return 0;
  }

 private:
  // Merges initial (final) states of in a left- (right-) linear dependency
  // SCC after dealing with the non-terminal arc and final weights.
  void ProcSCCs(MutableFst<Arc> *ofst,
                std::vector<StateId> *open_dest,
                std::vector<std::vector<StateWeightPair>> *close_src,
                std::vector<bool> *close_non_term_weight) const;


  // Computes components of (left-/right-) linear dependency SCC
  void GetSCCComps() const {
    const std::vector<LabelFstPair> &fst_array = FstArray();
    for (size_t i = 0; i < fst_array.size(); ++i) {
      Label label = fst_array[i].first;
      size_t scc_id = SCC(label);
      if (scc_comps_.size() <= scc_id)
        scc_comps_.resize(scc_id + 1);
      if (SCCLeftLinear(scc_id) || SCCRightLinear(scc_id))
        scc_comps_[scc_id].push_back(i);
    }
  }

  const std::set<StateId> &NonTermDests(StateId fst_id) const {
    if (non_term_dests_.empty())
      GetNonTermDests();
    return non_term_dests_[fst_id];
  }

  // Finds non-terminal destination states for right-linear FSTS. Does
  // nothing o.w.
  void GetNonTermDests() const;

  // For dependency SCC info
  mutable ReplaceUtil<Arc> replace_util_;
  // Components of (left-/right-) linear dependency SCCs; empty o.w.
  mutable std::vector<std::vector<size_t>> scc_comps_;
  // States that have non-terminals entering them for each (right-linear) FST.
  mutable std::vector<std::set<StateId>> non_term_dests_;
};

template <class Arc>
void PdtLeftSRParser<Arc>::GetParser(
    MutableFst<Arc> *ofst,
    std::vector<LabelPair> *parens) {
  ofst->DeleteStates();
  parens->clear();
  const std::vector<LabelFstPair> &fst_array = FstArray();

  // Map that gives the paren ID for a (non-terminal, dest. state) pair
  // (which can be unique).
  ParenMap paren_map;
  // Specifies the open parenthesis destination state for a given
  // non-terminal. The source is the non-terminal instance
  // source state.
  std::vector<StateId> open_dest(fst_array.size(), kNoStateId);
  // Specifies close parenthesis source states and weights for a given
  // non-terminal. The destination is the non-terminal instance
  // destination state.
  std::vector<std::vector<StateWeightPair>> close_src(fst_array.size());
  // Specifies non-terminals for which the non-terminal arc weight
  // should be applied on the close parenthesis (multiplying the
  // 'close_src' weight above) rather than on the open parenthesis.
  std::vector<bool> close_non_term_weight(fst_array.size(), false);

  CreateFst(ofst, &open_dest, &close_src);
  ProcSCCs(ofst, &open_dest, &close_src, &close_non_term_weight);
  size_t total_nparens = AssignParenIds(*ofst, &paren_map);
  AssignParenLabels(total_nparens, parens);
  AddParensToFst(*parens, paren_map, open_dest, close_src,
                 close_non_term_weight, ofst);
  if (!fst_array.empty()) {
    ofst->SetInputSymbols(fst_array[0].second->InputSymbols());
    ofst->SetOutputSymbols(fst_array[0].second->OutputSymbols());
  }
  AddParensToSymbolTables(*parens, ofst);
  Connect(ofst);
}

template <class Arc>
void PdtLeftSRParser<Arc>::ProcSCCs(
    MutableFst<Arc> *ofst,
    std::vector<StateId> *open_dest,
    std::vector<std::vector<StateWeightPair>> *close_src,
    std::vector<bool> *close_non_term_weight) const {
  const std::vector<LabelFstPair> &fst_array = FstArray();

  for (StateIterator<Fst<Arc>> siter(*ofst); !siter.Done(); siter.Next()) {
    StateId os = siter.Value();
    Label label = GetLabelStatePair(os).first;
    StateId is = GetLabelStatePair(os).second;
    StateId fst_id = Label2Id(label);
    size_t scc_id = SCC(label);
    StateId rs = RepState(scc_id);
    const Fst<Arc> *ifst = fst_array[fst_id].second;

    // SCC LEFT-LINEAR: puts non-terminal weights on close parentheses.
    // Merges initial states into SCC representative state and
    // updates open_dest.
    if (SCCLeftLinear(scc_id)) {
      (*close_non_term_weight)[fst_id] = true;
      if (is == ifst->Start() && os != rs) {
        for (ArcIterator<Fst<Arc>> aiter(*ofst, os);
             !aiter.Done();
             aiter.Next()) {
          const Arc &arc = aiter.Value();
          ofst->AddArc(rs, arc);
        }
        ofst->DeleteArcs(os);
        if (os == ofst->Start())
          ofst->SetStart(rs);
        (*open_dest)[fst_id] = rs;
      }
    }

    // SCC RIGHT-LINEAR: pushes back final weights onto non-terminals, if
    // possible, or o.w. adds weighted epsilons to the SCC representative
    // state. Merges final states into SCC representative state and updates
    // close_src.
    if (SCCRightLinear(scc_id)) {
      for (MutableArcIterator<MutableFst<Arc>> aiter(ofst, os);
           !aiter.Done();
           aiter.Next()) {
        Arc arc = aiter.Value();
        StateId idest = GetLabelStatePair(arc.nextstate).second;
        if (NonTermDests(fst_id).count(idest) > 0) {
          if (ofst->Final(arc.nextstate) != Weight::Zero()) {
            ofst->SetFinal(arc.nextstate, Weight::Zero());
            ofst->SetFinal(rs, Weight::One());
          }
          arc.weight = Times(arc.weight, ifst->Final(idest));
          arc.nextstate = rs;
          aiter.SetValue(arc);
        }
      }
      Weight final_weight = ifst->Final(is);
      if (final_weight != Weight::Zero() &&
          NonTermDests(fst_id).count(is) == 0) {
        Arc arc(0, 0, final_weight, rs);
        ofst->AddArc(os, arc);
        if (ofst->Final(os) != Weight::Zero()) {
          ofst->SetFinal(os, Weight::Zero());
          ofst->SetFinal(rs, Weight::One());
        }
      }
      if (is == ifst->Start()) {
        (*close_src)[fst_id].clear();
        (*close_src)[fst_id].emplace_back(rs, Weight::One());
      }
    }
  }
}

template <class Arc>
void PdtLeftSRParser<Arc>::GetNonTermDests() const {
  const std::vector<LabelFstPair> &fst_array = FstArray();
  non_term_dests_.resize(fst_array.size());
  for (size_t fst_id = 0; fst_id < fst_array.size(); ++fst_id) {
    Label label = fst_array[fst_id].first;
    size_t scc_id = SCC(label);
    if (SCCRightLinear(scc_id)) {
      const Fst<Arc> *ifst = fst_array[fst_id].second;
      for (StateIterator<Fst<Arc>> siter(*ifst); !siter.Done(); siter.Next()) {
        StateId is = siter.Value();
        for (ArcIterator<Fst<Arc>> aiter(*ifst, is); !aiter.Done();
             aiter.Next()) {
          Arc arc = aiter.Value();
          if (Label2Id(arc.olabel) != kNoStateId)
            non_term_dests_[fst_id].insert(arc.nextstate);
        }
      }
    }
  }
}

template <class Arc>
size_t PdtLeftSRParser<Arc>::AssignParenIds(
    const Fst<Arc> &ofst,
    ParenMap *paren_map) const {
  const std::vector<LabelFstPair> &fst_array = FstArray();

  // # of distinct parenthesis pairs per fst.
  std::vector<size_t> nparens(fst_array.size(), 0);
  // # of distinct parenthesis pairs overall.
  size_t total_nparens = 0;

  for (StateIterator<Fst<Arc>> siter(ofst); !siter.Done(); siter.Next()) {
    StateId os = siter.Value();
    Label label = GetLabelStatePair(os).first;
    size_t scc_id = SCC(label);

    for (ArcIterator<Fst<Arc>> aiter(ofst, os); !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      StateId nfst_id = Label2Id(arc.olabel);
      if (nfst_id != kNoStateId) {
        size_t nscc_id = SCC(arc.olabel);
        bool nscc_linear = !SCCComps(nscc_id).empty();
        // Assigns a parenthesis ID for the non-terminal transition
        // if the non-terminal belongs to a (left-/right-) linear dependency
        // SCC or if the transition is in an FST from a different SCC
        if (!nscc_linear || scc_id != nscc_id) {
          // For (left-/right-) linear SCCs instead of using nfst_id, we
          // will use its SCC prototype pfst_id for assigning distinct
          // parenthesis IDs.
          size_t pfst_id = nscc_linear ? SCCComps(nscc_id).front() : nfst_id;
          ParenKey paren_key(pfst_id, arc.nextstate);
          auto pit = paren_map->find(paren_key);
          if (pit == paren_map->end()) {
            // Assigns new paren ID for this fst/SCC, dest state pair.
            if (nscc_linear) {
              // This is mapping we'll need, but we also store (harmlessly)
              // for the prototype below so we can easily keep count per SCC.
              ParenKey nparen_key(nfst_id, arc.nextstate);
              (*paren_map)[nparen_key] = nparens[pfst_id];
            }
            (*paren_map)[paren_key] = nparens[pfst_id]++;
            if (nparens[pfst_id] > total_nparens)
              total_nparens = nparens[pfst_id];
          }
        }
      }
    }
  }
  return total_nparens;
}


// Builds a pushdown transducer (PDT) from an RTN specification
// identical to that in fst/lib/replace.h. The result is a PDT
// encoded as the FST 'ofst' where some transitions are labeled with
// open or close parentheses. To be interpreted as a PDT, the parens
// must balance on a path (see PdtExpand()). The open/close
// parenthesis label pairs are returned in 'parens'.

template <class Arc>
void Replace(
    const std::vector<std::pair<typename Arc::Label, const Fst<Arc> *>>
        &ifst_array,
    MutableFst<Arc> *ofst,
    std::vector<std::pair<typename Arc::Label, typename Arc::Label>> *parens,
    const PdtReplaceOptions<Arc> &opts) {
  switch (opts.type) {
    case PDT_LEFT_PARSER:
      {
        PdtLeftParser<Arc> pr(ifst_array, opts);
        pr.GetParser(ofst, parens);
        return;
      }
    case PDT_LEFT_SR_PARSER:
      {
        PdtLeftSRParser<Arc> pr(ifst_array, opts);
        pr.GetParser(ofst, parens);
        return;
      }
    default:
      FSTERROR() << "Replace: Bad PdtParserType: " << opts.type;
      ofst->DeleteStates();
      ofst->SetProperties(kError, kError);
      parens->clear();
      return;
  }
}

template <class Arc>
void Replace(
    const std::vector<std::pair<typename Arc::Label, const Fst<Arc> *>>
        &ifst_array,
    MutableFst<Arc> *ofst,
    std::vector<std::pair<typename Arc::Label, typename Arc::Label>> *parens,
    typename Arc::Label root) {
  PdtReplaceOptions<Arc> opts(root);
  Replace(ifst_array, ofst, parens, opts);
}

}  // namespace fst

#endif  // FST_EXTENSIONS_PDT_REPLACE_H__
