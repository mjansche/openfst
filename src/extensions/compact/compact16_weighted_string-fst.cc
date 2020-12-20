// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/compact-fst.h>

using fst::FstRegisterer;
using fst::CompactFst;
using fst::LogArc;
using fst::StdArc;
using fst::WeightedStringCompactor;

static FstRegisterer<
    CompactFst<StdArc, WeightedStringCompactor<StdArc>, uint16>>
    CompactFst_StdArc_WeightedStringCompactor_uint16_registerer;
static FstRegisterer<
    CompactFst<LogArc, WeightedStringCompactor<LogArc>, uint16>>
    CompactFst_LogArc_WeightedStringCompactor_uint16_registerer;
