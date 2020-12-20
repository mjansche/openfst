// info.h

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
// Author: riley@google.com (Michael Riley)
//
// \file
// Class to compute various information about FSTs, helper class for fstinfo.cc

#ifndef FST_SCRIPT_INFO_IMPL_H_
#define FST_SCRIPT_INFO_IMPL_H_

#include <string>
#include <vector>
using std::vector;

#include <fst/connect.h>
#include <fst/dfs-visit.h>
#include <fst/fst.h>
#include <fst/lookahead-matcher.h>
#include <fst/matcher.h>
#include <fst/queue.h>
#include <fst/test-properties.h>
#include <fst/visit.h>

namespace fst {

// Compute various information about FSTs, helper class for fstinfo.cc.
// WARNING: Stand-alone use of this class is not recommended, most code
// should call directly the relevant library functions: Fst<A>::NumStates,
// Fst<A>::NumArcs, TestProperties, ...
template <class A> class FstInfo {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  // When info_type is "short" (or "auto" and not an ExpandedFst)
  // then only minimal info is computed and can be requested.
  FstInfo(const Fst<A> &fst, bool test_properties,
          const string &arc_filter_type = "any",
          string info_type = "auto")
      : fst_type_(fst.Type()),
        input_symbols_(fst.InputSymbols() ?
                       fst.InputSymbols()->Name() : "none"),
        output_symbols_(fst.OutputSymbols() ?
                        fst.OutputSymbols()->Name() : "none"),
        nstates_(0), narcs_(0), start_(kNoStateId), nfinal_(0),
        nepsilons_(0), niepsilons_(0), noepsilons_(0),
        naccess_(0), ncoaccess_(0), nconnect_(0), ncc_(0), nscc_(0),
        arc_filter_type_(arc_filter_type) {
    if (info_type == "long")
      long_info_ = true;
    else if (info_type == "short")
      long_info_ = false;
    else if (info_type == "auto")
      long_info_ = fst.Properties(kExpanded, false);
    else
      LOG(FATAL) << "Bad info type: " << info_type;

    if (!long_info_)
      return;

    start_ = fst.Start();
    properties_ = fst.Properties(kFstProperties, test_properties);

    for (StateIterator< Fst<A> > siter(fst);
         !siter.Done();
         siter.Next()) {
      ++nstates_;
      StateId s = siter.Value();
      if (fst.Final(s) != Weight::Zero())
        ++nfinal_;
      for (ArcIterator< Fst<A> > aiter(fst, s);
           !aiter.Done();
           aiter.Next()) {
        const A &arc = aiter.Value();
        ++narcs_;
        if (arc.ilabel == 0 && arc.olabel == 0)
          ++nepsilons_;
        if (arc.ilabel == 0)
          ++niepsilons_;
        if (arc.olabel == 0)
          ++noepsilons_;
      }
    }

    {
      vector<StateId> cc;
      CcVisitor<Arc> cc_visitor(&cc);
      FifoQueue<StateId> fifo_queue;
      if (arc_filter_type == "any")
        Visit(fst, &cc_visitor, &fifo_queue);
      else if (arc_filter_type == "epsilon")
        Visit(fst, &cc_visitor, &fifo_queue, EpsilonArcFilter<Arc>());
      else if (arc_filter_type == "iepsilon")
        Visit(fst, &cc_visitor, &fifo_queue, InputEpsilonArcFilter<Arc>());
      else if (arc_filter_type == "oepsilon")
        Visit(fst, &cc_visitor, &fifo_queue, OutputEpsilonArcFilter<Arc>());
      else
        LOG(FATAL) << "Bad arc filter type: " << arc_filter_type;

      for (StateId s = 0; s < cc.size(); ++s) {
        if (cc[s] >= ncc_)
          ncc_ = cc[s] + 1;
      }
    }

    {
      vector<StateId> scc;
      vector<bool> access, coaccess;
      uint64 props = 0;
      SccVisitor<Arc> scc_visitor(&scc, &access, &coaccess, &props);
      if (arc_filter_type == "any")
        DfsVisit(fst, &scc_visitor);
      else if (arc_filter_type == "epsilon")
        DfsVisit(fst, &scc_visitor, EpsilonArcFilter<Arc>());
      else if (arc_filter_type == "iepsilon")
        DfsVisit(fst, &scc_visitor, InputEpsilonArcFilter<Arc>());
      else if (arc_filter_type == "oepsilon")
        DfsVisit(fst, &scc_visitor, OutputEpsilonArcFilter<Arc>());
      else
        LOG(FATAL) << "Bad arc filter type: " << arc_filter_type;


      for (StateId s = 0; s < scc.size(); ++s) {
        if (access[s])
          ++naccess_;
        if (coaccess[s])
          ++ncoaccess_;
        if (access[s] && coaccess[s])
          ++nconnect_;
        if (scc[s] >= nscc_)
          nscc_ = scc[s] + 1;
      }
    }

    LookAheadMatcher< Fst<A> > imatcher(fst, MATCH_INPUT);
    input_match_type_ =  imatcher.Type(test_properties);
    input_lookahead_ =  imatcher.Flags() & kInputLookAheadMatcher;

    LookAheadMatcher< Fst<A> > omatcher(fst, MATCH_OUTPUT);
    output_match_type_ =  omatcher.Type(test_properties);
    output_lookahead_ =  omatcher.Flags() & kOutputLookAheadMatcher;
  }

  // Short info
  const string& FstType() const { return fst_type_; }
  const string& ArcType() const { return A::Type(); }
  const string& InputSymbols() const { return input_symbols_; }
  const string& OutputSymbols() const { return output_symbols_; }
  const bool LongInfo() const { return long_info_; }
  const string& ArcFilterType() const { return arc_filter_type_; }

  // Long info
  MatchType InputMatchType() const { CheckLong(); return input_match_type_; }
  MatchType OutputMatchType() const { CheckLong(); return output_match_type_; }
  bool InputLookAhead() const { CheckLong(); return input_lookahead_; }
  bool OutputLookAhead() const { CheckLong();  return output_lookahead_; }
  int64 NumStates() const { CheckLong();  return nstates_; }
  int64 NumArcs() const { CheckLong();  return narcs_; }
  int64 Start() const { CheckLong();  return start_; }
  int64 NumFinal() const { CheckLong();  return nfinal_; }
  int64 NumEpsilons() const { CheckLong();  return nepsilons_; }
  int64 NumInputEpsilons() const { CheckLong(); return niepsilons_; }
  int64 NumOutputEpsilons() const { CheckLong(); return noepsilons_; }
  int64 NumAccessible() const { CheckLong(); return naccess_; }
  int64 NumCoAccessible() const { CheckLong(); return ncoaccess_; }
  int64 NumConnected() const { CheckLong(); return nconnect_; }
  int64 NumCc() const { CheckLong(); return ncc_; }
  int64 NumScc() const { CheckLong(); return nscc_; }
  uint64 Properties() const { CheckLong(); return properties_; }

 private:
  void CheckLong() const {
    if (!long_info_)
      LOG(FATAL) << "FstInfo: method only available with long info version";
  }

  string fst_type_;
  string input_symbols_;
  string output_symbols_;
  int64 nstates_;
  int64 narcs_;
  int64 start_;
  int64 nfinal_;
  int64 nepsilons_;
  int64 niepsilons_;
  int64 noepsilons_;
  int64 naccess_;
  int64 ncoaccess_;
  int64 nconnect_;
  int64 ncc_;
  int64 nscc_;
  MatchType input_match_type_;
  MatchType output_match_type_;
  bool input_lookahead_;
  bool output_lookahead_;
  uint64 properties_;
  string arc_filter_type_;
  bool long_info_;
  DISALLOW_COPY_AND_ASSIGN(FstInfo);
};

template <class A>
void PrintFstInfo(const FstInfo<A> &fstinfo, bool pipe = false) {
  FILE *fp = pipe ? stderr : stdout;

  fprintf(fp, "%-50s%s\n", "fst type",
          fstinfo.FstType().c_str());
    fprintf(fp, "%-50s%s\n", "arc type",
            fstinfo.ArcType().c_str());
  fprintf(fp, "%-50s%s\n", "input symbol table",
          fstinfo.InputSymbols().c_str());
  fprintf(fp, "%-50s%s\n", "output symbol table",
          fstinfo.OutputSymbols().c_str());

  if (!fstinfo.LongInfo())
    return;

  fprintf(fp, "%-50s%lld\n", "# of states",
          fstinfo.NumStates());
  fprintf(fp, "%-50s%lld\n", "# of arcs",
          fstinfo.NumArcs());
  fprintf(fp, "%-50s%lld\n", "initial state",
          fstinfo.Start());
  fprintf(fp, "%-50s%lld\n", "# of final states",
          fstinfo.NumFinal());
  fprintf(fp, "%-50s%lld\n", "# of input/output epsilons",
          fstinfo.NumEpsilons());
  fprintf(fp, "%-50s%lld\n", "# of input epsilons",
          fstinfo.NumInputEpsilons());
  fprintf(fp, "%-50s%lld\n", "# of output epsilons",
          fstinfo.NumOutputEpsilons());

  string arc_type = "";
  if (fstinfo.ArcFilterType() == "epsilon")
    arc_type = "epsilon ";
  else if (fstinfo.ArcFilterType() == "iepsilon")
    arc_type = "input-epsilon ";
  else if (fstinfo.ArcFilterType() == "oepsilon")
    arc_type = "output-epsilon ";

  string accessible_label = "# of " +  arc_type + "accessible states";
  fprintf(fp, "%-50s%lld\n", accessible_label.c_str(),
          fstinfo.NumAccessible());
  string coaccessible_label = "# of " +  arc_type + "coaccessible states";
  fprintf(fp, "%-50s%lld\n", coaccessible_label.c_str(),
          fstinfo.NumCoAccessible());
  string connected_label = "# of " +  arc_type + "connected states";
  fprintf(fp, "%-50s%lld\n", connected_label.c_str(),
          fstinfo.NumConnected());
  string numcc_label = "# of " +  arc_type + "connected components";
  fprintf(fp, "%-50s%lld\n", numcc_label.c_str(),
          fstinfo.NumCc());
  string numscc_label = "# of " +  arc_type + "strongly conn components";
  fprintf(fp, "%-50s%lld\n", numscc_label.c_str(),
          fstinfo.NumScc());

  fprintf(fp, "%-50s%c\n", "input matcher",
          fstinfo.InputMatchType() == MATCH_INPUT ? 'y' :
          fstinfo.InputMatchType() == MATCH_NONE ? 'n' : '?');
  fprintf(fp, "%-50s%c\n", "output matcher",
          fstinfo.OutputMatchType() == MATCH_OUTPUT ? 'y' :
          fstinfo.OutputMatchType() == MATCH_NONE ? 'n' : '?');
  fprintf(fp, "%-50s%c\n", "input lookahead",
          fstinfo.InputLookAhead() ? 'y' : 'n');
  fprintf(fp, "%-50s%c\n", "output lookahead",
          fstinfo.OutputLookAhead() ? 'y' : 'n');

  uint64 prop = 1;
  for (int i = 0; i < 64; ++i, prop <<= 1) {
    if (prop & kBinaryProperties) {
      char value = 'n';
      if (fstinfo.Properties() & prop) value = 'y';
      fprintf(fp, "%-50s%c\n", PropertyNames[i], value);
    } else if (prop & kPosTrinaryProperties) {
      char value = '?';
      if (fstinfo.Properties() & prop) value = 'y';
      else if (fstinfo.Properties() & prop << 1) value = 'n';
      fprintf(fp, "%-50s%c\n", PropertyNames[i], value);
    }
  }
}

}  // namespace fst

#endif  // FST_SCRIPT_INFO_IMPL_H_
