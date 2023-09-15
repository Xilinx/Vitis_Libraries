.. index:: pair: class; us::L2::focusing_sa_graph
.. _doxid-classus_1_1_l2_1_1focusing__sa__graph:
.. _cid-us::l2::focusing_sa_graph:

template class us::L2::focusing_sa_graph
========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "focusing_sa.hpp"




.. _doxid-classus_1_1_l2_1_1focusing__sa__graph_1a9a7810b0a97e01e186e5c0ba07b9bd9d:
.. _cid-us::l2::focusing_sa_graph::apo_ref_0:
.. _doxid-classus_1_1_l2_1_1focusing__sa__graph_1a2eed6b6525ef4950d3a0231b52170c5b:
.. _cid-us::l2::focusing_sa_graph::img_points_0:
.. _doxid-classus_1_1_l2_1_1focusing__sa__graph_1ac80b9d4b17eb895d913bb408ebab2ace:
.. _cid-us::l2::focusing_sa_graph::apo_ref_1:
.. _doxid-classus_1_1_l2_1_1focusing__sa__graph_1a1dc1992bb2f659d89f99bc463f2de95f:
.. _cid-us::l2::focusing_sa_graph::img_points_1:
.. _doxid-classus_1_1_l2_1_1focusing__sa__graph_1a7dd3c4d0ed855c5ddebf5c004412b6a2:
.. _cid-us::l2::focusing_sa_graph::focusing_output:
.. _doxid-classus_1_1_l2_1_1focusing__sa__graph_1a8840776696ef7587aa142f4d14333cf4:
.. _cid-us::l2::focusing_sa_graph::focusing_sa_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH_
	    >
	class focusing_sa_graph: public graph

	// fields

	adf::port <input> apo_ref_0
	adf::port <input> img_points_0
	adf::port <input> apo_ref_1
	adf::port <input> img_points_1
	adf::port <output> focusing_output

