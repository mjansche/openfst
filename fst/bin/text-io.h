// text-io.h
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: riley@google.com (Michael Riley)
//
// \file
// Utilities for reading and writing textual strings representing
// states, labels, and weights and files specifying label-label pairs
// and potentials (state-weight pairs).

#ifndef FST_TEXT_IO_MAIN_H__
#define FST_TEXT_IO_MAIN_H__

#include <cstring>
#include <sstream>
#include <vector>

namespace fst {

int64 StrToInt64(const string &s, const string &src, size_t nline,
                 bool allow_negative = false) {
  int64 n;
  const char *cs = s.c_str();
  char *p;
  n = strtoll(cs, &p, 10);
  if (p < cs + s.size() || !allow_negative && n < 0)
    LOG(FATAL) << "StrToInt64: Bad integer = " << s
               << "\", source = " << src << ", line = " << nline;
  return n;
}

template <typename Weight>
Weight StrToWeight(const string &s, const string &src, size_t nline) {
  Weight w;
  istringstream strm(s);
  strm >> w;
  if (strm.fail())
    LOG(FATAL) << "StrToWeight: Bad weight = \"" << s
               << "\", source = " << src << ", line = " << nline;
  return w;
}

template <typename Label>
void ReadLabelPairs(const string& filename,
                    vector<pair<Label, Label> >* pairs) {
  ifstream strm(filename.c_str());
  if (!strm)
    LOG(FATAL) << "ReadLabelPairs: Can't open file: " << filename;

  const int kLineLen = 8096;
  char line[kLineLen];
  size_t nline = 0;

  pairs->clear();
  while (strm.getline(line, kLineLen)) {
    ++nline;
    vector<char *> col;
    SplitToVector(line, "\n\t ", &col, true);
    if (col.size() == 0 || col[0][0] == '\0')  // empty line
      continue;
    if (col.size() != 2)
      LOG(FATAL) << "ReadLabelPairs: Bad number of columns, "
                 << "file = " << filename << ", line = " << nline;

    Label fromlabel = StrToInt64(col[0], filename, nline, false);
    Label tolabel   = StrToInt64(col[1], filename, nline, false);
    pairs->push_back(make_pair(fromlabel, tolabel));
  }
  if (!strm.eof())
    LOG(FATAL) << "ReadLabelPairs: Read failed: " << filename;
}

template <typename Weight>
void ReadPotentials(const string& filename,
                    vector<Weight>* potential) {
  ifstream strm(filename.c_str());
  if (!strm)
    LOG(FATAL) << "ReadPotentials: Can't open file: " << filename;

  const int kLineLen = 8096;
  char line[kLineLen];
  size_t nline = 0;

  potential->clear();
  while (strm.getline(line, kLineLen)) {
    ++nline;
    vector<char *> col;
    SplitToVector(line, "\n\t ", &col, true);
    if (col.size() == 0 || col[0][0] == '\0')  // empty line
      continue;
    if (col.size() != 2)
      LOG(FATAL) << "ReadPotentials: Bad number of columns, "
                 << "file = " << filename << ", line = " << nline;

    ssize_t s = StrToInt64(col[0], filename, nline, false);
    Weight weight = StrToWeight<Weight>(col[1], filename, nline);
    while (potential->size() <= s)
      potential->push_back(Weight::Zero());
    (*potential)[s] = weight;
  }
  if (!strm.eof())
    LOG(FATAL) << "ReadPotentials: Read failed: " << filename;
}

template <typename Weight>
void WritePotentials(const string& filename,
                     const vector<Weight>& potential) {
  ostream *strm = &std::cout;
  if (!filename.empty()) {
    strm = new ofstream(filename.c_str());
    if (!strm)
      LOG(FATAL) << "WritePotentials: Can't open file: " << filename;
  }
  strm->precision(9);
  for (ssize_t s = 0; s < potential.size(); ++s)
    *strm << s << "\t" << potential[s] << "\n";

  if (!strm)
    LOG(FATAL) << "WritePotentials: Write failed: "
               << (filename.empty() ? "standard output" : filename);
  if (strm != &std::cout)
    delete strm;
}

}  // namespace fst

#endif  // FST_TEXT_IO_MAIN_H__
