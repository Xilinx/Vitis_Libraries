.. index:: pair: class; us::L2::interpolation_graph_scaler_shell
.. _doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell:
.. _cid-us::l2::interpolation_graph_scaler_shell:

template class us::L2::interpolation_graph_scaler_shell
=======================================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "graph_interpolation.hpp"




.. _doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell_1a75dfa529a31e974260f83311ba466c2c:
.. _cid-us::l2::interpolation_graph_scaler_shell::out:
.. _doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell_1a00f5f8da1881d59a59c71cf702a55011:
.. _cid-us::l2::interpolation_graph_scaler_shell::p_sample_in:
.. _doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell_1a0b09f764eaa811eb58ac54b1ea147e42:
.. _cid-us::l2::interpolation_graph_scaler_shell::p_inside_in:
.. _doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell_1ab0238da1040eeb8c21fac5bc61860ad0:
.. _cid-us::l2::interpolation_graph_scaler_shell::p_rfdata_in:
.. _doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell_1a4f528f3e9b0fc62508e783a382f5c315:
.. _cid-us::l2::interpolation_graph_scaler_shell::para_interp_const:
.. _doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell_1abe401d456f6ccdd268628846321d1013:
.. _cid-us::l2::interpolation_graph_scaler_shell::interpolation_graph_scaler_shell:
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
	class interpolation_graph_scaler_shell: public graph

	// fields

	port <output> out
	port <input> p_sample_in
	port <input> p_inside_in
	port <input> p_rfdata_in
	port <input> para_interp_const

