// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/matcher-fst.h>

using fst::FstRegisterer;
using fst::StdOLabelLookAheadFst;
using fst::LogOLabelLookAheadFst;
using fst::LogArc;
using fst::StdArc;

// Register OLabelLookAhead Fsts with common arc types
static FstRegisterer<StdOLabelLookAheadFst>
    OLabelLookAheadFst_StdArc_registerer;
static FstRegisterer<LogOLabelLookAheadFst>
    OLabelLookAheadFst_LogArc_registerer;
