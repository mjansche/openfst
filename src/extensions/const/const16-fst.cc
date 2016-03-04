// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/const-fst.h>

using fst::FstRegisterer;
using fst::ConstFst;
using fst::LogArc;
using fst::Log64Arc;
using fst::StdArc;

// Register ConstFst for common arcs types with uint16 size type
static FstRegisterer<ConstFst<StdArc, uint16>>
    ConstFst_StdArc_uint16_registerer;
static FstRegisterer<ConstFst<LogArc, uint16>>
    ConstFst_LogArc_uint16_registerer;
static FstRegisterer<ConstFst<Log64Arc, uint16>>
    ConstFst_Log64Arc_uint16_registerer;
