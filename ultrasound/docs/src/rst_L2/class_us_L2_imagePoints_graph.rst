.. index:: pair: class; us::L2::imagePoints_graph
.. _doxid-classus_1_1_l2_1_1image_points__graph:
.. _cid-us::l2::imagepoints_graph:

template class us::L2::imagePoints_graph
========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "imagePoints.hpp"




.. _doxid-classus_1_1_l2_1_1image_points__graph_1a4d23c9ca8c9112e4ecb4005da14bb3cb:
.. _cid-us::l2::imagepoints_graph::start_positions:
.. _doxid-classus_1_1_l2_1_1image_points__graph_1ad0d6a52a4bca36c483e438c5fdc16da8:
.. _cid-us::l2::imagepoints_graph::directions:
.. _doxid-classus_1_1_l2_1_1image_points__graph_1a8b7072ebdf329631df14dd6480e05d3d:
.. _cid-us::l2::imagepoints_graph::samples_arange:
.. _doxid-classus_1_1_l2_1_1image_points__graph_1a2347f3ab152770418dfea9fcaaebba88:
.. _cid-us::l2::imagepoints_graph::image_points:
.. _doxid-classus_1_1_l2_1_1image_points__graph_1a9b2c750cff1f423c674a55527003a8a4:
.. _cid-us::l2::imagepoints_graph::imagepoints_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH
	    >
	class imagePoints_graph: public graph

	// fields

	adf::port <input> start_positions
	adf::port <input> directions
	adf::port <input> samples_arange
	adf::port <output> image_points

