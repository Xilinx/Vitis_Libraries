.. index:: pair: class; us::L2::graph_img_wrapper
.. _doxid-classus_1_1_l2_1_1graph__img__wrapper:
.. _cid-us::l2::graph_img_wrapper:

template class us::L2::graph_img_wrapper
========================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_imagepoints.hpp"




.. _doxid-classus_1_1_l2_1_1graph__img__wrapper_1a4af21d2cdb2ac8420371afe07e97fbcc:
.. _cid-us::l2::graph_img_wrapper::p_para_const:
.. _doxid-classus_1_1_l2_1_1graph__img__wrapper_1a13d417c00ad6a6ff9869bd1300eef9ac:
.. _cid-us::l2::graph_img_wrapper::p_para_start:
.. _doxid-classus_1_1_l2_1_1graph__img__wrapper_1a31bfb3c8fc05a8a02d888bef11257a90:
.. _cid-us::l2::graph_img_wrapper::dataout1:
.. _doxid-classus_1_1_l2_1_1graph__img__wrapper_1a0f0f8cadbffba8a4378f1dd37961fdc0:
.. _cid-us::l2::graph_img_wrapper::k_img:
.. _doxid-classus_1_1_l2_1_1graph__img__wrapper_1a6abe4780c39ca5420bad480eefbc7d74:
.. _cid-us::l2::graph_img_wrapper::graph_img_wrapper:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int NUM_SAMPLE_t,
	    int NUM_SEG_t,
	    int VECDIM_img_t,
	    int LEN_OUT_img_t,
	    int LEN32b_PARA_img_t
	    >
	class graph_img_wrapper: public graph

	// fields

	port <input> p_para_const
	port <input> p_para_start
	port <output> dataout1
	kernel k_img

