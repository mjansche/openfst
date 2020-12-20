// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Definition of ReadLabelTriples based on ReadLabelPairs, like that in
// nlp/fst/lib/util.h for pairs, and similarly for WriteLabelTriples.

#ifndef FST_EXTENSIONS_MPDT_READ_WRITE_UTILS_H__
#define FST_EXTENSIONS_MPDT_READ_WRITE_UTILS_H__

#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <fstream>
#include <fst/test-properties.h>

namespace fst {

// Returns true on success
template <typename Label>
bool ReadLabelTriples(const string& filename,
                      std::vector<std::pair<Label, Label>>* pairs,
                      std::vector<Label>* assignments,
                      bool allow_negative = false) {
  std::ifstream strm(filename.c_str());

  if (!strm) {
    LOG(ERROR) << "ReadIntTriples: Can't open file: " << filename;
    return false;
  }

  const int kLineLen = 8096;
  char line[kLineLen];
  size_t nline = 0;

  pairs->clear();
  while (strm.getline(line, kLineLen)) {
    ++nline;
    std::vector<char*> col;
    SplitToVector(line, "\n\t ", &col, true);
    // empty line or comment?
    if (col.size() == 0 || col[0][0] == '\0' || col[0][0] == '#') continue;
    if (col.size() != 3) {
      LOG(ERROR) << "ReadLabelTriples: Bad number of columns, "
                 << "file = " << filename << ", line = " << nline;
      return false;
    }

    bool err;
    Label i1 = StrToInt64(col[0], filename, nline, allow_negative, &err);
    if (err) return false;
    Label i2 = StrToInt64(col[1], filename, nline, allow_negative, &err);
    if (err) return false;
    Label i3 = StrToInt64(col[2], filename, nline, allow_negative, &err);
    if (err) return false;
    pairs->push_back(std::make_pair(i1, i2));
    assignments->push_back(i3);
  }
  return true;
}

// Returns true on success
template <typename Label>
bool WriteLabelTriples(const string& filename,
                       const std::vector<std::pair<Label, Label>>& pairs,
                       const std::vector<Label>& assignments) {
  std::ostream* strm = &std::cout;
  if (pairs.size() != assignments.size()) {
    LOG(ERROR) << "WriteLabelTriples: Pairs and assignments of different sizes";
    return false;
  }

  if (!filename.empty()) {
    strm = new std::ofstream(filename.c_str());
    if (!*strm) {
      LOG(ERROR) << "WriteLabelTriples: Can't open file: " << filename;
      return false;
    }
  }

  for (ssize_t n = 0; n < pairs.size(); ++n)
    *strm << pairs[n].first << "\t" << pairs[n].second << "\t" << assignments[n]
          << "\n";

  if (!*strm) {
    LOG(ERROR) << "WriteLabelTriples: Write failed: "
               << (filename.empty() ? "standard output" : filename);
    return false;
  }
  if (strm != &std::cout) delete strm;
  return true;
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_READ_WRITE_UTILS_H__
