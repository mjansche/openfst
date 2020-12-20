// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/compact-fst.h>

using fst::FstRegisterer;
using fst::CompactUnweightedAcceptorFst;
using fst::LogArc;
using fst::StdArc;

static FstRegisterer<
    CompactUnweightedAcceptorFst<StdArc, uint64>>
    CompactUnweightedAcceptorFst_StdArc_uint64_registerer;
static FstRegisterer<
    CompactUnweightedAcceptorFst<LogArc, uint64>>
    CompactUnweightedAcceptorFst_LogArc_uint64_registerer;
