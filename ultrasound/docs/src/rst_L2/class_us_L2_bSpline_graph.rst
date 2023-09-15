.. index:: pair: class; us::L2::bSpline_graph
.. _doxid-classus_1_1_l2_1_1b_spline__graph:
.. _cid-us::l2::bspline_graph:

template class us::L2::bSpline_graph
====================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "bSpline.hpp"




.. _doxid-classus_1_1_l2_1_1b_spline__graph_1aa2f808f8cd6b2c13932d69e4c56bca1b:
.. _cid-us::l2::bspline_graph::p1:
.. _doxid-classus_1_1_l2_1_1b_spline__graph_1a78ee94da22913fdedbe21fc26db2fe98:
.. _cid-us::l2::bspline_graph::p2:
.. _doxid-classus_1_1_l2_1_1b_spline__graph_1a33a655b4b079cdadf76e692b261514ad:
.. _cid-us::l2::bspline_graph::p3:
.. _doxid-classus_1_1_l2_1_1b_spline__graph_1a3f3c67acfb6986d837c4f8b7beab1d51:
.. _cid-us::l2::bspline_graph::p4:
.. _doxid-classus_1_1_l2_1_1b_spline__graph_1a9ee3ab615c8aa9d77f1747ae36b77b5b:
.. _cid-us::l2::bspline_graph::p5:
.. _doxid-classus_1_1_l2_1_1b_spline__graph_1afe0249cbc94159cbbab6158c43652026:
.. _cid-us::l2::bspline_graph::p6:
.. _doxid-classus_1_1_l2_1_1b_spline__graph_1a0b159baae88163647630f10981e61888:
.. _cid-us::l2::bspline_graph::c:
.. _doxid-classus_1_1_l2_1_1b_spline__graph_1ab8f0d0e78aa556e720b39165b185ea9c:
.. _cid-us::l2::bspline_graph::bspline_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class bSpline_graph: public graph

	// fields

	adf::port <input> P1
	adf::port <input> P2
	adf::port <input> P3
	adf::port <input> P4
	adf::port <input> P5
	adf::port <input> P6
	adf::port <output> C

