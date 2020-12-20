// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/const-fst.h>

using fst::FstRegisterer;
using fst::ConstFst;
using fst::LogArc;
using fst::Log64Arc;
using fst::StdArc;

// Register ConstFst for common arcs types with uint8 size type
static FstRegisterer<ConstFst<StdArc, uint8>> ConstFst_StdArc_uint8_registerer;
static FstRegisterer<ConstFst<LogArc, uint8>> ConstFst_LogArc_uint8_registerer;
static FstRegisterer<ConstFst<Log64Arc, uint8>>
    ConstFst_Log64Arc_uint8_registerer;
