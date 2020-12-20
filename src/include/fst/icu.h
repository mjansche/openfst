// icu.h

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
// Author: roubert@google.com (Fredrik Roubert)

// Wrapper class for UErrorCode, with conversion operators for direct use in
// ICU C and C++ APIs.
//
// Features:
// - The constructor initializes the internal UErrorCode to U_ZERO_ERROR,
//   removing one common source of errors.
// - Same use in C APIs taking a UErrorCode* (pointer) and C++ taking
//   UErrorCode& (reference), via conversion operators.
// - Automatic checking for success when it goes out of scope. On failure,
//   the destructor will LOG(FATAL) an error message.
//
// Most of ICU will handle errors gracefully and provide sensible fallbacks.
// Using IcuErrorCode, it is therefore possible to write very compact code
// that does sensible things on failure and provides logging for debugging.
//
// Example:
//
// IcuErrorCode icuerrorcode;
// return collator.compareUTF8(a, b, icuerrorcode) == UCOL_EQUAL;

#ifndef FST_LIB_ICU_H_
#define FST_LIB_ICU_H_

#include <unicode/errorcode.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>
#include <unicode/utf8.h>

class IcuErrorCode : public icu::ErrorCode {
 public:
  IcuErrorCode() {}
  virtual ~IcuErrorCode() { if (isFailure()) handleFailure(); }

  // Redefine 'errorName()' in order to be compatible with ICU version 4.2
  const char* errorName() const {
    return u_errorName(errorCode);
  }

 protected:
  virtual void handleFailure() const {
    LOG(FATAL) << errorName();
}

 private:
  DISALLOW_COPY_AND_ASSIGN(IcuErrorCode);
};

namespace fst {

template <class Label>
bool UTF8StringToLabels(const string &str, vector<Label> *labels) {
  const char *c_str = str.c_str();
  int32_t length = str.size();
  UChar32 c;
  for (int32_t i = 0; i < length; /* no update */) {
    U8_NEXT(c_str, i, length, c);
    if (c < 0) {
      LOG(ERROR) << "UTF8StringToLabels: Invalid character found: " << c;
      return false;
    }
    labels->push_back(c);
  }
  return true;
}

template <class Label>
bool LabelsToUTF8String(const vector<Label> &labels, string *str) {
  icu::UnicodeString u_str;
  char c_str[5];
  for (size_t i = 0; i < labels.size(); ++i) {
    u_str.setTo(labels[i]);
    IcuErrorCode error;
    u_strToUTF8(c_str, 5, NULL, u_str.getTerminatedBuffer(), -1, error);
    if (error.isFailure()) {
      LOG(ERROR) << "LabelsToUTF8String: Bad encoding: "
                 << error.errorName();
      return false;
    }
    *str += c_str;
  }
  return true;
}

}  // namespace fst

#endif  // FST_LIB_ICU_H_
