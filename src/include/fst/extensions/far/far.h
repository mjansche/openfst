// far.h

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
// Author: riley@google.com (Michael Riley)
//
// \file
// Finite-State Transducer (FST) archive classes.
//

#ifndef FST_EXTENSIONS_FAR_FAR_H__
#define FST_EXTENSIONS_FAR_FAR_H__

#include <fst/extensions/far/sttable.h>
#include <fst/fst.h>

namespace fst {

enum FarEntryType { FET_LINE, FET_FILE };
enum FarTokenType { FTT_SYMBOL, FTT_BYTE, FTT_UTF8 };

// FST archive header class
class FarHeader {
 public:
  const string &FarType() const { return fartype_; }
  const string &ArcType() const { return arctype_; }

  bool Read(const string &filename) {
    FstHeader fsthdr;
    if (IsSTTable(filename)) {  // Check if STTable
      ReadSTTableHeader(filename, &fsthdr);
      fartype_ = "sttable";
      arctype_ = fsthdr.ArcType().empty() ? "unknown" : fsthdr.ArcType();
      return true;
    }
    return false;
  }

 private:
  string fartype_;
  string arctype_;
};

enum FarType { FAR_DEFAULT = 0, FAR_STTABLE = 1, FAR_SSTABLE = 2 };

// This class creates an archive of FSTs.
template <class A>
class FarWriter {
 public:
  typedef A Arc;

  // Creates a new (empty) FST archive; returns NULL on error.
  static FarWriter *Create(const string &filename, FarType type = FAR_DEFAULT);

  // Adds an FST to the end of an archive. Keys must be non-empty and
  // in lexicographic order. FSTs must have a suitable write method.
  virtual void Add(const string &key, const Fst<A> &fst) = 0;

  virtual FarType Type() const = 0;

  virtual ~FarWriter() {}

 protected:
  FarWriter() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FarWriter);
};


// This class iterates through an existing archive of FSTs.
template <class A>
class FarReader {
 public:
 typedef A Arc;

  // Opens an existing FST archive in a single file; returns NULL on error.
  // Sets current position to the beginning of the achive.
  static FarReader *Open(const string &filename);

  // Opens an existing FST archive in multiple files; returns NULL on error.
  // Sets current position to the beginning of the achive.
  static FarReader *Open(const vector<string> &filenames);

  // Resets current posision to beginning of archive.
  virtual void Reset() = 0;

  // Sets current position to first entry >= key.  Returns true if a match.
  virtual bool Find(const string &key) = 0;

  // Current position at end of archive?
  virtual bool Done() const = 0;

  // Move current position to next FST.
  virtual void Next() = 0;

  // Returns key at the current position. This reference is invalidated if
  // the current position in the archive is changed.
  virtual const string &GetKey() const = 0;

  // Returns FST at the current position. This reference is invalidated if
  // the current position in the archive is changed.
  virtual const Fst<A> &GetFst() const = 0;

  virtual FarType Type() const = 0;

  virtual ~FarReader() {}

 protected:
  FarReader() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FarReader);
};


template <class A>
class FstWriter {
 public:
  void operator()(ostream &strm, const Fst<A> &fst) const {
    fst.Write(strm, FstWriteOptions());
  }
};


template <class A>
class STTableFarWriter : public FarWriter<A> {
 public:
  typedef A Arc;

  static STTableFarWriter *Create(const string filename) {
    STTableWriter<Fst<A>, FstWriter<A> > *writer =
        STTableWriter<Fst<A>, FstWriter<A> >::Create(filename);
    return new STTableFarWriter(writer);
  }

  void Add(const string &key, const Fst<A> &fst) { writer_->Add(key, fst); }

  FarType Type() const { return FAR_STTABLE; }

  ~STTableFarWriter() { delete writer_; }

 private:
  explicit STTableFarWriter(STTableWriter<Fst<A>, FstWriter<A> > *writer)
      : writer_(writer) {}

 private:
  STTableWriter<Fst<A>, FstWriter<A> > *writer_;

  DISALLOW_COPY_AND_ASSIGN(STTableFarWriter);
};


template <class A>
FarWriter<A> *FarWriter<A>::Create(const string &filename, FarType type) {
  switch(type) {
    case FAR_DEFAULT:
    case FAR_STTABLE:
      return STTableFarWriter<A>::Create(filename);
      break;
    default:
      LOG(ERROR) << "FarWriter::Create: unknown far type";
      return 0;
  }
}


template <class A>
class FstReader {
 public:
  Fst<A> *operator()(istream &strm) const {
    return Fst<A>::Read(strm, FstReadOptions());
  }
};


template <class A>
class STTableFarReader : public FarReader<A> {
 public:
  typedef A Arc;

  static STTableFarReader *Open(const string &filename) {
    STTableReader<Fst<A>, FstReader<A> > *reader =
        STTableReader<Fst<A>, FstReader<A> >::Open(filename);
    // TODO: error check
    return new STTableFarReader(reader);
  }

  static STTableFarReader *Open(const vector<string> &filenames) {
    STTableReader<Fst<A>, FstReader<A> > *reader =
        STTableReader<Fst<A>, FstReader<A> >::Open(filenames);
    // TODO: error check
    return new STTableFarReader(reader);
  }

  void Reset() { reader_->Reset(); }

  bool Find(const string &key) { return reader_->Find(key); }

  bool Done() const { return reader_->Done(); }

  void Next() { return reader_->Next(); }

  const string &GetKey() const { return reader_->GetKey(); }

  const Fst<A> &GetFst() const { return reader_->GetEntry(); }

  FarType Type() const { return FAR_STTABLE; }

  ~STTableFarReader() { delete reader_; }

 private:
  explicit STTableFarReader(STTableReader<Fst<A>, FstReader<A> > *reader)
      : reader_(reader) {}

 private:
  STTableReader<Fst<A>, FstReader<A> > *reader_;

  DISALLOW_COPY_AND_ASSIGN(STTableFarReader);
};


template <class A>
FarReader<A> *FarReader<A>::Open(const string &filename) {
  if (IsSTTable(filename))
    return STTableFarReader<A>::Open(filename);
  return 0;
}


template <class A>
FarReader<A> *FarReader<A>::Open(const vector<string> &filenames) {
  if (!filenames.empty() && IsSTTable(filenames[0]))
    return STTableFarReader<A>::Open(filenames);
  return 0;
}

}  // namespace fst;

#endif  // FST_EXTENSIONS_FAR_FAR_H__
