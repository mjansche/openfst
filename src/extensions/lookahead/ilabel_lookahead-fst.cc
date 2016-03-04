// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/matcher-fst.h>

using fst::FstRegisterer;
using fst::StdILabelLookAheadFst;
using fst::LogILabelLookAheadFst;
using fst::LogArc;
using fst::StdArc;

// Register InputLabelLookAhead Fsts with common arc types
static FstRegisterer<StdILabelLookAheadFst>
    ILabelLookAheadFst_StdArc_registerer;
static FstRegisterer<LogILabelLookAheadFst>
    ILabelLookAheadFst_LogArc_registerer;
