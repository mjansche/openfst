// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/compact-fst.h>

using fst::FstRegisterer;
using fst::CompactWeightedStringFst;
using fst::LogArc;
using fst::StdArc;

static FstRegisterer<
    CompactWeightedStringFst<StdArc, uint16>>
    CompactWeightedStringFst_StdArc_uint16_registerer;
static FstRegisterer<
    CompactWeightedStringFst<LogArc, uint16>>
    CompactWeightedStringFst_LogArc_uint16_registerer;
