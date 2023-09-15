.. index:: pair: class; us::L2::graph_foc_wrapper
.. _doxid-classus_1_1_l2_1_1graph__foc__wrapper:
.. _cid-us::l2::graph_foc_wrapper:

template class us::L2::graph_foc_wrapper
========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_focusing.hpp"




.. _doxid-classus_1_1_l2_1_1graph__foc__wrapper_1a4dfdbd3db3f73caf3d72335137d1f76c:
.. _cid-us::l2::graph_foc_wrapper::p_para_const:
.. _doxid-classus_1_1_l2_1_1graph__foc__wrapper_1aad648d6368d9134cee55e92a18f29789:
.. _cid-us::l2::graph_foc_wrapper::p_para_pos:
.. _doxid-classus_1_1_l2_1_1graph__foc__wrapper_1a63c08151755ec89140fe67d57142985b:
.. _cid-us::l2::graph_foc_wrapper::dataout1:
.. _doxid-classus_1_1_l2_1_1graph__foc__wrapper_1a9cec54addbcc9109e91ef5c4480cad20:
.. _cid-us::l2::graph_foc_wrapper::k_foc:
.. _doxid-classus_1_1_l2_1_1graph__foc__wrapper_1af4131fd802e8ee1e1fb8dc3307f16b15:
.. _cid-us::l2::graph_foc_wrapper::graph_foc_wrapper:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int NUM_SAMPLE_t,
	    int NUM_SEG_t,
	    int VECDIM_foc_t,
	    int LEN32b_PARA_foc_t
	    >
	class graph_foc_wrapper: public graph

	// fields

	port <input> p_para_const
	port <input> p_para_pos
	port <output> dataout1
	kernel k_foc

