.. index:: pair: class; us::L2::focusing_graph
.. _doxid-classus_1_1_l2_1_1focusing__graph:
.. _cid-us::l2::focusing_graph:

template class us::L2::focusing_graph
=====================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "focusing.hpp"




.. _doxid-classus_1_1_l2_1_1focusing__graph_1abb9990fb56fdc529c4c1eeafbfd7d675:
.. _cid-us::l2::focusing_graph::apo_ref_0:
.. _doxid-classus_1_1_l2_1_1focusing__graph_1aad115da1cd9297d4739ac3bf18594e17:
.. _cid-us::l2::focusing_graph::xdc_def_0:
.. _doxid-classus_1_1_l2_1_1focusing__graph_1ac0a1bfad75ab03ee8752035742a427f9:
.. _cid-us::l2::focusing_graph::apo_ref_1:
.. _doxid-classus_1_1_l2_1_1focusing__graph_1a9bf32d94f2cb550a33010744bc72d286:
.. _cid-us::l2::focusing_graph::xdc_def_1:
.. _doxid-classus_1_1_l2_1_1focusing__graph_1ae2b9d258522ee8448fde7f0448489351:
.. _cid-us::l2::focusing_graph::focusing_output:
.. _doxid-classus_1_1_l2_1_1focusing__graph_1a37f285bead8fb1bc6880a585e8b2253a:
.. _cid-us::l2::focusing_graph::focusing_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH
	    >
	class focusing_graph: public graph

	// fields

	adf::port <input> apo_ref_0
	adf::port <input> xdc_def_0
	adf::port <input> apo_ref_1
	adf::port <input> xdc_def_1
	adf::port <output> focusing_output

