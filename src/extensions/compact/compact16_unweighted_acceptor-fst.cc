// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/compact-fst.h>

using fst::FstRegisterer;
using fst::CompactFst;
using fst::LogArc;
using fst::StdArc;
using fst::UnweightedAcceptorCompactor;

static FstRegisterer<
    CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc>, uint16>>
    CompactFst_StdArc_UnweightedAcceptorCompactor_uint16_registerer;
static FstRegisterer<
    CompactFst<LogArc, UnweightedAcceptorCompactor<LogArc>, uint16>>
    CompactFst_LogArc_UnweightedAcceptorCompactor_uint16_registerer;
