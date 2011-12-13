/**
 * @file neighbor_search.hpp
 * @author Ryan Curtin
 *
 * Defines the NeighborSearch class, which performs an abstract
 * nearest-neighbor-like query on two datasets.
 */
#ifndef __MLPACK_METHODS_NEIGHBOR_SEARCH_NEIGHBOR_SEARCH_HPP
#define __MLPACK_METHODS_NEIGHBOR_SEARCH_NEIGHBOR_SEARCH_HPP

#include <mlpack/core.hpp>
#include <mlpack/core/tree/bounds.hpp>
#include <mlpack/core/tree/binary_space_tree.hpp>
#include <vector>
#include <string>

#include <mlpack/core/metrics/lmetric.hpp>
#include "sort_policies/nearest_neighbor_sort.hpp"

namespace mlpack {
namespace neighbor /** Neighbor-search routines.  These include
                    * all-nearest-neighbors and all-furthest-neighbors
                    * searches. */ {

/**
 * Extra data for each node in the tree.  For neighbor searches, each node only
 * needs to store a bound on neighbor distances.
 */
template<typename SortPolicy>
class QueryStat
{
 private:
  //! The bound on the node's neighbor distances.
  double bound;

 public:
  /**
   * Initialize the statistic with the worst possible distance according to
   * our sorting policy.
   */
  QueryStat() : bound(SortPolicy::WorstDistance()) { }

  //! Get the bound.
  const double Bound() const { return bound; }
  //! Modify the bound.
  double& Bound() { return bound; }
};

/**
 * The NeighborSearch class is a template class for performing distance-based
 * neighbor searches.  It takes a query dataset and a reference dataset (or just
 * a reference dataset) and, for each point in the query dataset, finds the k
 * neighbors in the reference dataset which have the 'best' distance according
 * to a given sorting policy.  A constructor is given which takes only a
 * reference dataset, and if that constructor is used, the given reference
 * dataset is also used as the query dataset.
 *
 * The template parameters SortPolicy and Metric define the sort function used
 * and the metric (distance function) used.  More information on those classes
 * can be found in the NearestNeighborSort class and the kernel::ExampleKernel
 * class.
 *
 * @tparam SortPolicy The sort policy for distances; see NearestNeighborSort.
 * @tparam MetricType The metric to use for computation.
 * @tparam TreeType The tree type to use.
 */
template<typename SortPolicy = NearestNeighborSort,
         typename MetricType = mlpack::metric::SquaredEuclideanDistance,
         typename TreeType = tree::BinarySpaceTree<bound::HRectBound<2>,
                                                   QueryStat<SortPolicy> > >
class NeighborSearch
{
 public:
  /**
   * Initialize the NeighborSearch object, passing both a query and reference
   * dataset.  Optionally, already-built trees can be passed, for the case where
   * a special tree-building procedure is needed.  If referenceTree is given, it
   * is assumed that the points in referenceTree correspond to the points in
   * referenceSet.  The same is true for queryTree and querySet.  An initialized
   * distance metric can be given, for cases where the metric has internal data
   * (i.e. the distance::MahalanobisDistance class).
   *
   * If naive mode is being used and a pre-built tree is given, it may not work:
   * naive mode operates by building a one-node tree (the root node holds all
   * the points).  If that condition is not satisfied with the pre-built tree,
   * then naive mode will not work.
   *
   * @param referenceSet Set of reference points.
   * @param querySet Set of query points.
   * @param naive If true, O(n^2) naive search will be used (as opposed to
   *      dual-tree search).  This overrides singleMode (if it is set to true).
   * @param singleMode If true, single-tree search will be used (as opposed to
   *      dual-tree search).
   * @param leafSize Leaf size for tree construction (ignored if tree is given).
   * @param referenceTree Optionally pass a pre-built tree for the reference
   *      set.
   * @param queryTree Optionally pass a pre-built tree for the query set.
   * @param metric An optional instance of the MetricType class.
   */
  NeighborSearch(const arma::mat& referenceSet,
                 const arma::mat& querySet,
                 const bool naive = false,
                 const bool singleMode = false,
                 const size_t leafSize = 20,
                 TreeType* referenceTree = NULL,
                 TreeType* queryTree = NULL,
                 const MetricType metric = MetricType());

  /**
   * Initialize the NeighborSearch object, passing only one dataset, which is
   * used as both the query and the reference dataset.  Optionally, an
   * already-built tree can be passed, for the case where a special
   * tree-building procedure is needed.  If referenceTree is given, it is
   * assumed that the points in referenceTree correspond to the points in
   * referenceSet.  An initialized distance metric can be given, for cases where
   * the metric has internal data (i.e. the distance::MahalanobisDistance
   * class).
   *
   * If naive mode is being used and a pre-built tree is given, it may not work:
   * naive mode operates by building a one-node tree (the root node holds all
   * the points).  If that condition is not satisfied with the pre-built tree,
   * then naive mode will not work.
   *
   * @param referenceSet Set of reference points.
   * @param naive If true, O(n^2) naive search will be used (as opposed to
   *      dual-tree search).  This overrides singleMode (if it is set to true).
   * @param singleMode If true, single-tree search will be used (as opposed to
   *      dual-tree search).
   * @param leafSize Leaf size for tree construction (ignored if tree is given).
   * @param referenceTree Optionally pass a pre-built tree for the reference
   *      set.
   * @param metric An optional instance of the MetricType class.
   */
  NeighborSearch(const arma::mat& referenceSet,
                 const bool naive = false,
                 const bool singleMode = false,
                 const size_t leafSize = 20,
                 TreeType* referenceTree = NULL,
                 const MetricType metric = MetricType());

  /**
   * Delete the NeighborSearch object. The tree is the only member we are
   * responsible for deleting.  The others will take care of themselves.
   */
  ~NeighborSearch();

  /**
   * Compute the nearest neighbors and store the output in the given matrices.
   * The matrices will be set to the size of n columns by k rows, where n is the
   * number of points in the query dataset and k is the number of neighbors
   * being searched for.
   *
   * @param k Number of neighbors to search for.
   * @param resultingNeighbors Matrix storing lists of neighbors for each query
   *     point.
   * @param distances Matrix storing distances of neighbors for each query
   *     point.
   */
  void Search(const size_t k,
              arma::Mat<size_t>& resultingNeighbors,
              arma::mat& distances);

 private:
  /**
   * Perform exhaustive computation between two leaves, comparing every node in
   * the leaf to the other leaf to find the furthest neighbor.  The
   * neighbors and distances matrices will be updated with the changed
   * information.
   *
   * @param queryNode Node in query tree.  This should be a leaf
   *     (bottom-level).
   * @param referenceNode Node in reference tree.  This should be a leaf
   *     (bottom-level).
   * @param neighbors List of neighbors for each point.
   * @param distances List of distances for each point.
   */
  void ComputeBaseCase(TreeType* queryNode,
                       TreeType* referenceNode,
                       arma::Mat<size_t>& neighbors,
                       arma::mat& distances);

  /**
   * Recurse down the trees, computing base case computations when the leaves
   * are reached.
   *
   * @param queryNode Node in query tree.
   * @param referenceNode Node in reference tree.
   * @param lowerBound The lower bound; if above this, we can prune.
   * @param neighbors List of neighbors for each point.
   * @param distances List of distances for each point.
   */
  void ComputeDualNeighborsRecursion(TreeType* queryNode,
                                     TreeType* referenceNode,
                                     const double lowerBound,
                                     arma::Mat<size_t>& neighbors,
                                     arma::mat& distances);

  /**
   * Perform a recursion only on the reference tree; the query point is given.
   * This method is similar to ComputeBaseCase().
   *
   * @param pointId Index of query point.
   * @param point The query point.
   * @param referenceNode Reference node.
   * @param bestDistSoFar Best distance to a node so far -- used for pruning.
   * @param neighbors List of neighbors for each point.
   * @param distances List of distances for each point.
   */
  template<typename VecType>
  void ComputeSingleNeighborsRecursion(const size_t pointId,
                                       const VecType& point,
                                       TreeType* referenceNode,
                                       double& bestDistSoFar,
                                       arma::Mat<size_t>& neighbors,
                                       arma::mat& distances);

  /**
   * Insert a point into the neighbors and distances matrices; this is a helper
   * function.
   *
   * @param queryIndex Index of point whose neighbors we are inserting into.
   * @param pos Position in list to insert into.
   * @param neighbor Index of reference point which is being inserted.
   * @param distance Distance from query point to reference point.
   * @param neighbors List of neighbors for each point.
   * @param distances List of distances for each point.
   */
  void InsertNeighbor(const size_t queryIndex,
                      const size_t pos,
                      const size_t neighbor,
                      const double distance,
                      arma::Mat<size_t>& neighbors,
                      arma::mat& distances);

  //! Copy of reference dataset (if we need it, because tree building modifies
  //! it).
  arma::mat referenceCopy;
  //! Copy of query dataset (if we need it, because tree building modifies it).
  arma::mat queryCopy;

  //! Reference dataset.
  const arma::mat& referenceSet;
  //! Query dataset (may not be given).
  const arma::mat& querySet;

  //! Indicates if O(n^2) naive search is being used.
  bool naive;
  //! Indicates if single-tree search is being used (opposed to dual-tree).
  bool singleMode;

  //! Pointer to the root of the reference tree.
  TreeType* referenceTree;
  //! Pointer to the root of the query tree (might not exist).
  TreeType* queryTree;

  //! Indicates if we should free the reference tree at deletion time.
  bool ownReferenceTree;
  //! Indicates if we should free the query tree at deletion time.
  bool ownQueryTree;

  //! Instantiation of kernel.
  MetricType metric;

  //! Permutations of reference points during tree building.
  std::vector<size_t> oldFromNewReferences;
  //! Permutations of query points during tree building.
  std::vector<size_t> oldFromNewQueries;

  //! Total number of pruned nodes during the neighbor search.
  size_t numberOfPrunes;
}; // class NeighborSearch

}; // namespace neighbor
}; // namespace mlpack

// Include implementation.
#include "neighbor_search_impl.hpp"

// Include convenience typedefs.
#include "typedef.hpp"

#endif