// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_LIB_MAPPED_FILE_H_
#define FST_LIB_MAPPED_FILE_H_

#include <cstddef>
#include <istream>
#include <string>

#include <fst/compat.h>

namespace fst {

// A memory region is a simple abstraction for allocated memory or data from
// mmap'ed files.  If mmap equals NULL, then data represents an owned region of
// size bytes.  Otherwise, mmap and size refer to the mapping and data is a
// casted pointer to a region contained within [mmap, mmap + size).
// If size is 0, then mmap refers and data refer to a block of memory managed
// externally by some other allocator.
// offset is used when allocating memory to providing padding for alignment.
struct MemoryRegion {
  void* data;
  void* mmap;
  size_t size;
  int offset;
};

class MappedFile {
 public:
  virtual ~MappedFile();

  void* mutable_data() const { return reinterpret_cast<void*>(region_.data); }

  const void* data() const { return reinterpret_cast<void*>(region_.data); }

  // Returns a MappedFile object that contains the contents of the input
  // stream s starting from the current file position with size bytes.
  // the memorymap bool is advisory, and Map will default to allocating and
  // reading.  source needs to contain the filename that was used to open
  // the istream.
  static MappedFile* Map(std::istream* s, bool memorymap, const string& source,
                         size_t size);

  // Creates a MappedFile object with a new[]'ed block of memory of size.
  // Align can be used to specify a desired block alignment.
  // RECOMMENDED FOR INTERNAL USE ONLY, may change in future releases.
  static MappedFile* Allocate(size_t size, int align = kArchAlignment);

  // Creates a MappedFile object pointing to a borrowed reference to data.
  // This block of memory is not owned by the MappedFile object and will not
  // be freed.
  // RECOMMENDED FOR INTERNAL USE ONLY, may change in future releases.
  static MappedFile* Borrow(void* data);

  static const int kArchAlignment;

 private:
  explicit MappedFile(const MemoryRegion& region);

  MemoryRegion region_;
  MappedFile(const MappedFile&) = delete;
  MappedFile& operator=(const MappedFile&) = delete;
};
}  // namespace fst

#endif  // FST_LIB_MAPPED_FILE_H_
