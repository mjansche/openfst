// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Common classes for Multi Pushdown Transducer (MPDT) expansion/traversal.

#ifndef FST_EXTENSIONS_MPDT_MPDT_H__
#define FST_EXTENSIONS_MPDT_MPDT_H__

#include <map>
#include <vector>

#include <fst/compat.h>
#include <fst/extensions/pdt/pdt.h>

namespace fst {

enum MPdtType {
  MPDT_READ_RESTRICT,   // Can only read from first empty stack
  MPDT_WRITE_RESTRICT,  // Can only write to first empty stack
  MPDT_NO_RESTRICT,     // No read-write restrictions
};

// PLEASE READ THIS CAREFULLY:
//
// When USEVECTOR is set, the stack configurations --- the statewise
// representation of the StackId's for each substack --- is stored in a vector.
// I would like to do this using an array for efficiency reasons, thus the
// definition of StackConfig below. However, while this *works* in that tests
// pass, etc. It causes a memory leak in the compose and expand tests, evidently
// in the map[] that is being used to store the mapping between these
// StackConfigs and the external StackId that the caller sees. There are no
// memory leaks when I use a vector, only with this StackId. Why there should be
// memory leaks given that I am not mallocing anything is a mystery. In case you
// were wondering, clearing the map at the end does not help.

template <typename K, int nlevels>
struct StackConfig {
  StackConfig() : array_() {}

  StackConfig(const StackConfig<K, nlevels> &config) {
    for (int i = 0; i < nlevels; ++i) array_[i] = config.array_[i];
  }

  K &operator[](const int index) { return array_[index]; }

  const K &operator[](const int index) const { return array_[index]; }

  StackConfig &operator=(const StackConfig<K, nlevels> &config) {
    if (this == &config) return *this;
    for (int i = 0; i < nlevels; ++i) array_[i] = config.array_[i];
    return *this;
  }

  K array_[nlevels];
};

template <typename K, int nlevels>
class CompConfig {
 public:
  bool operator()(const StackConfig<K, nlevels> &a,
                  const StackConfig<K, nlevels> &b) const {
    for (size_t i = 0; i < nlevels; ++i) {
      if (a.array_[i] < b.array_[i]) {
        return true;
      } else if (a.array_[i] > b.array_[i]) {
        return false;
      }
    }
    return false;
  }
};

// Defines the KeyPair type used as the key to MPdtStack.paren_id_map_. The hash
// function is provided as a separate struct to match templating syntax.
template <typename L>
struct KeyPair {
  L level;
  size_t underlying_id;
  KeyPair(L lev, size_t id) : level(lev), underlying_id(id) {}
  inline bool operator==(const KeyPair<L> &rhs) const {
    return level == rhs.level && underlying_id == rhs.underlying_id;
  }
};

template <typename L>
struct KeyPairHasher {
  inline size_t operator()(const KeyPair<L> &keypair) const {
    return std::hash<L>()(keypair.level) ^
           (hash<size_t>()(keypair.underlying_id) << 1);
  }
};

template <typename K, typename L, L nlevels = 2,
          MPdtType restrict = MPDT_READ_RESTRICT>
class MPdtStack {
 public:
  typedef K StackId;
  typedef L Label;
  typedef L Level;  // Not really needed, but I find it confusing otherwise.
  typedef StackConfig<StackId, nlevels> Config;
  typedef std::map<Config, StackId, CompConfig<StackId, nlevels>>
      ConfigToStackId;

  MPdtStack(const std::vector<std::pair<Label, Label>> &parens,
            const std::vector<int> &assignments);

  MPdtStack(const MPdtStack &mstack);

  ~MPdtStack() {
    for (Level i = 0; i < nlevels; ++i) delete stacks_[i];
  }

  StackId Find(StackId stack_id, Label label);

  // For now we do not implement Pop since this is needed only for
  // ShortestPath()
  // For Top we find the first non-empty config, and find the paren id of that
  // (or -1) if there is none. Then map that to the external stack_id to return
  ssize_t Top(StackId stack_id) const {
    if (stack_id == -1) return -1;
    const Config config = InternalStackIds(stack_id);
    Level lev = 0;
    StackId underlying_id = -1;
    for (; lev < nlevels; ++lev) {
      if (!Empty(config, lev)) {
        underlying_id = stacks_[lev]->Top(config[lev]);
        break;
      }
    }
    if (underlying_id == -1) return -1;
    typename std::unordered_map<KeyPair<Level>, size_t,
                                KeyPairHasher<L>>::const_iterator ix =
        paren_id_map_.find(KeyPair<Level>(lev, underlying_id));
    if (ix == paren_id_map_.end()) return -1;  // NB: shouldn't happen
    return ix->second;
  }

  ssize_t ParenId(Label label) const {
    typename std::unordered_map<Label, size_t>::const_iterator pit =
        paren_map_.find(label);
    if (pit == paren_map_.end())  // Non-paren.
      return -1;
    return pit->second;
  }

  // TODO(rws): For debugging purposes only: remove later
  string PrintConfig(const Config &config) const {
    string result = "[";
    for (Level i = 0; i < nlevels; ++i) {
      char s[128];
      snprintf(s, sizeof(s), "%d", config[i]);
      result += string(s);
      if (i < nlevels - 1) result += ", ";
    }
    result += "]";
    return result;
  }

  bool Error() { return error_; }

  // Each of the component stacks keeps its own stack id for a given
  // configuration and label. This function relates a configuration of those to
  // the stack id that the caller of the mpdt sees.
  inline StackId ExternalStackId(const Config &config) {
    typename ConfigToStackId::const_iterator idx =
        config_to_stack_id_map_.find(config);
    StackId result;
    if (idx == config_to_stack_id_map_.end()) {
      result = next_stack_id_++;
      config_to_stack_id_map_.insert(
          std::pair<Config, StackId>(config, result));
      stack_id_to_config_map_[result] = config;
    } else {
      result = idx->second;
    }
    return result;
  }

  // This function gets the internal stack id from an external stack id
  inline const Config InternalStackIds(StackId stack_id) const {
    typename std::unordered_map<StackId, Config>::const_iterator idx =
        stack_id_to_config_map_.find(stack_id);
    if (idx == stack_id_to_config_map_.end())
      idx = stack_id_to_config_map_.find(-1);
    return idx->second;
  }

  inline bool Empty(const Config &config, Level lev) const {
    return config[lev] <= 0;
  }

  inline bool AllEmpty(const Config &config) {
    for (Level lev = 0; lev < nlevels; ++lev)
      if (!Empty(config, lev)) return false;
    return true;
  }

  bool error_;
  Label min_paren_;                      // For faster paren. check
  Label max_paren_;                      // For faster paren. check
  std::unordered_map<Label, Label>
      paren_levels_;                             // Stores "level" of each paren
  std::vector<std::pair<Label, Label>> parens_;  // As in pdt.h
  std::unordered_map<Label, size_t> paren_map_;  // As in pdt.h
  // Map between internal paren_id and external paren_id
  std::unordered_map<KeyPair<Level>, size_t, KeyPairHasher<Level>>
      paren_id_map_;
  // Maps between internal stack ids and external stack id.
  ConfigToStackId config_to_stack_id_map_;
  std::unordered_map<StackId, Config> stack_id_to_config_map_;
  StackId next_stack_id_;
  // Underlying stacks
  PdtStack<StackId, Label> *stacks_[nlevels];  // Array of stacks
};

template <typename K, typename L, L nlevels, MPdtType restrict>
MPdtStack<K, L, nlevels, restrict>::MPdtStack(
    const std::vector<std::pair<L, L>> &parens,
    const std::vector<int> &assignments)
    : error_(false),
      min_paren_(kNoLabel),
      max_paren_(kNoLabel),
      parens_(parens),
      next_stack_id_(1) {
  typedef K StackId;
  typedef L Label;
  typedef L Level;  // Not really needed, but I find it confusing otherwise.
  typedef StackConfig<StackId, nlevels> Config;
  typedef std::map<Config, StackId, CompConfig<StackId, nlevels>>
      ConfigToStackId;
  if (parens.size() != assignments.size()) {
    FSTERROR() << "MPdtStack: Parens of different size from assignments";
    error_ = true;
    return;
  }
  std::vector<std::pair<Label, Label>> vectors[nlevels];
  for (int i = 0; i < assignments.size(); ++i) {
    // Assignments here start at 0, so assuming the human-readable version has
    // them starting at 1, we should subtract 1 here
    Level lev = assignments[i] - 1;
    if (lev < 0 || lev >= nlevels) {
      FSTERROR() << "MPdtStack: Specified level " << lev << " out of bounds";
      error_ = true;
      return;
    }
    const std::pair<Label, Label> &p = parens[i];
    vectors[lev].push_back(p);
    paren_levels_[p.first] = lev;
    paren_levels_[p.second] = lev;
    paren_map_[p.first] = i;
    paren_map_[p.second] = i;
    KeyPair<Level> q(lev, vectors[lev].size() - 1);
    paren_id_map_[q] = i;

    if (min_paren_ == kNoLabel || p.first < min_paren_) min_paren_ = p.first;
    if (p.second < min_paren_) min_paren_ = p.second;

    if (max_paren_ == kNoLabel || p.first > max_paren_) max_paren_ = p.first;
    if (p.second > max_paren_) max_paren_ = p.second;
  }
  Config neg_one;
  Config zero;
  for (Level i = 0; i < nlevels; ++i) {
    stacks_[i] = new PdtStack<StackId, Label>(vectors[i]);
    neg_one[i] = -1;
    zero[i] = 0;
  }
  config_to_stack_id_map_[neg_one] = -1;
  config_to_stack_id_map_[zero] = 0;
  stack_id_to_config_map_[-1] = neg_one;
  stack_id_to_config_map_[0] = zero;
}

template <typename K, typename L, L nlevels, MPdtType restrict>
MPdtStack<K, L, nlevels, restrict>::MPdtStack(
    const MPdtStack<K, L, nlevels, restrict> &mstack)
    : error_(mstack.error_),
      min_paren_(mstack.min_paren_),
      max_paren_(mstack.max_paren_),
      parens_(mstack.parens_),
      next_stack_id_(mstack.next_stack_id_) {
  typedef K StackId;
  typedef L Label;
  typedef L Level;  // Not really needed, but I find it confusing otherwise.
  typedef StackConfig<StackId, nlevels> Config;
  for (const auto &kv : mstack.paren_levels_) {
    paren_levels_[kv.first] = kv.second;
  }
  for (int i = 0; i < mstack.parens_.size(); ++i) {
    parens_.push_back(mstack.parens_[i]);
  }
  for (const auto &kv : mstack.paren_map_) {
    paren_map_[kv.first] = kv.second;
  }
  for (const auto &kv : mstack.paren_id_map_) {
    paren_id_map_[kv.first] = kv.second;
  }
  for (typename ConfigToStackId::const_iterator ix =
           mstack.config_to_stack_id_map_.begin();
       ix != mstack.config_to_stack_id_map_.end(); ++ix) {
    config_to_stack_id_map_[ix->first] = ix->second;
  }
  for (const auto &kv : mstack.stack_id_to_config_map_) {
    Config config(kv.second);
    stack_id_to_config_map_[kv.first] = config;
  }
  for (int i = 0; i < nlevels; ++i) {
    stacks_[i] = mstack.stacks_[i];
  }
}

template <typename K, typename L, L nlevels, MPdtType restrict>
K MPdtStack<K, L, nlevels, restrict>::Find(K stack_id, L label) {
  typedef K StackId;
  typedef L Label;
  typedef L Level;  // Not really needed, but I find it confusing otherwise.
  typedef StackConfig<StackId, nlevels> Config;
  if (min_paren_ == kNoLabel || label < min_paren_ || label > max_paren_)
    return stack_id;  // Non-paren.
  typename std::unordered_map<Label, size_t>::const_iterator pit =
      paren_map_.find(label);
  if (pit == paren_map_.end())  // Non-paren.
    return stack_id;
  ssize_t paren_id = pit->second;
  // Get the configuration associated with this stack_id
  const Config config = InternalStackIds(stack_id);
  // Get the level
  auto pli = paren_levels_.find(label);
  Level lev = pli->second;
  // If label is an open paren we push
  // 1) if the restrict type is not MPDT_WRITE_RESTRICT
  // 2) the restrict type *is* MPDT_WRITE_RESTRICT, and all the stacks
  //    above lev are empty
  if (label == parens_[paren_id].first) {  // Open paren.
    if (restrict == MPDT_WRITE_RESTRICT) {
      for (Level i = 0; i < lev; ++i)
        // Non-empty stack blocks
        if (!Empty(config, i)) return -1;
    }
    // If label is an close paren we pop
    // 1) if the restrict type is not MPDT_READ_RESTRICT
    // 2) the restrict type *is* MPDT_READ_RESTRICT, and all the stacks
    //    above lev are empty
  } else if (restrict == MPDT_READ_RESTRICT) {
    for (Level i = 0; i < lev; ++i)
      // Non-empty stack blocks
      if (!Empty(config, i)) return -1;
  }
  StackId nid = stacks_[lev]->Find(config[lev], label);
  // If the new id is -1, that means that there is no valid transition at the
  // level we want.
  if (nid == -1) {
    return -1;
  } else {
    Config nconfig(config);
    nconfig[lev] = nid;
    return ExternalStackId(nconfig);
  }
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_MPDT_H__
