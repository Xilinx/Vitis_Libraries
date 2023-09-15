.. index:: pair: class; us::L2::interpolation_graph
.. _doxid-classus_1_1_l2_1_1interpolation__graph:
.. _cid-us::l2::interpolation_graph:

template class us::L2::interpolation_graph
==========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_interpolation.hpp"




.. _doxid-classus_1_1_l2_1_1interpolation__graph_1a21ea9de4076e6736da8f964dff88ec07:
.. _cid-us::l2::interpolation_graph::out:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1acc3b21d5d491a5ace11129037f1ad2e8:
.. _cid-us::l2::interpolation_graph::p_sample_in:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1a4bc65a43a56d2b7b0253346056e37d65:
.. _cid-us::l2::interpolation_graph::p_inside_in:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1a923c83afa75217c44318ffd02278ccc9:
.. _cid-us::l2::interpolation_graph::p_rfdata_in:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1a86a9bbb635e01bd5e35e198a5ce29532:
.. _cid-us::l2::interpolation_graph::para_interp_const_0:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1a301cfeca115faad0fe5f6b3b54ca8d16:
.. _cid-us::l2::interpolation_graph::para_interp_const_1:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1aa0cae06c86d7ee361e053cd584d6b5ab:
.. _cid-us::l2::interpolation_graph::para_interp_const_2:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1ad037e4dd7bd891120119c5b6227decf0:
.. _cid-us::l2::interpolation_graph::para_interp_const_3:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1aacdac85b18bc97a6d897cdd11f6204b0:
.. _cid-us::l2::interpolation_graph::para_local:
.. _doxid-classus_1_1_l2_1_1interpolation__graph_1a5ff3f12fa57c6590f1672e3efea68de8:
.. _cid-us::l2::interpolation_graph::interpolation_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T,
	    int NUM_LINE,
	    int NUM_ELEMENT,
	    int NUM_SAMPLE,
	    int NUM_SEG,
	    int LEN_OUT,
	    int LEN_IN,
	    int LEN_RF_IN,
	    int VECDIM,
	    int APODI_PRE_LEN32b_PARA
	    >
	class interpolation_graph: public graph

	// fields

	port <output> out
	port <input> p_sample_in
	port <input> p_inside_in
	port <input> p_rfdata_in
	port <input> para_interp_const_0
	port <input> para_interp_const_1
	port <input> para_interp_const_2
	port <input> para_interp_const_3
	port <input> para_local

