// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// A generic (string,type) list file format.
//
// This is a stripped-down version of STTable that does not support the Find()
// operation but that does support reading/writting from standard in/out.

#ifndef FST_EXTENSIONS_FAR_STLIST_H_
#define FST_EXTENSIONS_FAR_STLIST_H_

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <fstream>
#include <fst/util.h>

namespace fst {

static const int32 kSTListMagicNumber = 5656924;
static const int32 kSTListFileVersion = 1;

// String-type list writing class for object of type 'T' using functor 'W'
// to write an object of type 'T' from a stream. 'W' must conform to the
// following interface:
//
//   struct Writer {
//     void operator()(std::ostream &, const T &) const;
//   };
//
template <class T, class W>
class STListWriter {
 public:
  typedef T EntryType;
  typedef W EntryWriter;

  explicit STListWriter(const string& filename)
      : stream_(filename.empty() ? &std::cout : new std::ofstream(
                                                    filename.c_str(),
                                                    std::ios_base::out |
                                                        std::ios_base::binary)),
        error_(false) {
    WriteType(*stream_, kSTListMagicNumber);
    WriteType(*stream_, kSTListFileVersion);
    if (!stream_) {
      FSTERROR() << "STListWriter::STListWriter: Error writing to file: "
                 << filename;
      error_ = true;
    }
  }

  static STListWriter<T, W> *Create(const string &filename) {
    return new STListWriter<T, W>(filename);
  }

  void Add(const string &key, const T &t) {
    if (key == "") {
      FSTERROR() << "STListWriter::Add: Key empty: " << key;
      error_ = true;
    } else if (key < last_key_) {
      FSTERROR() << "STListWriter::Add: Key out of order: " << key;
      error_ = true;
    }
    if (error_) return;
    last_key_ = key;
    WriteType(*stream_, key);
    entry_writer_(*stream_, t);
  }

  bool Error() const { return error_; }

  ~STListWriter() {
    WriteType(*stream_, string());
    if (stream_ != &std::cout) delete stream_;
  }

 private:
  EntryWriter entry_writer_;  // Write functor for 'EntryType'
  std::ostream *stream_;      // Output stream
  string last_key_;           // Last key
  bool error_;

  STListWriter(const STListWriter &) = delete;
  STListWriter &operator=(const STListWriter &) = delete;
};

// String-type list reading class for object of type 'T' using functor 'R'
// to read an object of type 'T' form a stream. 'R' must conform to the
// following interface:
//
//   struct Reader {
//     T *operator()(std::istream &) const;
//   };
//
template <class T, class R>
class STListReader {
 public:
  typedef T EntryType;
  typedef R EntryReader;

  explicit STListReader(const std::vector<string> &filenames)
      : sources_(filenames), error_(false) {
    streams_.resize(filenames.size(), 0);
    bool has_stdin = false;
    for (size_t i = 0; i < filenames.size(); ++i) {
      if (filenames[i].empty()) {
        if (!has_stdin) {
          streams_[i] = &std::cin;
          sources_[i] = "stdin";
          has_stdin = true;
        } else {
          FSTERROR() << "STListReader::STListReader: stdin should only "
                     << "appear once in the input file list.";
          error_ = true;
          return;
        }
      } else {
        streams_[i] = new std::ifstream(
            filenames[i].c_str(), std::ios_base::in | std::ios_base::binary);
      }
      int32 magic_number = 0, file_version = 0;
      ReadType(*streams_[i], &magic_number);
      ReadType(*streams_[i], &file_version);
      if (magic_number != kSTListMagicNumber) {
        FSTERROR() << "STListReader::STListReader: Wrong file type: "
                   << filenames[i];
        error_ = true;
        return;
      }
      if (file_version != kSTListFileVersion) {
        FSTERROR() << "STListReader::STListReader: Wrong file version: "
                   << filenames[i];
        error_ = true;
        return;
      }
      string key;
      ReadType(*streams_[i], &key);
      if (!key.empty()) heap_.push(std::make_pair(key, i));
      if (!*streams_[i]) {
        FSTERROR() << "STListReader: Error reading file: " << sources_[i];
        error_ = true;
        return;
      }
    }
    if (heap_.empty()) return;
    size_t current = heap_.top().second;
    entry_.reset(entry_reader_(*streams_[current]));
    if (!entry_ || !*streams_[current]) {
      FSTERROR() << "STListReader: Error reading entry for key: "
                 << heap_.top().first << ", file: " << sources_[current];
      error_ = true;
    }
  }

  ~STListReader() {
    for (size_t i = 0; i < streams_.size(); ++i) {
      if (streams_[i] != &std::cin) delete streams_[i];
    }
  }

  static STListReader<T, R> *Open(const string &filename) {
    std::vector<string> filenames;
    filenames.push_back(filename);
    return new STListReader<T, R>(filenames);
  }

  static STListReader<T, R> *Open(const std::vector<string> &filenames) {
    return new STListReader<T, R>(filenames);
  }

  void Reset() {
    FSTERROR()
        << "STListReader::Reset: stlist does not support reset operation";
    error_ = true;
  }

  bool Find(const string &key) {
    FSTERROR() << "STListReader::Find: stlist does not support find operation";
    error_ = true;
    return false;
  }

  bool Done() const { return error_ || heap_.empty(); }

  void Next() {
    if (error_) return;
    size_t current = heap_.top().second;
    string key;
    heap_.pop();
    ReadType(*(streams_[current]), &key);
    if (!*streams_[current]) {
      FSTERROR() << "STListReader: Error reading file: " << sources_[current];
      error_ = true;
      return;
    }
    if (!key.empty()) heap_.push(std::make_pair(key, current));

    if (!heap_.empty()) {
      current = heap_.top().second;
      entry_.reset(entry_reader_(*streams_[current]));
      if (!entry_ || !*streams_[current]) {
        FSTERROR() << "STListReader: Error reading entry for key: "
                   << heap_.top().first << ", file: " << sources_[current];
        error_ = true;
      }
    }
  }

  const string &GetKey() const { return heap_.top().first; }

  const EntryType *GetEntry() const { return entry_.get(); }

  bool Error() const { return error_; }

 private:
  EntryReader entry_reader_;        // Read functor for 'EntryType'
  std::vector<std::istream *> streams_;  // Input streams
  std::vector<string> sources_;          // and corresponding file names
  priority_queue<
      std::pair<string, size_t>, std::vector<std::pair<string, size_t>>,
      std::greater<std::pair<string, size_t>>> heap_;  // (Key, stream id) heap
  mutable std::unique_ptr<EntryType> entry_;  // the currently read entry
  bool error_;

  STListReader(const STListReader &) = delete;
  STListReader &operator=(const STListReader &) = delete;
};

// String-type list header reading function template on the entry header
// type 'H' having a member function:
//   Read(std::istream &strm, const string &filename);
// Checks that 'filename' is an STList and call the H::Read() on the last
// entry in the STList.
// Does not support reading from stdin.
template <class H>
bool ReadSTListHeader(const string &filename, H *header) {
  if (filename.empty()) {
    LOG(ERROR) << "ReadSTListHeader: Reading header not supported on stdin";
    return false;
  }
  std::ifstream strm(filename.c_str(),
                          std::ios_base::in | std::ios_base::binary);
  int32 magic_number = 0;
  int32 file_version = 0;
  ReadType(strm, &magic_number);
  ReadType(strm, &file_version);
  if (magic_number != kSTListMagicNumber) {
    LOG(ERROR) << "ReadSTListHeader: Wrong file type: " << filename;
    return false;
  }
  if (file_version != kSTListFileVersion) {
    LOG(ERROR) << "ReadSTListHeader: Wrong file version: " << filename;
    return false;
  }
  string key;
  ReadType(strm, &key);
  header->Read(strm, filename + ":" + key);
  if (!strm) {
    LOG(ERROR) << "ReadSTListHeader: Error reading file: " << filename;
    return false;
  }
  return true;
}

bool IsSTList(const string &filename);

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_STLIST_H_
