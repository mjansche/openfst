// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_EXTENSIONS_FAR_INFO_H_
#define FST_EXTENSIONS_FAR_INFO_H_

#include <iomanip>
#include <set>
#include <string>
#include <vector>

#include <fst/extensions/far/far.h>
#include <fst/extensions/far/main.h>  // For FarTypeToString

namespace fst {

template <class Arc>
void CountStatesAndArcs(const Fst<Arc> &fst, size_t *nstate, size_t *narc,
                        size_t *nfinal) {
  StateIterator<Fst<Arc>> siter(fst);
  for (; !siter.Done(); siter.Next(), ++(*nstate)) {
    ArcIterator<Fst<Arc>> aiter(fst, siter.Value());
    for (; !aiter.Done(); aiter.Next(), ++(*narc)) {
    }
    if (fst.Final(siter.Value()) != Arc::Weight::Zero()) ++(*nfinal);
  }
}

struct KeyInfo {
  string key;
  string type;
  size_t nstate;
  size_t narc;
  size_t nfinal;

  KeyInfo(string k, string t, int64 ns = 0, int64 na = 0, int nf = 0)
      : key(k), type(t), nstate(ns), narc(na), nfinal(nf) {}
};

template <class Arc>
void FarInfo(const std::vector<string> &filenames, const string &begin_key,
             const string &end_key, const bool list_fsts) {
  FarReader<Arc> *far_reader = FarReader<Arc>::Open(filenames);
  if (!far_reader) return;

  if (!begin_key.empty()) far_reader->Find(begin_key);

  std::vector<KeyInfo> *infos =
      list_fsts ? new std::vector<KeyInfo>() : nullptr;
  size_t nfst = 0, nstate = 0, narc = 0, nfinal = 0;
  std::set<string> fst_types;
  for (; !far_reader->Done(); far_reader->Next()) {
    string key = far_reader->GetKey();
    if (!end_key.empty() && end_key < key) break;
    ++nfst;
    const Fst<Arc> &fst = far_reader->GetFst();
    fst_types.insert(fst.Type());
    if (infos) {
      KeyInfo info(key, fst.Type());
      CountStatesAndArcs(fst, &info.nstate, &info.narc, &info.nfinal);
      nstate += info.nstate;
      narc += info.narc;
      nfinal += info.nfinal;
      infos->push_back(info);
    } else {
      CountStatesAndArcs(fst, &nstate, &narc, &nfinal);
    }
  }

  if (!infos) {
    std::cout << std::left << std::setw(50) << "far type"
              << FarTypeToString(far_reader->Type()) << std::endl;
    std::cout << std::left << std::setw(50) << "arc type" << Arc::Type()
              << std::endl;
    std::cout << std::left << std::setw(50) << "fst type";
    for (set<string>::const_iterator iter = fst_types.begin();
         iter != fst_types.end(); ++iter) {
      if (iter != fst_types.begin()) std::cout << ",";
      std::cout << *iter;
    }
    std::cout << std::endl;
    std::cout << std::left << std::setw(50) << "# of FSTs" << nfst << std::endl;
    std::cout << std::left << std::setw(50) << "total # of states" << nstate
              << std::endl;
    std::cout << std::left << std::setw(50) << "total # of arcs" << narc
              << std::endl;
    std::cout << std::left << std::setw(50) << "total # of final states"
              << nfinal << std::endl;

  } else {
    int wkey = 10, wtype = 10, wnstate = 14, wnarc = 12, wnfinal = 20;
    for (size_t i = 0; i < infos->size(); ++i) {
      const KeyInfo &info = (*infos)[i];
      if (info.key.size() + 2 > wkey) wkey = info.key.size() + 2;
      if (info.type.size() + 2 > wtype) wtype = info.type.size() + 2;
      if (ceil(log10(info.nstate)) + 2 > wnstate)
        wnstate = ceil(log10(info.nstate)) + 2;
      if (ceil(log10(info.narc)) + 2 > wnarc)
        wnarc = ceil(log10(info.narc)) + 2;
      if (ceil(log10(info.nfinal)) + 2 > wnfinal)
        wnfinal = ceil(log10(info.nfinal)) + 2;
    }

    std::cout << std::left << std::setw(wkey) << "key" << std::setw(wtype)
              << "type" << std::right << std::setw(wnstate) << "# of states"
              << std::setw(wnarc) << "# of arcs" << std::setw(wnfinal)
              << "# of final states" << std::endl;

    for (size_t i = 0; i < infos->size(); ++i) {
      const KeyInfo &info = (*infos)[i];
      std::cout << std::left << std::setw(wkey) << info.key << std::setw(wtype)
                << info.type << std::right << std::setw(wnstate) << info.nstate
                << std::setw(wnarc) << info.narc << std::setw(wnfinal)
                << info.nfinal << std::endl;
    }
  }
}

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_INFO_H_
