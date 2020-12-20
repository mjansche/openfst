// fstshortestdistance.cc

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
// Modified: jpr@google.com (Jake Ratkiewicz) to use FstClass
//
// \file
// Find shortest distances in an FST.

#include <string>
#include <vector>
using std::vector;

#include <fst/script/shortest-distance.h>
#include <fst/script/text-io.h>

DEFINE_bool(reverse, false, "Perform in the reverse direction");
DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_int64(nstate, fst::kNoStateId, "State number parameter");
DEFINE_string(queue_type, "auto", "Queue type: one of \"trivial\", "
              "\"fifo\", \"lifo\", \"top\", \"auto\".");
DEFINE_string(arc_filter, "any", "Arc filter: one of :"
              " \"any\", \"epsilon\", \"iepsilon\", \"oepsilon\"");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;

  string usage = "Finds shortest distance(s) in an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [distance.txt]]\n";

  std::set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_fname = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_fname = argc > 2 ? argv[2] : "";

  FstClass *ifst = FstClass::Read(in_fname);
  if (!ifst) return 1;

  vector<s::WeightClass> distance;

  s::ArcFilterType arc_filter;
  if (FLAGS_arc_filter == "any") {
    arc_filter = s::ANY_ARC_FILTER;
  } else if (FLAGS_arc_filter == "epsilon") {
    arc_filter = s::EPSILON_ARC_FILTER;
  } else if (FLAGS_arc_filter == "iepsilon") {
    arc_filter = s::INPUT_EPSILON_ARC_FILTER;
  } else if (FLAGS_arc_filter == "oepsilon") {
    arc_filter = s::OUTPUT_EPSILON_ARC_FILTER;
  } else {
    LOG(FATAL) << "Unknown arc filter type: " << FLAGS_arc_filter;
  }

  fst::QueueType qt;

  if (FLAGS_queue_type == "trivial") {
    qt = fst::TRIVIAL_QUEUE;
  } else if (FLAGS_queue_type == "fifo") {
    qt = fst::FIFO_QUEUE;
  } else if (FLAGS_queue_type == "lifo") {
    qt = fst::LIFO_QUEUE;
  } else if (FLAGS_queue_type == "top") {
    qt = fst::TOP_ORDER_QUEUE;
  } else if (FLAGS_queue_type == "auto") {
    qt = fst::AUTO_QUEUE;
  } else {
    LOG(FATAL) << "Unknown or unsupported queue type: " << FLAGS_queue_type;
  }

  if (FLAGS_reverse &&
      (qt != fst::AUTO_QUEUE || arc_filter != s::ANY_ARC_FILTER)) {
    LOG(FATAL) << "Specifying a non-default queue or arc filter type in "
        "conjunction with reverse is not supported.";
  }

  if (FLAGS_reverse) {
    s::ShortestDistance(*ifst, &distance, FLAGS_reverse, FLAGS_delta);
  } else {
    s::ShortestDistanceOptions opts(qt, arc_filter, FLAGS_nstate, FLAGS_delta);
    s::ShortestDistance(*ifst, &distance, opts);
  }

  s::WritePotentials(out_fname, distance);

  return 0;
}
