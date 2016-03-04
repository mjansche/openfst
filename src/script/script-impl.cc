// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <string>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

// Utility function for checking that arc types match.

bool ArcTypesMatch(const FstClass &a, const FstClass &b,
                   const string &op_name) {
  if (a.ArcType() != b.ArcType()) {
    FSTERROR() << "FSTs with non-matching arc types passed to " << op_name
               << ":\n\t" << a.ArcType() << " and " << b.ArcType();
    return false;
  }
  return true;
}

}  // namespace script
}  // namespace fst
