.. index:: pair: namespace; sr_asym
.. _doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1sr__asym:
.. _cid-xf::dsp::aie::fir::sr_asym:

namespace sr_asym
=================

.. toctree::
	:hidden:

	class_xf_dsp_aie_fir_sr_asym_conditioanl_rtp_graph.rst
	class_xf_dsp_aie_fir_sr_asym_conditioanl_rtp_graph-2.rst
	class_xf_dsp_aie_fir_sr_asym_conditional_in_graph.rst
	class_xf_dsp_aie_fir_sr_asym_conditional_in_graph-2.rst
	class_xf_dsp_aie_fir_sr_asym_conditional_out_graph.rst
	class_xf_dsp_aie_fir_sr_asym_conditional_out_graph-2.rst
	class_xf_dsp_aie_fir_sr_asym_fir_sr_asym_base_graph.rst
	class_xf_dsp_aie_fir_sr_asym_fir_sr_asym_graph.rst





.. ref-code-block:: cpp
	:class: overview-code-block

	// classes

	template <
	    unsigned int TP_USE_RTP = 1,
	    unsigned int TP_PORT_POS = 1
	    >
	class :ref:`conditioanl_rtp_graph<doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1conditioanl__rtp__graph>` 

	template <unsigned int TP_PORT_POS>
	class :ref:`conditioanl_rtp_graph <0, TP_PORT_POS><doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1conditioanl__rtp__graph_3_010_00_01_t_p___p_o_r_t___p_o_s_01_4>` 

	template <
	    unsigned int TP_INPUT_WINDOW_BYTESIZE,
	    unsigned int TP_MARGIN,
	    unsigned int TP_CASC_LEN,
	    unsigned int TP_API,
	    unsigned int TP_PORT_POS
	    >
	class :ref:`conditional_in_graph <TP_INPUT_WINDOW_BYTESIZE, TP_MARGIN, TP_CASC_LEN, DUAL_IP_SINGLE, TP_API, TP_PORT_POS><doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1conditional__in__graph_3_01_t_p___i_n_p_u_t___w_i_n_6ea3d8b2cd686da3e92552dffb6302c1>` 

	template <
	    unsigned int TP_INPUT_WINDOW_BYTESIZE,
	    unsigned int TP_MARGIN,
	    unsigned int TP_CASC_LEN = 1,
	    unsigned int TP_DUAL_IP = DUAL_IP_DUAL,
	    unsigned int TP_API = 0,
	    unsigned int TP_PORT_POS = 1
	    >
	class :ref:`conditional_in_graph<doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1conditional__in__graph>` 

	template <
	    unsigned int TP_OUTPUT_WINDOW_BYTESIZE,
	    unsigned int TP_CASC_LEN,
	    unsigned int TP_API,
	    unsigned int TP_PORT_POS
	    >
	class :ref:`conditional_out_graph <TP_OUTPUT_WINDOW_BYTESIZE, TP_CASC_LEN, 1, TP_API, TP_PORT_POS><doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1conditional__out__graph_3_01_t_p___o_u_t_p_u_t___w_ibb33a09a162fd879a5d2f5894d734db4>` 

	template <
	    unsigned int TP_OUTPUT_WINDOW_BYTESIZE,
	    unsigned int TP_CASC_LEN = 1,
	    unsigned int TP_NUM_OUTPUTS = 2,
	    unsigned int TP_API = 0,
	    unsigned int TP_PORT_POS = 1
	    >
	class :ref:`conditional_out_graph<doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1conditional__out__graph>` 

	template <
	    typename TT_DATA,
	    typename TT_COEFF,
	    unsigned int TP_FIR_LEN,
	    unsigned int TP_SHIFT,
	    unsigned int TP_RND,
	    unsigned int TP_INPUT_WINDOW_VSIZE,
	    unsigned int TP_CASC_LEN = 1,
	    unsigned int TP_USE_COEFF_RELOAD = 0,
	    unsigned int TP_NUM_OUTPUTS = 1,
	    unsigned int TP_DUAL_IP = 1,
	    unsigned int TP_API = 0
	    >
	class :ref:`fir_sr_asym_base_graph<doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1fir__sr__asym__base__graph>` 

	template <
	    typename TT_DATA,
	    typename TT_COEFF,
	    unsigned int TP_FIR_LEN,
	    unsigned int TP_SHIFT,
	    unsigned int TP_RND,
	    unsigned int TP_INPUT_WINDOW_VSIZE,
	    unsigned int TP_CASC_LEN = 1,
	    unsigned int TP_USE_COEFF_RELOAD = 0,
	    unsigned int TP_NUM_OUTPUTS = 1,
	    unsigned int TP_DUAL_IP = 0,
	    unsigned int TP_API = 0
	    >
	class :ref:`fir_sr_asym_graph<doxid-classxf_1_1dsp_1_1aie_1_1fir_1_1sr__asym_1_1fir__sr__asym__graph>` 

