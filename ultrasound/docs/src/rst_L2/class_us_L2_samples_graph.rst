.. index:: pair: class; us::L2::samples_graph
.. _doxid-classus_1_1_l2_1_1samples__graph:
.. _cid-us::l2::samples_graph:

template class us::L2::samples_graph
====================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "samples.hpp"




.. _doxid-classus_1_1_l2_1_1samples__graph_1ab56153bb54c409f79038ad074fbad3ed:
.. _cid-us::l2::samples_graph::image_points_from_pl:
.. _doxid-classus_1_1_l2_1_1samples__graph_1a9b3724eb4cd1f4603f4d73004b212e02:
.. _cid-us::l2::samples_graph::delay_from_pl:
.. _doxid-classus_1_1_l2_1_1samples__graph_1a7e7280426b250cd3662607e503465669:
.. _cid-us::l2::samples_graph::xdc_def_positions:
.. _doxid-classus_1_1_l2_1_1samples__graph_1a1438c32ba1f52b031cb191deaa4aa9ba:
.. _cid-us::l2::samples_graph::sampling_frequency:
.. _doxid-classus_1_1_l2_1_1samples__graph_1a7c7dc44e7b9eb5540c3c52e08385c6fd:
.. _cid-us::l2::samples_graph::samples_to_pl:
.. _doxid-classus_1_1_l2_1_1samples__graph_1af97ae05f0e7bbb567aaab3d66e193c5f:
.. _cid-us::l2::samples_graph::samples_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class samples_graph: public graph

	// fields

	adf::port <input> image_points_from_PL
	adf::port <input> delay_from_PL
	adf::port <input> xdc_def_positions
	adf::port <input> sampling_frequency
	adf::port <output> samples_to_PL

