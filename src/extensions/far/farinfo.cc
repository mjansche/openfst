// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Prints some basic information about the FSTs in an FST archive.

#include <fst/extensions/far/farscript.h>
#include <fst/extensions/far/main.h>

DEFINE_string(begin_key, "",
              "First key to extract (def: first key in archive)");
DEFINE_string(end_key, "", "Last key to extract (def: last key in archive)");

DEFINE_bool(list_fsts, false, "Display FST information for each key");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Prints some basic information about the FSTs in an FST ";
  usage += "archive.\n\n Usage:";
  usage += argv[0];
  usage += " [in1.far in2.far...]\n";
  usage += "  Flags: begin_key end_key list_fsts";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  fst::ExpandArgs(argc, argv, &argc, &argv);

  std::vector<string> filenames;
  for (int i = 1; i < argc; ++i)
    filenames.push_back(strcmp(argv[i], "") != 0 ? argv[i] : "");
  if (filenames.empty()) filenames.push_back("");

  s::FarInfo(filenames, fst::LoadArcTypeFromFar(filenames[0]),
             FLAGS_begin_key, FLAGS_end_key, FLAGS_list_fsts);
}
