// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/compact-fst.h>

using fst::FstRegisterer;
using fst::CompactAcceptorFst;
using fst::LogArc;
using fst::StdArc;

static FstRegisterer<CompactAcceptorFst<StdArc, uint16>>
    CompactAcceptorFst_StdArc_uint16_registerer;
static FstRegisterer<CompactAcceptorFst<LogArc, uint16>>
    CompactAcceptorFst_LogArc_uint16_registerer;
