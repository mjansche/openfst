// sttable.h

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
// A generic string-to-type table file format
//
// This is not meant as a generalization of SSTable. This is more of
// a simple replacement for SSTable in order to provide an open-source
// implementation of the FAR format for the external version of the
// FST Library.

#ifndef FST_EXTENSIONS_FAR_STTABLE_H_
#define FST_EXTENSIONS_FAR_STTABLE_H_

#include <algorithm>
#include <iostream>
#include <fstream>
#include <fst/util.h>

namespace fst {

static const int32 kSTTableMagicNumber = 2125656924;
static const int32 kSTTableFileVersion = 1;

// String-to-type table writing class for object of type 'T' using functor 'W'
// to write an object of type 'T' from a stream. 'W' must conform to the
// following interface:
//
//   struct Writer {
//     void operator()(ostream &, const T &) const;
//   };
//
template <class T, class W>
class STTableWriter {
 public:
  typedef T EntryType;
  typedef W EntryWriter;

  STTableWriter(const string &filename)
      : stream_(filename.c_str(), ofstream::out | ofstream::binary) {
    WriteType(stream_, kSTTableMagicNumber);
    WriteType(stream_, kSTTableFileVersion);
    if (!stream_)
      LOG(FATAL) << "STTableWriter::STTableWriter: error writing to file: "
                 << filename;
  }

  static STTableWriter<T, W> *Create(const string &filename) {
    return new STTableWriter<T, W>(filename);
  }

  void Add(const string &key, const T &t) {
    if (key == "")
      LOG(FATAL) << "STTableWriter::Add: key empty: " << key;
    else if (key < last_key_)
      LOG(FATAL) << "STTableWriter::Add: key disorder: " << key;
    last_key_ = key;
    positions_.push_back(stream_.tellp());
    WriteType(stream_, key);
    entry_writer_(stream_, t);
  }

  ~STTableWriter() {
    WriteType(stream_, positions_);
    WriteType(stream_, static_cast<int64>(positions_.size()));
  }

 private:
  EntryWriter entry_writer_;  // Write functor for 'EntryType'
  ofstream stream_;           // Output stream
  vector<int64> positions_;   // Position in file of each key-entry pair
  string last_key_;           // Last key

  DISALLOW_COPY_AND_ASSIGN(STTableWriter);
};


// String-to-type table reading class for object of type 'T' using functor 'R'
// to read an object of type 'T' form a stream. 'R' must conform to the
// following interface:
//
//   struct Reader {
//     T *operator()(istream &) const;
//   };
//
template <class T, class R>
class STTableReader {
 public:
  typedef T EntryType;
  typedef R EntryReader;

  STTableReader(const vector<string> &filenames)
      : sources_(filenames), entry_(0) {
    compare_ = new Compare(&keys_);
    keys_.resize(filenames.size());
    streams_.resize(filenames.size());
    positions_.resize(filenames.size());
    for (size_t i = 0; i < filenames.size(); ++i) {
      streams_[i] = new ifstream(
          filenames[i].c_str(), ifstream::in | ifstream::binary);
      int32 magic_number = 0, file_version = 0;
      ReadType(*streams_[i], &magic_number);
      ReadType(*streams_[i], &file_version);
      if (magic_number != kSTTableMagicNumber)
        LOG(FATAL) << "STTableReader::STTableReader: wrong file type: "
                   << filenames[i];
      if (file_version != kSTTableFileVersion)
        LOG(FATAL) << "STTableReader::STTableReader: wrong file version: "
                   << filenames[i];
      int64 num_entries;
      streams_[i]->seekg(-static_cast<int>(sizeof(int64)), ios_base::end);
      ReadType(*streams_[i], &num_entries);
      streams_[i]->seekg(-static_cast<int>(sizeof(int64)) *
                         (num_entries + 1), ios_base::end);
      positions_[i].resize(num_entries);
      for (size_t j = 0; (j < num_entries) && (*streams_[i]); ++j)
        ReadType(*streams_[i], &(positions_[i][j]));
      streams_[i]->seekg(positions_[i][0]);
      if (!*streams_[i])
        LOG(FATAL) << "STTableReader::STTableReader: error reading file: "
                   << filenames[i];
    }
    MakeHeap();
  }

  ~STTableReader() {
    for (size_t i = 0; i < streams_.size(); ++i)
      delete streams_[i];
    delete compare_;
    if (entry_)
      delete entry_;
  }


  static STTableReader<T, R> *Open(const string &filename) {
    vector<string> filenames;
    filenames.push_back(filename);
    return new STTableReader<T, R>(filenames);
  }

  static STTableReader<T, R> *Open(const vector<string> &filenames) {
    return new STTableReader<T, R>(filenames);
  }

  void Reset() {
    for (size_t i = 0; i < streams_.size(); ++i)
      streams_[i]->seekg(positions_[i].front());
    MakeHeap();
  }

  bool Find(const string &key) {
    for (size_t i = 0; i < streams_.size(); ++i)
      LowerBound(i, key);
    MakeHeap();
    return keys_[current_] == key;
  }

  bool Done() const { return heap_.empty(); }

  void Next() {
    if (streams_[current_]->tellg() <= positions_[current_].back()) {
      ReadType(*(streams_[current_]), &(keys_[current_]));
      if (!*streams_[current_])
        LOG(FATAL) << "STTableReader: error reading file: "
                   << sources_[current_];
      push_heap(heap_.begin(), heap_.end(), *compare_);
    } else {
      heap_.pop_back();
    }
    if (!heap_.empty())
      PopHeap();
  }

  const string &GetKey() const {
    return keys_[current_];
  }

  const EntryType &GetEntry() const {
    return *entry_;
  }

 private:
  // Comparison functor used to compare stream IDs in the heap
  struct Compare {
    Compare(const vector<string> *keys) : keys_(keys) {}

    bool operator()(size_t i, size_t j) const {
      return (*keys_)[i] > (*keys_)[j];
    };

   private:
    const vector<string> *keys_;
  };

  // Position the stream with ID 'id' at the position corresponding
  // to the lower bound for key 'find_key'
  void LowerBound(size_t id, const string &find_key) {
    ifstream *strm = streams_[id];
    const vector<int64> &positions = positions_[id];
    size_t low = 0, high = positions.size() - 1;

    while (low < high) {
      size_t mid = (low + high)/2;
      strm->seekg(positions[mid]);
      string key;
      ReadType(*strm, &key);
      if (key > find_key) {
        high = mid;
      } else if (key < find_key) {
        low = mid + 1;
      } else {
        for (size_t i = mid; i > low; --i) {
          strm->seekg(positions[i - 1]);
          ReadType(*strm, &key);
          if (key != find_key) {
            strm->seekg(positions[i]);
            return;
          }
        }
        strm->seekg(positions[low]);
        return;
      }
    }
    strm->seekg(positions[low]);
  }

  // Add all streams to the heap
  void MakeHeap() {
    heap_.clear();
    for (size_t i = 0; i < streams_.size(); ++i) {
      ReadType(*streams_[i], &(keys_[i]));
      if (!*streams_[i])
        LOG(FATAL) << "STTableReader: error reading file: " << sources_[i];
      heap_.push_back(i);
    }
    make_heap(heap_.begin(), heap_.end(), *compare_);
    PopHeap();
  }

  // Position the stream with the lowest key at the top
  // of the heap, set 'current_' to the ID of that stream
  // and read the current entry from that stream
  void PopHeap() {
    pop_heap(heap_.begin(), heap_.end(), *compare_);
    current_ = heap_.back();
    if (entry_)
      delete entry_;
    entry_ = entry_reader_(*streams_[current_]);
    if (!*streams_[current_])
      LOG(FATAL) << "STTableReader: error reading entry for key: "
                 << keys_[current_] << ", file: " << sources_[current_];
  }


  EntryReader entry_reader_;   // Read functor for 'EntryType'
  vector<ifstream*> streams_;  // Input streams
  vector<string> sources_;     // and corresponding file names
  vector<vector<int64> > positions_;  // Index of positions for each stream
  vector<string> keys_;  // Lowest unread key for each stream
  vector<int64> heap_;   // Heap containing ID of streams with unread keys
  int64 current_;        // Id of current stream to be read
  Compare *compare_;     // Functor comparing stream IDs for the heap
  mutable EntryType *entry_;  // Pointer to the currently read entry

  DISALLOW_COPY_AND_ASSIGN(STTableReader);
};


// String-to-type table header reading function template on the entry header
// type 'H' having a member function:
//   Read(istream &strm, const string &filename);
// Checks that 'filename' is an STTable and call the H::Read() on the last
// entry in the STTable.
template <class H>
bool ReadSTTableHeader(const string &filename, H *header) {
  ifstream strm(filename.c_str(), ifstream::in | ifstream::binary);
  int32 magic_number = 0, file_version = 0;
  ReadType(strm, &magic_number);
  ReadType(strm, &file_version);
  if (magic_number != kSTTableMagicNumber) {
    LOG(ERROR) << "ReadSTTableHeader: wrong file type: " << filename;
    return false;
  }
  if (file_version != kSTTableFileVersion) {
    LOG(ERROR) << "ReadSTTableHeader: wrong file version: " << filename;
    return false;
  }
  int64 i = -1;
  strm.seekg(-static_cast<int>(sizeof(int64)), ios_base::end);
  ReadType(strm, &i);  // Read number of entries
  if (!strm) {
    LOG(ERROR) << "ReadSTTableHeader: error reading file: " << filename;
    return false;
  }
  if (i == 0) return true;  // No entry header to read
  strm.seekg(-2 * static_cast<int>(sizeof(int64)), ios_base::end);
  ReadType(strm, &i);  // Read position for last entry in file
  strm.seekg(i);
  string key;
  ReadType(strm, &key);
  header->Read(strm, filename + ":" + key);
  if (!strm) {
    LOG(ERROR) << "ReadSTTableHeader: error reading file: " << filename;
    return false;
  }
  return true;
}

bool IsSTTable(const string &filename);

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_STTABLE_H_
