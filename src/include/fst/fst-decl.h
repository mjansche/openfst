// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// This file contains declarations of classes in the Fst template library.

#ifndef FST_LIB_FST_DECL_H_
#define FST_LIB_FST_DECL_H_

#include <sys/types.h>
#include <memory>  // for allocator<>

#include <fst/types.h>

namespace fst {

class SymbolTable;
class SymbolTableIterator;

template <class W>
class FloatWeightTpl;
template <class W>
class TropicalWeightTpl;
template <class W>
class LogWeightTpl;
template <class W>
class MinMaxWeightTpl;

typedef FloatWeightTpl<float> FloatWeight;
typedef TropicalWeightTpl<float> TropicalWeight;
typedef LogWeightTpl<float> LogWeight;
typedef MinMaxWeightTpl<float> MinMaxWeight;

template <class W>
class ArcTpl;
typedef ArcTpl<TropicalWeight> StdArc;
typedef ArcTpl<LogWeight> LogArc;

template <class E, class U>
class DefaultCompactStore;

template <class A, class C, class U = uint32,
          class S = DefaultCompactStore<typename C::Element, U>>
class CompactFst;
template <class A, class U = uint32>
class ConstFst;
template <class A, class W, class M>
class EditFst;
template <class A>
class ExpandedFst;
template <class A>
class Fst;
template <class A>
class MutableFst;
template <class A, class M = std::allocator<A>>
class VectorState;
template <class A, class S = VectorState<A>>
class VectorFst;

template <class A>
class DefaultCacheStore;
template <class A, class P = ssize_t>
class DefaultReplaceStateTable;

template <class A, class C>
class ArcSortFst;
template <class A>
class ClosureFst;
template <class A, class C = DefaultCacheStore<A>>
class ComposeFst;
template <class A>
class ConcatFst;
template <class A>
class DeterminizeFst;
template <class A>
class DifferenceFst;
template <class A>
class IntersectFst;
template <class A>
class InvertFst;
template <class A, class B, class C>
class ArcMapFst;
template <class A>
class ProjectFst;
template <class A, class B, class S>
class RandGenFst;
template <class A>
class RelabelFst;
template <class A, class T = DefaultReplaceStateTable<A>,
          class C = DefaultCacheStore<A>>
class ReplaceFst;  // NOLINT
template <class A>
class RmEpsilonFst;
template <class A>
class UnionFst;

template <class T, class Compare>
class Heap;

template <class A>
class AcceptorCompactor;
template <class A>
class StringCompactor;
template <class A>
class UnweightedAcceptorCompactor;
template <class A>
class UnweightedCompactor;
template <class A>
class WeightedStringCompactor;

typedef ConstFst<StdArc> StdConstFst;
typedef ExpandedFst<StdArc> StdExpandedFst;
typedef Fst<StdArc> StdFst;
typedef MutableFst<StdArc> StdMutableFst;
typedef VectorFst<StdArc> StdVectorFst;

template <class C>
class StdArcSortFst;
typedef ClosureFst<StdArc> StdClosureFst;
typedef ComposeFst<StdArc> StdComposeFst;
typedef ConcatFst<StdArc> StdConcatFst;
typedef DeterminizeFst<StdArc> StdDeterminizeFst;
typedef DifferenceFst<StdArc> StdDifferenceFst;
typedef IntersectFst<StdArc> StdIntersectFst;
typedef InvertFst<StdArc> StdInvertFst;
typedef ProjectFst<StdArc> StdProjectFst;
typedef RelabelFst<StdArc> StdRelabelFst;
typedef ReplaceFst<StdArc> StdReplaceFst;
typedef RmEpsilonFst<StdArc> StdRmEpsilonFst;
typedef UnionFst<StdArc> StdUnionFst;

template <typename T>
class IntegerFilterState;
typedef IntegerFilterState<signed char> CharFilterState;
typedef IntegerFilterState<short> ShortFilterState;
typedef IntegerFilterState<int> IntFilterState;

template <class F>
class Matcher;
template <class M1, class M2 = M1>
class NullComposeFilter;
template <class M1, class M2 = M1>
class TrivialComposeFilter;
template <class M1, class M2 = M1>
class SequenceComposeFilter;
template <class M1, class M2 = M1>
class AltSequenceComposeFilter;
template <class M1, class M2 = M1>
class MatchComposeFilter;

}  // namespace fst

#endif  // FST_LIB_FST_DECL_H_
