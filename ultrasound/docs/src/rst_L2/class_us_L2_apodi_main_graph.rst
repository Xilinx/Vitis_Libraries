.. index:: pair: class; us::L2::apodi_main_graph
.. _doxid-classus_1_1_l2_1_1apodi__main__graph:
.. _cid-us::l2::apodi_main_graph:

template class us::L2::apodi_main_graph
=======================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_apodization.hpp"




.. _doxid-classus_1_1_l2_1_1apodi__main__graph_1a136cc2270de4200fda10a6d7cb9f8fec:
.. _cid-us::l2::apodi_main_graph::out:
.. _doxid-classus_1_1_l2_1_1apodi__main__graph_1a366436c05c9cd33d89fc59cd9e8958b2:
.. _cid-us::l2::apodi_main_graph::p_focal:
.. _doxid-classus_1_1_l2_1_1apodi__main__graph_1aa22fec6df105fde1190133ed28d79032:
.. _cid-us::l2::apodi_main_graph::p_invd:
.. _doxid-classus_1_1_l2_1_1apodi__main__graph_1a65c8292436d072b6b0fa38db741ae527:
.. _cid-us::l2::apodi_main_graph::para_amain_const:
.. _doxid-classus_1_1_l2_1_1apodi__main__graph_1a4761c252f5156dd65507ce79c87b64fe:
.. _cid-us::l2::apodi_main_graph::apodi_main_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename T,
	    int NUM_LINE,
	    int NUM_ELEMENT,
	    int NUM_SAMPLE,
	    int NUM_SEG,
	    int LEN_OUT,
	    int LEN_IN_F,
	    int LEN_IN_D,
	    int VECDIM,
	    int APODI_PRE_LEN32b_PARA
	    >
	class apodi_main_graph: public graph

	// fields

	port <output> out
	port <input> p_focal
	port <input> p_invD
	port <input> para_amain_const

