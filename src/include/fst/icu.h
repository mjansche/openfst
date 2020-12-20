// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// This library implements an unrestricted Thompson/Pike UTF-8 parser and
// serializer.  UTF-8 is a restricted subset of this byte stream encoding. See
// http://en.wikipedia.org/wiki/UTF-8 for a good description of the encoding
// details.

#ifndef FST_LIB_ICU_H_
#define FST_LIB_ICU_H_

#include <sstream>
#include <vector>

namespace fst {

template <class Label>
bool UTF8StringToLabels(const string &str, std::vector<Label> *labels) {
  const char *data = str.data();
  size_t length = str.size();
  for (int i = 0; i < length; /* no update */) {
    int c = data[i++] & 0xff;
    if ((c & 0x80) == 0) {
      labels->push_back(c);
    } else {
      if ((c & 0xc0) == 0x80) {
        LOG(ERROR) << "UTF8StringToLabels: Continuation byte as lead byte";
        return false;
      }
      int count =
          (c >= 0xc0) + (c >= 0xe0) + (c >= 0xf0) + (c >= 0xf8) + (c >= 0xfc);
      int code = c & ((1 << (6 - count)) - 1);
      while (count != 0) {
        if (i == length) {
          LOG(ERROR) << "UTF8StringToLabels: Truncated UTF-8 byte sequence";
          return false;
        }
        char cb = data[i++];
        if ((cb & 0xc0) != 0x80) {
          LOG(ERROR) << "UTF8StringToLabels: Missing/invalid continuation byte";
          return false;
        }
        code = (code << 6) | (cb & 0x3f);
        count--;
      }
      if (code < 0) {
        // This should not be able to happen.
        LOG(ERROR) << "UTF8StringToLabels: Invalid character found: " << c;
        return false;
      }
      labels->push_back(code);
    }
  }
  return true;
}

template <class Label>
bool LabelsToUTF8String(const std::vector<Label> &labels, string *str) {
  std::ostringstream ostr;
  for (size_t i = 0; i < labels.size(); ++i) {
    int32_t code = labels[i];
    if (code < 0) {
      LOG(ERROR) << "LabelsToUTF8String: Invalid character found: " << code;
      return false;
    } else if (code < 0x80) {
      ostr << static_cast<char>(code);
    } else if (code < 0x800) {
      ostr << static_cast<char>((code >> 6) | 0xc0);
      ostr << static_cast<char>((code & 0x3f) | 0x80);
    } else if (code < 0x10000) {
      ostr << static_cast<char>((code >> 12) | 0xe0);
      ostr << static_cast<char>(((code >> 6) & 0x3f) | 0x80);
      ostr << static_cast<char>((code & 0x3f) | 0x80);
    } else if (code < 0x200000) {
      ostr << static_cast<char>((code >> 18) | 0xf0);
      ostr << static_cast<char>(((code >> 12) & 0x3f) | 0x80);
      ostr << static_cast<char>(((code >> 6) & 0x3f) | 0x80);
      ostr << static_cast<char>((code & 0x3f) | 0x80);
    } else if (code < 0x4000000) {
      ostr << static_cast<char>((code >> 24) | 0xf8);
      ostr << static_cast<char>(((code >> 18) & 0x3f) | 0x80);
      ostr << static_cast<char>(((code >> 12) & 0x3f) | 0x80);
      ostr << static_cast<char>(((code >> 6) & 0x3f) | 0x80);
      ostr << static_cast<char>((code & 0x3f) | 0x80);
    } else {
      ostr << static_cast<char>((code >> 30) | 0xfc);
      ostr << static_cast<char>(((code >> 24) & 0x3f) | 0x80);
      ostr << static_cast<char>(((code >> 18) & 0x3f) | 0x80);
      ostr << static_cast<char>(((code >> 12) & 0x3f) | 0x80);
      ostr << static_cast<char>(((code >> 6) & 0x3f) | 0x80);
      ostr << static_cast<char>((code & 0x3f) | 0x80);
    }
  }
  *str = ostr.str();
  return true;
}

}  // namespace fst

#endif  // FST_LIB_ICU_H_
