
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2010 Google, Inc.
// Author: allauzen@google.com (Cyril Allauzen)
//
// \file
// A generic (string,type) list file format.
//
// This is a stripped-down version of STTable that does
// not support the Find() operation but that does support
// reading/writting from standard in/out.

#ifndef FST_EXTENSIONS_FAR_STLIST_H_
#define FST_EXTENSIONS_FAR_STLIST_H_

#include <iostream>
#include <fstream>
#include <fst/util.h>

#include <algorithm>
#include <queue>
#include <string>
#include <utility>
using std::pair; using std::make_pair;
#include <vector>
using std::vector;

namespace fst {

static const int32 kSTListMagicNumber = 5656924;
static const int32 kSTListFileVersion = 1;

// String-type list writing class for object of type 'T' using functor 'W'
// to write an object of type 'T' from a stream. 'W' must conform to the
// following interface:
//
//   struct Writer {
//     void operator()(ostream &, const T &) const;
//   };
//
template <class T, class W>
class STListWriter {
 public:
  typedef T EntryType;
  typedef W EntryWriter;

  explicit STListWriter(const string filename)
      : stream_(
          filename.empty() ? &std::cout :
          new ofstream(filename.c_str(), ofstream::out | ofstream::binary)) {
    WriteType(*stream_, kSTListMagicNumber);
    WriteType(*stream_, kSTListFileVersion);
    if (!stream_)
      LOG(FATAL) << "STListWriter::STListWriter: error writing to file: "
                 << filename;
  }

  static STListWriter<T, W> *Create(const string &filename) {
    return new STListWriter<T, W>(filename);
  }

  void Add(const string &key, const T &t) {
    if (key == "")
      LOG(FATAL) << "STListWriter::Add: key empty: " << key;
    else if (key < last_key_)
      LOG(FATAL) << "STListWriter::Add: key disorder: " << key;
    last_key_ = key;
    WriteType(*stream_, key);
    entry_writer_(*stream_, t);
  }

  ~STListWriter() {
    WriteType(*stream_, string());
    if (stream_ != &std::cout)
      delete stream_;
  }

 private:
  EntryWriter entry_writer_;  // Write functor for 'EntryType'
  ostream *stream_;           // Output stream
  string last_key_;           // Last key

  DISALLOW_COPY_AND_ASSIGN(STListWriter);
};


// String-type list reading class for object of type 'T' using functor 'R'
// to read an object of type 'T' form a stream. 'R' must conform to the
// following interface:
//
//   struct Reader {
//     T *operator()(istream &) const;
//   };
//
template <class T, class R>
class STListReader {
 public:
  typedef T EntryType;
  typedef R EntryReader;

  explicit STListReader(const vector<string> &filenames)
      : sources_(filenames), entry_(0) {
    streams_.resize(filenames.size());
    bool has_stdin = false;
    for (size_t i = 0; i < filenames.size(); ++i) {
      if (filenames[i].empty()) {
        if (!has_stdin) {
          streams_[i] = &std::cin;
          sources_[i] = "stdin";
          has_stdin = true;
        } else {
          LOG(FATAL) << "STListReader::STListReader: stdin should only "
                     << "appear once in the input file list.";
        }
      } else {
        streams_[i] = new ifstream(
            filenames[i].c_str(), ifstream::in | ifstream::binary);
      }
      int32 magic_number = 0, file_version = 0;
      ReadType(*streams_[i], &magic_number);
      ReadType(*streams_[i], &file_version);
      if (magic_number != kSTListMagicNumber)
        LOG(FATAL) << "STListReader::STTableReader: wrong file type: "
                   << filenames[i];
      if (file_version != kSTListFileVersion)
        LOG(FATAL) << "STListReader::STTableReader: wrong file version: "
                   << filenames[i];
      string key;
      ReadType(*streams_[i], &key);
      if (!key.empty())
        heap_.push(make_pair(key, i));
      if (!*streams_[i])
        LOG(FATAL) << "STTableReader: error reading file: " << sources_[i];
    }
    if (heap_.empty()) return;
    size_t current = heap_.top().second;
    entry_ = entry_reader_(*streams_[current]);
    if (!*streams_[current])
      LOG(FATAL) << "STTableReader: error reading entry for key: "
                 << heap_.top().first << ", file: " << sources_[current];
  }

  ~STListReader() {
    for (size_t i = 0; i < streams_.size(); ++i) {
      if (streams_[i] != &std::cin)
        delete streams_[i];
    }
    if (entry_)
      delete entry_;
  }

  static STListReader<T, R> *Open(const string &filename) {
    vector<string> filenames;
    filenames.push_back(filename);
    return new STListReader<T, R>(filenames);
  }

  static STListReader<T, R> *Open(const vector<string> &filenames) {
    return new STListReader<T, R>(filenames);
  }

  void Reset() {
    LOG(FATAL)
        << "STListReader::Reset: stlist does not support reset operation";
  }

  bool Find(const string &key) {
    LOG(FATAL)
        << "STListReader::Find: stlist does not support find operation";
    return false;
  }

  bool Done() const {
    return heap_.empty();
  }

  void Next() {
    size_t current = heap_.top().second;
    string key;
    heap_.pop();
    ReadType(*(streams_[current]), &key);
    if (!*streams_[current])
      LOG(FATAL) << "STTableReader: error reading file: "
                 << sources_[current];
    if (!key.empty())
      heap_.push(make_pair(key, current));

    if(!heap_.empty()) {
      current = heap_.top().second;
      if (entry_)
        delete entry_;
      entry_ = entry_reader_(*streams_[current]);
      if (!*streams_[current])
        LOG(FATAL) << "STTableReader: error reading entry for key: "
                   << heap_.top().first << ", file: " << sources_[current];

    }
  }

  const string &GetKey() const {
    return heap_.top().first;
  }

  const EntryType &GetEntry() const {
    return *entry_;
  }

 private:
  EntryReader entry_reader_;   // Read functor for 'EntryType'
  vector<istream*> streams_;   // Input streams
  vector<string> sources_;     // and corresponding file names
  priority_queue<
    pair<string, size_t>, vector<pair<string, size_t> >,
    greater<pair<string, size_t> > > heap_;  // (Key, stream id) heap
  mutable EntryType *entry_;   // Pointer to the currently read entry

  DISALLOW_COPY_AND_ASSIGN(STListReader);
};


// String-type list header reading function template on the entry header
// type 'H' having a member function:
//   Read(istream &strm, const string &filename);
// Checks that 'filename' is an STTable and call the H::Read() on the last
// entry in the STTable.
// Does not support reading from stdin.
template <class H>
bool ReadSTListHeader(const string &filename, H *header) {
  if (filename.empty()) {
    LOG(ERROR) << "ReadSTListHeader: reading header not supported on stdin";
    return false;
  }
  ifstream strm(filename.c_str(), ifstream::in | ifstream::binary);
  int32 magic_number = 0, file_version = 0;
  ReadType(strm, &magic_number);
  ReadType(strm, &file_version);
  if (magic_number != kSTListMagicNumber) {
    LOG(ERROR) << "ReadSTTableHeader: wrong file type: " << filename;
    return false;
  }
  if (file_version != kSTListFileVersion) {
    LOG(ERROR) << "ReadSTTableHeader: wrong file version: " << filename;
    return false;
  }
  string key;
  ReadType(strm, &key);
  header->Read(strm, filename + ":" + key);
  if (!strm) {
    LOG(ERROR) << "ReadSTTableHeader: error reading file: " << filename;
    return false;
  }
  return true;
}

bool IsSTList(const string &filename);

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_STLIST_H_
