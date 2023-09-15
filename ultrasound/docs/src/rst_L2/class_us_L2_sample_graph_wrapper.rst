.. index:: pair: class; us::L2::sample_graph_wrapper
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper:
.. _cid-us::l2::sample_graph_wrapper:

template class us::L2::sample_graph_wrapper
===========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_samples.hpp"




.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a574f53c446b1f6151cb8ce6db4e2ac7e:
.. _cid-us::l2::sample_graph_wrapper::para_const:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a3f14c1d6cd9761223fb3c7dc05fc278b:
.. _cid-us::l2::sample_graph_wrapper::para_rfdim:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1aa1103355cc835c91af0e67781fd2a81d:
.. _cid-us::l2::sample_graph_wrapper::para_elem:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a14f188d4f1102570379a3609d6e7af9e:
.. _cid-us::l2::sample_graph_wrapper::sample:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a2d671f7a725533696e819c06b74b75d3:
.. _cid-us::l2::sample_graph_wrapper::inside:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a48ca5e67aaec46fe8e9d97f775b553ac:
.. _cid-us::l2::sample_graph_wrapper::img_x:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a8011d435b139dac4b46d8aea70ab9db4:
.. _cid-us::l2::sample_graph_wrapper::img_z:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a1eeb913914c9e50ae4aed835980c7b21:
.. _cid-us::l2::sample_graph_wrapper::delay:
.. _doxid-classus_1_1_l2_1_1sample__graph__wrapper_1a726d6afbfbc03f1b096834c0d5b44f65:
.. _cid-us::l2::sample_graph_wrapper::sample_graph_wrapper:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int VECDIM_sample_t,
	    int LEN_IN_sample_t,
	    int LEN_OUT_sample_t,
	    int LEN32b_PARA_sample_t
	    >
	class sample_graph_wrapper: public graph

	// fields

	adf::port <adf::direction::in> para_const
	adf::port <adf::direction::in> para_rfdim
	adf::port <adf::direction::in> para_elem
	adf::port <output> sample
	adf::port <output> inside
	adf::port <input> img_x
	adf::port <input> img_z
	adf::port <input> delay

