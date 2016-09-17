// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// A generic string-to-type table file format.
//
// This is not meant as a generalization of SSTable. This is more of a simple
// replacement for SSTable in order to provide an open-source implementation
// of the FAR format for the external version of the FST library.

#ifndef FST_EXTENSIONS_FAR_STTABLE_H_
#define FST_EXTENSIONS_FAR_STTABLE_H_

#include <algorithm>
#include <istream>
#include <memory>

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
//     void operator()(std::ostream &, const T &) const;
//   };
//
template <class T, class W>
class STTableWriter {
 public:
  typedef T EntryType;
  typedef W EntryWriter;

  explicit STTableWriter(const string &filename)
      : stream_(filename.c_str(), std::ios_base::out | std::ios_base::binary),
        error_(false) {
    WriteType(stream_, kSTTableMagicNumber);
    WriteType(stream_, kSTTableFileVersion);
    if (stream_.fail()) {
      FSTERROR() << "STTableWriter::STTableWriter: Error writing to file: "
                 << filename;
      error_ = true;
    }
  }

  static STTableWriter<T, W> *Create(const string &filename) {
    if (filename.empty()) {
      LOG(ERROR) << "STTableWriter: Writing to standard out unsupported.";
      return nullptr;
    }
    return new STTableWriter<T, W>(filename);
  }

  void Add(const string &key, const T &t) {
    if (key == "") {
      FSTERROR() << "STTableWriter::Add: Key empty: " << key;
      error_ = true;
    } else if (key < last_key_) {
      FSTERROR() << "STTableWriter::Add: Key out of order: " << key;
      error_ = true;
    }
    if (error_) return;
    last_key_ = key;
    positions_.push_back(stream_.tellp());
    WriteType(stream_, key);
    entry_writer_(stream_, t);
  }

  bool Error() const { return error_; }

  ~STTableWriter() {
    WriteType(stream_, positions_);
    WriteType(stream_, static_cast<int64>(positions_.size()));
  }

 private:
  EntryWriter entry_writer_;      // Write functor for 'EntryType'
  std::ofstream stream_;    // Output stream
  std::vector<int64> positions_;  // Position in file of each key-entry pair
  string last_key_;               // Last key
  bool error_;

  STTableWriter(const STTableWriter &) = delete;
  STTableWriter &operator=(const STTableWriter &) = delete;
};

// String-to-type table reading class for object of type 'T' using functor 'R'
// to read an object of type 'T' form a stream. 'R' must conform to the
// following interface:
//
//   struct Reader {
//     T *operator()(std::istream &) const;
//   };
//
template <class T, class R>
class STTableReader {
 public:
  typedef T EntryType;
  typedef R EntryReader;

  explicit STTableReader(const std::vector<string> &filenames)
      : sources_(filenames), error_(false) {
    compare_.reset(new Compare(&keys_));
    keys_.resize(filenames.size());
    streams_.resize(filenames.size(), 0);
    positions_.resize(filenames.size());
    for (size_t i = 0; i < filenames.size(); ++i) {
      streams_[i] = new std::ifstream(
          filenames[i].c_str(), std::ios_base::in | std::ios_base::binary);
      int32 magic_number = 0, file_version = 0;
      ReadType(*streams_[i], &magic_number);
      ReadType(*streams_[i], &file_version);
      if (magic_number != kSTTableMagicNumber) {
        FSTERROR() << "STTableReader::STTableReader: Wrong file type: "
                   << filenames[i];
        error_ = true;
        return;
      }
      if (file_version != kSTTableFileVersion) {
        FSTERROR() << "STTableReader::STTableReader: Wrong file version: "
                   << filenames[i];
        error_ = true;
        return;
      }
      int64 num_entries;
      streams_[i]->seekg(-static_cast<int>(sizeof(int64)), ios_base::end);
      ReadType(*streams_[i], &num_entries);
      if (num_entries > 0) {
        streams_[i]->seekg(-static_cast<int>(sizeof(int64)) * (num_entries + 1),
                           ios_base::end);
        positions_[i].resize(num_entries);
        for (size_t j = 0; (j < num_entries) && (!streams_[i]->fail()); ++j)
          ReadType(*streams_[i], &(positions_[i][j]));
        streams_[i]->seekg(positions_[i][0]);
        if (streams_[i]->fail()) {
          FSTERROR() << "STTableReader::STTableReader: Error reading file: "
                     << filenames[i];
          error_ = true;
          return;
        }
      }
    }
    MakeHeap();
  }

  ~STTableReader() {
    for (size_t i = 0; i < streams_.size(); ++i) delete streams_[i];
  }

  static STTableReader<T, R> *Open(const string &filename) {
    if (filename.empty()) {
      LOG(ERROR) << "STTableReader: operation not supported on stdin";
      return nullptr;
    }
    std::vector<string> filenames;
    filenames.push_back(filename);
    return new STTableReader<T, R>(filenames);
  }

  static STTableReader<T, R> *Open(const std::vector<string> &filenames) {
    return new STTableReader<T, R>(filenames);
  }

  void Reset() {
    if (error_) return;
    for (size_t i = 0; i < streams_.size(); ++i)
      streams_[i]->seekg(positions_[i].front());
    MakeHeap();
  }

  bool Find(const string &key) {
    if (error_) return false;
    for (size_t i = 0; i < streams_.size(); ++i) LowerBound(i, key);
    MakeHeap();
    if (heap_.empty()) return false;
    return keys_[current_] == key;
  }

  bool Done() const { return error_ || heap_.empty(); }

  void Next() {
    if (error_) return;
    if (streams_[current_]->tellg() <= positions_[current_].back()) {
      ReadType(*(streams_[current_]), &(keys_[current_]));
      if (streams_[current_]->fail()) {
        FSTERROR() << "STTableReader: Error reading file: "
                   << sources_[current_];
        error_ = true;
        return;
      }
      std::push_heap(heap_.begin(), heap_.end(), *compare_);
    } else {
      heap_.pop_back();
    }
    if (!heap_.empty()) PopHeap();
  }

  const string &GetKey() const { return keys_[current_]; }

  const EntryType *GetEntry() const { return entry_.get(); }

  bool Error() const { return error_; }

 private:
  // Comparison functor used to compare stream IDs in the heap
  struct Compare {
    explicit Compare(const std::vector<string> *k) : keys(k) {}

    bool operator()(size_t i, size_t j) const {
      return (*keys)[i] > (*keys)[j];
    };

   private:
    const std::vector<string> *keys;
  };

  // Position the stream with ID 'id' at the position corresponding
  // to the lower bound for key 'find_key'
  void LowerBound(size_t id, const string &find_key) {
    std::istream *strm = streams_[id];
    const std::vector<int64> &positions = positions_[id];
    if (positions.empty()) return;
    size_t low = 0;
    size_t high = positions.size() - 1;

    while (low < high) {
      size_t mid = (low + high) / 2;
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
      if (positions_[i].empty()) continue;
      ReadType(*streams_[i], &(keys_[i]));
      if (streams_[i]->fail()) {
        FSTERROR() << "STTableReader: Error reading file: " << sources_[i];
        error_ = true;
        return;
      }
      heap_.push_back(i);
    }
    if (heap_.empty()) return;
    std::make_heap(heap_.begin(), heap_.end(), *compare_);
    PopHeap();
  }

  // Position the stream with the lowest key at the top
  // of the heap, set 'current_' to the ID of that stream
  // and read the current entry from that stream
  void PopHeap() {
    std::pop_heap(heap_.begin(), heap_.end(), *compare_);
    current_ = heap_.back();
    entry_.reset(entry_reader_(*streams_[current_]));
    if (!entry_) error_ = true;
    if (streams_[current_]->fail()) {
      FSTERROR() << "STTableReader: Error reading entry for key: "
                 << keys_[current_] << ", file: " << sources_[current_];
      error_ = true;
    }
  }

  EntryReader entry_reader_;                   // Read functor for 'EntryType'
  std::vector<std::istream *> streams_;        // Input streams
  std::vector<string> sources_;                // and corresponding file names
  std::vector<std::vector<int64>> positions_;  // Index of positions
  std::vector<string> keys_;  // Lowest unread key for each stream
  std::vector<int64> heap_;   // Heap containing ID of streams with unread keys
  int64 current_;             // Id of current stream to be read
  std::unique_ptr<Compare> compare_;          // Functor comparing stream IDs
  mutable std::unique_ptr<EntryType> entry_;  // the currently read entry
  bool error_;
};

// String-to-type table header reading function template on the entry header
// type 'H' having a member function:
//   Read(std::istream &strm, const string &filename);
// Checks that 'filename' is an STTable and call the H::Read() on the last
// entry in the STTable.
template <class H>
bool ReadSTTableHeader(const string &filename, H *header) {
  std::ifstream strm(filename);
  int32 magic_number = 0;
  int32 file_version = 0;
  ReadType(strm, &magic_number);
  ReadType(strm, &file_version);
  if (magic_number != kSTTableMagicNumber) {
    LOG(ERROR) << "ReadSTTableHeader: Wrong file type: " << filename;
    return false;
  }
  if (file_version != kSTTableFileVersion) {
    LOG(ERROR) << "ReadSTTableHeader: Wrong file version: " << filename;
    return false;
  }
  int64 i = -1;
  strm.seekg(-static_cast<int>(sizeof(int64)), ios_base::end);
  ReadType(strm, &i);  // Read number of entries
  if (strm.fail()) {
    LOG(ERROR) << "ReadSTTableHeader: Error reading file: " << filename;
    return false;
  }
  if (i == 0) return true;  // No entry header to read
  strm.seekg(-2 * static_cast<int>(sizeof(int64)), ios_base::end);
  ReadType(strm, &i);  // Read position for last entry in file
  strm.seekg(i);
  string key;
  ReadType(strm, &key);
  header->Read(strm, filename + ":" + key);
  if (strm.fail()) {
    LOG(ERROR) << "ReadSTTableHeader: Error reading file: " << filename;
    return false;
  }
  return true;
}

bool IsSTTable(const string &filename);

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_STTABLE_H_
