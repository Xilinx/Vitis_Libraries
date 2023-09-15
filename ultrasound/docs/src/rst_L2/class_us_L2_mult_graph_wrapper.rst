.. index:: pair: class; us::L2::mult_graph_wrapper
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper:
.. _cid-us::l2::mult_graph_wrapper:

template class us::L2::mult_graph_wrapper
=========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_mult.hpp"




.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1adb28f0ed6133b6008963f20a3d3466b8:
.. _cid-us::l2::mult_graph_wrapper::para_const_pre:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a4cd312ffa83e246e3953cb43378327ed:
.. _cid-us::l2::mult_graph_wrapper::para_const_0:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a5639c8b0a2c6d81975b2caf945ad9f19:
.. _cid-us::l2::mult_graph_wrapper::para_const_1:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a3002d6525e719691cff9b6d73b4c5766:
.. _cid-us::l2::mult_graph_wrapper::para_const_2:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a2c3dad54d7ea0b1d4d4c68cc15187091:
.. _cid-us::l2::mult_graph_wrapper::para_const_3:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1abe7a99e36cd732684961361213087f5e:
.. _cid-us::l2::mult_graph_wrapper::para_local_0_0:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a8f61fc4d4638a4ed145cbe82d4efef88:
.. _cid-us::l2::mult_graph_wrapper::para_local_0_1:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a0910bf3356fac057948fc11c8345d323:
.. _cid-us::l2::mult_graph_wrapper::para_local_0_2:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a7d657b20f9b037b44713e967a4a98f7f:
.. _cid-us::l2::mult_graph_wrapper::para_local_0_3:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a2b988e5f705ebd8394a3968d0d1ec076:
.. _cid-us::l2::mult_graph_wrapper::para_local_1_0:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a54c60d56d11a61c679ca139c036e149d:
.. _cid-us::l2::mult_graph_wrapper::para_local_1_1:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a64be657e8ac67f7280dcf6b12427605a:
.. _cid-us::l2::mult_graph_wrapper::para_local_1_2:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a5b5ef4d0d870885f85dcccb4d8f81d12:
.. _cid-us::l2::mult_graph_wrapper::para_local_1_3:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a57fd66a10840bc686b90a3eac53a6bb9:
.. _cid-us::l2::mult_graph_wrapper::interp:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a1728edf3fd29029b4609041c4553dd7c:
.. _cid-us::l2::mult_graph_wrapper::apod:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a082a7dc5966adf6e52ee7e243c1f036a:
.. _cid-us::l2::mult_graph_wrapper::mult_0:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1ae63099d670c0940bc863d39ddcba2a1f:
.. _cid-us::l2::mult_graph_wrapper::mult_1:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a44dadfdafaeab58eee965805c95efe84:
.. _cid-us::l2::mult_graph_wrapper::mult_2:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1a827ec2f2f0252de56368877a25d93b2c:
.. _cid-us::l2::mult_graph_wrapper::mult_3:
.. _doxid-classus_1_1_l2_1_1mult__graph__wrapper_1ad8af69ad7dc351dc5625e5673d31a22a:
.. _cid-us::l2::mult_graph_wrapper::mult_graph_wrapper:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int NUM_SAMPLE_t,
	    int NUM_SEG_t,
	    int NUM_DEP_SEG_t,
	    int VECDIM_mult_t,
	    int LEN_IN_mult_t,
	    int LEN_OUT_mult_t,
	    int LEN32b_PARA_mult_t,
	    int MULT_ID_t
	    >
	class mult_graph_wrapper: public graph

	// fields

	port <direction::in> para_const_pre
	port <direction::in> para_const_0
	port <direction::in> para_const_1
	port <direction::in> para_const_2
	port <direction::in> para_const_3
	port <direction::in> para_local_0_0
	port <direction::in> para_local_0_1
	port <direction::in> para_local_0_2
	port <direction::in> para_local_0_3
	port <direction::in> para_local_1_0
	port <direction::in> para_local_1_1
	port <direction::in> para_local_1_2
	port <direction::in> para_local_1_3
	port <input> interp
	port <input> apod
	port <output> mult_0
	port <output> mult_1
	port <output> mult_2
	port <output> mult_3

