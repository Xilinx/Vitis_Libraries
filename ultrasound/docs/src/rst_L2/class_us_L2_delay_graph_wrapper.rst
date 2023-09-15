.. index:: pair: class; us::L2::delay_graph_wrapper
.. _doxid-classus_1_1_l2_1_1delay__graph__wrapper:
.. _cid-us::l2::delay_graph_wrapper:

template class us::L2::delay_graph_wrapper
==========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_delay.hpp"




.. _doxid-classus_1_1_l2_1_1delay__graph__wrapper_1a06f5d98b4addbfae6b00d6df33f1ea8c:
.. _cid-us::l2::delay_graph_wrapper::para_const:
.. _doxid-classus_1_1_l2_1_1delay__graph__wrapper_1ae4801c0d49c067893b832b8cf766d9a0:
.. _cid-us::l2::delay_graph_wrapper::para_t_start:
.. _doxid-classus_1_1_l2_1_1delay__graph__wrapper_1af95b89b122097b2f485e608ec87cfe01:
.. _cid-us::l2::delay_graph_wrapper::img_x:
.. _doxid-classus_1_1_l2_1_1delay__graph__wrapper_1aadcfb5b1b7dc77e1c4032380b4054382:
.. _cid-us::l2::delay_graph_wrapper::img_z:
.. _doxid-classus_1_1_l2_1_1delay__graph__wrapper_1afe0bb08276b072a474feebf980566529:
.. _cid-us::l2::delay_graph_wrapper::delay:
.. _doxid-classus_1_1_l2_1_1delay__graph__wrapper_1a34e35dd12da84664d2ce335812ce6f0e:
.. _cid-us::l2::delay_graph_wrapper::delay_graph_wrapper:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    class T,
	    int NUM_LINE_t,
	    int VECDIM_delay_t,
	    int LEN_IN_delay_t,
	    int LEN_OUT_delay_t,
	    int LEN32b_PARA_delay_t
	    >
	class delay_graph_wrapper: public graph

	// fields

	adf::port <adf::direction::in> para_const
	adf::port <adf::direction::in> para_t_start
	adf::port <input> img_x
	adf::port <input> img_z
	adf::port <output> delay

