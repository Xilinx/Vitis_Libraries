.. index:: pair: class; us::L2::delay_pw_graph
.. _doxid-classus_1_1_l2_1_1delay__pw__graph:
.. _cid-us::l2::delay_pw_graph:

template class us::L2::delay_pw_graph
=====================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "delay_pw.hpp"




.. _doxid-classus_1_1_l2_1_1delay__pw__graph_1a7335b75c5779cddc4df4e1d9f4abd126:
.. _cid-us::l2::delay_pw_graph::image_points_from_pl:
.. _doxid-classus_1_1_l2_1_1delay__pw__graph_1a7a371f1cdda15246066fc0a7dc3bca5e:
.. _cid-us::l2::delay_pw_graph::tx_def_reference_point:
.. _doxid-classus_1_1_l2_1_1delay__pw__graph_1a70eb439a1b4eef7b26541fe1f9934b2f:
.. _cid-us::l2::delay_pw_graph::t_start:
.. _doxid-classus_1_1_l2_1_1delay__pw__graph_1a26deb904efd63c8726b2f3f487f29bf7:
.. _cid-us::l2::delay_pw_graph::delay_to_pl:
.. _doxid-classus_1_1_l2_1_1delay__pw__graph_1a800672cecdae19913a62928016e0a7c8:
.. _cid-us::l2::delay_pw_graph::delay_pw_graph:
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
	class delay_pw_graph: public graph

	// fields

	adf::port <input> image_points_from_PL
	adf::port <input> tx_def_reference_point
	adf::port <input> t_start
	adf::port <output> delay_to_PL

