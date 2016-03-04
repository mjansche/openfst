// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/fst.h>
#include <fst/matcher-fst.h>

using fst::FstRegisterer;
using fst::StdArcLookAheadFst;
using fst::LogArcLookAheadFst;
using fst::LogArc;
using fst::StdArc;

// Register ArcLookAhead Fsts with common arc types
static FstRegisterer<StdArcLookAheadFst> ArcLookAheadFst_StdArc_registerer;
static FstRegisterer<LogArcLookAheadFst> ArcLookAheadFst_LogArc_registerer;
