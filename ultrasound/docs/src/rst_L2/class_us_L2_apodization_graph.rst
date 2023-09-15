.. index:: pair: class; us::L2::apodization_graph
.. _doxid-classus_1_1_l2_1_1apodization__graph:
.. _cid-us::l2::apodization_graph:

template class us::L2::apodization_graph
========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "apodization.hpp"




.. _doxid-classus_1_1_l2_1_1apodization__graph_1a3de487495e01c4d52a0187a5b0bfd312:
.. _cid-us::l2::apodization_graph::image_points:
.. _doxid-classus_1_1_l2_1_1apodization__graph_1a083835226249050bbc9497855c00bc6e:
.. _cid-us::l2::apodization_graph::apodization_reference_i:
.. _doxid-classus_1_1_l2_1_1apodization__graph_1a4fcec06c38c223a8f5352d488e6008e0:
.. _cid-us::l2::apodization_graph::apo_distance_k:
.. _doxid-classus_1_1_l2_1_1apodization__graph_1a2bd674c95a677a9937f5f3019b7f87e2:
.. _cid-us::l2::apodization_graph::f_number:
.. _doxid-classus_1_1_l2_1_1apodization__graph_1a382d140ccb8193e32295543d09bc8afa:
.. _cid-us::l2::apodization_graph::apodization_output:
.. _doxid-classus_1_1_l2_1_1apodization__graph_1a1147ce1320b3f28c71cc7910cd140ace:
.. _cid-us::l2::apodization_graph::apodization_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class apodization_graph: public graph

	// fields

	adf::port <input> image_points
	adf::port <input> apodization_reference_i
	adf::port <input> apo_distance_k
	adf::port <input> F_number
	adf::port <output> apodization_output

