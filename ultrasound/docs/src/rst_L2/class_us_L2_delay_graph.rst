.. index:: pair: class; us::L2::delay_graph
.. _doxid-classus_1_1_l2_1_1delay__graph:
.. _cid-us::l2::delay_graph:

template class us::L2::delay_graph
==================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "delay.hpp"




.. _doxid-classus_1_1_l2_1_1delay__graph_1ac667eea93cfa1da51767eb4b02f47f01:
.. _cid-us::l2::delay_graph::image_points_from_pl:
.. _doxid-classus_1_1_l2_1_1delay__graph_1a8802682b5e06b2e6a8e6c14285635333:
.. _cid-us::l2::delay_graph::image_points_from_pl_:
.. _doxid-classus_1_1_l2_1_1delay__graph_1a656c0ac7c8522abc9e4d10915931fd43:
.. _cid-us::l2::delay_graph::tx_def_ref_point:
.. _doxid-classus_1_1_l2_1_1delay__graph_1af0eb7cfb7f18d028f23792fa378a9c55:
.. _cid-us::l2::delay_graph::tx_def_delay_distance:
.. _doxid-classus_1_1_l2_1_1delay__graph_1a6acb7a69094f88ec7598ecadac753900:
.. _cid-us::l2::delay_graph::tx_def_delay_distance2:
.. _doxid-classus_1_1_l2_1_1delay__graph_1a61ad6190167bc1a968ede538573a245d:
.. _cid-us::l2::delay_graph::tx_def_focal_point:
.. _doxid-classus_1_1_l2_1_1delay__graph_1a694bcb369c75c705cc5513434241e46a:
.. _cid-us::l2::delay_graph::t_start:
.. _doxid-classus_1_1_l2_1_1delay__graph_1ac1a969ac98e61c747499361e1a9989c0:
.. _cid-us::l2::delay_graph::delay_to_pl:
.. _doxid-classus_1_1_l2_1_1delay__graph_1ae6163504b98564c4b9bdfc99b6322dde:
.. _cid-us::l2::delay_graph::delay_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class delay_graph: public graph

	// fields

	adf::port <input> image_points_from_PL
	adf::port <input> image_points_from_PL_
	adf::port <input> tx_def_ref_point
	adf::port <input> tx_def_delay_distance
	adf::port <input> tx_def_delay_distance2
	adf::port <input> tx_def_focal_point
	adf::port <input> t_start
	adf::port <output> delay_to_PL

