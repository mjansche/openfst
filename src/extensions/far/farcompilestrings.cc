// farcompilestrings.cc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2010 Google, Inc.
// Author: allauzen@google.com (Cyril Allauzen)
// Modified: jpr@google.com (Jake Ratkiewicz) to use new arc-type dispatching
//
// \file
// Compiles a set of stings as FSTs and stores them in a finite-state
// archive.
//

#include <fst/extensions/far/farscript.h>
#include <fst/extensions/far/main.h>
#include <iostream>
#include <fstream>

DEFINE_string(key_prefix, "", "Prefix to append to keys");
DEFINE_string(key_suffix, "", "Suffix to append to keys");
DEFINE_int32(generate_keys, 0,
             "Generate N digit numeric keys (def: use file basenames)");
DEFINE_string(far_type, "default", "FAR file format type: one of: ");
DEFINE_bool(allow_negative_labels, false,
            "Allow negative labels (not recommended; may cause conflicts)");
DEFINE_string(arc_type, "standard", "Output arc type");
DEFINE_string(entry_type, "line", "Entry type: one of : "
              "\"file\" (one FST per file), \"line\" (one FST per line)");
DEFINE_string(fst_type, "vector", "Output FST type");
DEFINE_string(token_type, "symbol", "Token type: one of : "
              "\"symbol\", \"byte\", \"utf8\"");
DEFINE_string(symbols, "", "Label symbol table");
DEFINE_bool(file_list_input, false,
            "Each input files contains a list of files to be processed");


int  main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Compiles a set of strings as FSTs and stores them in";
  usage += " a finite-state archive.\n\n Usage:";
  usage += argv[0];
  usage += " in1.txt [in2.txt ...] out.far\n";

  std::set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, true);

  if (argc < 3) {
    ShowUsage();
    return 1;
  }

  vector<string> in_fnames(argc - 2);

  for (unsigned i = 1; i < argc - 1; ++i) {
    in_fnames[i - 1] = argv[i];
  }

  string out_fname = argv[argc - 1];

  fst::FarEntryType fet = fst::StringToFarEntryType(FLAGS_entry_type);
  fst::FarTokenType ftt = fst::StringToFarTokenType(FLAGS_token_type);
  fst::FarType far_type = fst::FarTypeFromString(FLAGS_far_type);

  s::FarCompileStrings(in_fnames, out_fname, FLAGS_arc_type, FLAGS_fst_type,
                       far_type, FLAGS_generate_keys, fet, ftt,
                       FLAGS_symbols, FLAGS_allow_negative_labels,
                       FLAGS_file_list_input, FLAGS_key_prefix,
                       FLAGS_key_suffix);

  return 0;
}
