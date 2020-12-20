// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Classes and functions for registering and invoking Far main
// functions that support multiple and extensible arc types.

#ifndef FST_EXTENSIONS_FAR_MAIN_H_
#define FST_EXTENSIONS_FAR_MAIN_H_

#include <fst/extensions/far/far.h>

namespace fst {

FarEntryType StringToFarEntryType(const string &s);
FarTokenType StringToFarTokenType(const string &s);

// Return the 'FarType' value corresponding to a far type name.
FarType FarTypeFromString(const string &str);

// Return the textual name  corresponding to a 'FarType;.
string FarTypeToString(FarType type);

void ExpandArgs(int argc, char **argv, int *argcp, char ***argvp);

string LoadArcTypeFromFar(const string &far_fname);
string LoadArcTypeFromFst(const string &fst_fname);

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_MAIN_H_
