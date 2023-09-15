.. index:: pair: class; us::L2::apodization_sa_graph
.. _doxid-classus_1_1_l2_1_1apodization__sa__graph:
.. _cid-us::l2::apodization_sa_graph:

template class us::L2::apodization_sa_graph
===========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "apodization_sa.hpp"




.. _doxid-classus_1_1_l2_1_1apodization__sa__graph_1ab0b1ffb84d95156a8b8b77ffc52b48af:
.. _cid-us::l2::apodization_sa_graph::image_points:
.. _doxid-classus_1_1_l2_1_1apodization__sa__graph_1a908e37c68ac758fe8cdbe62a9d961919:
.. _cid-us::l2::apodization_sa_graph::apodization_reference:
.. _doxid-classus_1_1_l2_1_1apodization__sa__graph_1a124b81199c9ecc8087fb67e5c28dc619:
.. _cid-us::l2::apodization_sa_graph::apo_distance_k:
.. _doxid-classus_1_1_l2_1_1apodization__sa__graph_1affa83f7dba23fa89a3c411c3dd5777de:
.. _cid-us::l2::apodization_sa_graph::f_number:
.. _doxid-classus_1_1_l2_1_1apodization__sa__graph_1a5ea8daeff6145deebc16decaa2a5a5e1:
.. _cid-us::l2::apodization_sa_graph::apodization_output:
.. _doxid-classus_1_1_l2_1_1apodization__sa__graph_1aef900e956608839eda67fc8a83d78651:
.. _cid-us::l2::apodization_sa_graph::apodization_sa_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class apodization_sa_graph: public graph

	// fields

	adf::port <input> image_points
	adf::port <input> apodization_reference
	adf::port <input> apo_distance_k
	adf::port <input> F_number
	adf::port <output> apodization_output

