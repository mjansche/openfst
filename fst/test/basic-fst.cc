// basic-fst.cc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: riley@google.com (Michael Riley)
//
// Source for basic fst shared object.

#include "fst/test/basic-fst.h"

using fst::BasicFst;
using fst::StdArc;
using fst::LogArc;

extern "C" {

void basic_fst_init() {
  REGISTER_FST(BasicFst, StdArc);
  REGISTER_FST(BasicFst, LogArc);
}

}
