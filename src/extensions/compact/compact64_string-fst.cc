// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/compact-fst.h>

using fst::FstRegisterer;
using fst::CompactFst;
using fst::LogArc;
using fst::StdArc;
using fst::StringCompactor;

static FstRegisterer<CompactFst<StdArc, StringCompactor<StdArc>, uint64>>
    CompactFst_StdArc_StringCompactor_uint64_registerer;
static FstRegisterer<CompactFst<LogArc, StringCompactor<LogArc>, uint64>>
    CompactFst_LogArc_StringCompactor_uint64_registerer;
