.. index:: pair: class; us::L2::apodi_pre_graph
.. _doxid-classus_1_1_l2_1_1apodi__pre__graph:
.. _cid-us::l2::apodi_pre_graph:

template class us::L2::apodi_pre_graph
======================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_apodization.hpp"




.. _doxid-classus_1_1_l2_1_1apodi__pre__graph_1a9dd713e49f8f0848ff1942f5c98a53fe:
.. _cid-us::l2::apodi_pre_graph::out:
.. _doxid-classus_1_1_l2_1_1apodi__pre__graph_1aa166bf98368eac98c6ba56ee278187c3:
.. _cid-us::l2::apodi_pre_graph::img_x_in:
.. _doxid-classus_1_1_l2_1_1apodi__pre__graph_1a96fa64f564a938aabe58d91f27278a71:
.. _cid-us::l2::apodi_pre_graph::img_z_in:
.. _doxid-classus_1_1_l2_1_1apodi__pre__graph_1ad7189805fe0593e0984cc4b7b45451bb:
.. _cid-us::l2::apodi_pre_graph::para_apodi_const:
.. _doxid-classus_1_1_l2_1_1apodi__pre__graph_1a997a9aac1010e427d90f6bcdd66bab3c:
.. _cid-us::l2::apodi_pre_graph::apodi_pre_graph:
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
	    int VECDIM,
	    int APODI_PRE_LEN32b_PARA
	    >
	class apodi_pre_graph: public graph

	// fields

	port <output> out
	port <input> img_x_in
	port <input> img_z_in
	port <input> para_apodi_const

