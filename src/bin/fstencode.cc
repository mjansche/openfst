// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
//  Encode transducer labels and/or weights.

#include <fst/script/decode.h>
#include <fst/script/encode.h>

/// EncodeMain specific flag definitions
DEFINE_bool(encode_labels, false, "Encode output labels");
DEFINE_bool(encode_weights, false, "Encode weights");
DEFINE_bool(encode_reuse, false, "Re-use existing codex");
DEFINE_bool(decode, false, "Decode labels and/or weights");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;

  string usage = "Encodes transducer labels and/or weights.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.fst codex [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  string in_name = (strcmp(argv[1], "-") != 0) ? argv[1] : "";
  string codex_name = argv[2];
  string out_name = argc > 3 ? argv[3] : "";

  MutableFstClass *fst = MutableFstClass::Read(in_name, true);
  if (!fst) return 1;

  if (FLAGS_decode == false) {
    uint32 flags = 0;
    flags |= FLAGS_encode_labels ? fst::kEncodeLabels : 0;
    flags |= FLAGS_encode_weights ? fst::kEncodeWeights : 0;
    s::Encode(fst, flags, FLAGS_encode_reuse, codex_name);
    fst->Write(out_name);
  } else {
    s::Decode(fst, codex_name);
    fst->Write(out_name);
  }

  delete fst;
  return 0;
}
