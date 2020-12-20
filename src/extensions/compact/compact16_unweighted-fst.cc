// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/compact-fst.h>

using fst::FstRegisterer;
using fst::CompactUnweightedFst;
using fst::LogArc;
using fst::StdArc;

static FstRegisterer<CompactUnweightedFst<StdArc, uint16>>
    CompactUnweightedFst_StdArc_uint16_registerer;
static FstRegisterer<CompactUnweightedFst<LogArc, uint16>>
    CompactUnweightedFst_LogArc_uint16_registerer;
