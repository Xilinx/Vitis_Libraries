.. index:: pair: class; xf::dsp::aie::blas::matrix_mult::matrix_mult_graph <TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_SHIFT, TP_RND, TP_DIM_A_LEADING, TP_DIM_B_LEADING, TP_DIM_OUT_LEADING, TP_ADD_TILING_A, TP_ADD_TILING_B, TP_ADD_DETILING_OUT, TP_INPUT_WINDOW_VSIZE_A, TP_INPUT_WINDOW_VSIZE_B, 1>
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph:

template class xf::dsp::aie::blas::matrix_mult::matrix_mult_graph <TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_SHIFT, TP_RND, TP_DIM_A_LEADING, TP_DIM_B_LEADING, TP_DIM_OUT_LEADING, TP_ADD_TILING_A, TP_ADD_TILING_B, TP_ADD_DETILING_OUT, TP_INPUT_WINDOW_VSIZE_A, TP_INPUT_WINDOW_VSIZE_B, 1>
=============================================================================================================================================================================================================================================================================================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "matrix_mult_graph.hpp"




.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1aaffd982fc8d5f6ed27a041a6ce7ae711:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::matmult:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1aa927b040f2bfa6de008112b4d2639982:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::tilerclassa:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a3b3d277afd5bdbb99b4f11ab334a0a65:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::tilerclassb:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1ada5c62864a38c55d6da5c921083c3bfb:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::detilerclassout:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a503dfc1b595e43c2e94455e4c90fe093:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::tileaconditional:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a69ceb3357b80a4cee2e2cb66354f0917:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::tilebconditional:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a991138250525b2a9ef2546487b41900f:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::detileoutconditional:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1ad22f852dce1724b6f7407dbda47dea4b:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::ina:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a2bd31dd7e78445d0b18b057871b79e36:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::inb:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a41e7c04251d709fef4d6638b337d007c:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::out:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a7920ff5370a23455edd39a911ab84eea:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::m_matmult:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1aacfdcbee10d4ac1b73fad3d5d8c2b6e3:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::untiler:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1aabe8b93bec7aa38755bff5a152462021:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::tilera:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a83672091dd6f2b8627f3839028f77db6:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::tilerb:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a883b6a41b94f1b9845ff53cc721ce4b0:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::tilingscheme:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1afa8258e961c1655c30aa5385183abd2e:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::getkernels:
.. _doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a8179c852ab155184ee62609b8e6c5b83:
.. _cid-xf::dsp::aie::blas::matrix_mult::matrix_mult_graph::matrix_mult_graph:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    typename TT_DATA_A,
	    typename TT_DATA_B,
	    unsigned int TP_DIM_A,
	    unsigned int TP_DIM_AB,
	    unsigned int TP_DIM_B,
	    unsigned int TP_SHIFT,
	    unsigned int TP_RND,
	    unsigned int TP_DIM_A_LEADING,
	    unsigned int TP_DIM_B_LEADING,
	    unsigned int TP_DIM_OUT_LEADING,
	    unsigned int TP_ADD_TILING_A,
	    unsigned int TP_ADD_TILING_B,
	    unsigned int TP_ADD_DETILING_OUT,
	    unsigned int TP_INPUT_WINDOW_VSIZE_A,
	    unsigned int TP_INPUT_WINDOW_VSIZE_B
	    >
	class matrix_mult_graph <TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_SHIFT, TP_RND, TP_DIM_A_LEADING, TP_DIM_B_LEADING, TP_DIM_OUT_LEADING, TP_ADD_TILING_A, TP_ADD_TILING_B, TP_ADD_DETILING_OUT, TP_INPUT_WINDOW_VSIZE_A, TP_INPUT_WINDOW_VSIZE_B, 1>: public graph

	// typedefs

	typedef matrix_mult <TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_SHIFT, TP_RND, TP_DIM_A_LEADING, TP_DIM_B_LEADING, TP_DIM_OUT_LEADING, TP_INPUT_WINDOW_VSIZE_A, TP_INPUT_WINDOW_VSIZE_B> MatMult
	typedef tilerKernelClass <tilingScheme.Atile, tilingScheme.ABtile, TP_INPUT_WINDOW_VSIZE_A/TP_DIM_AB, TP_DIM_AB, TP_DIM_A_LEADING, TT_DATA_A> TilerClassA
	typedef tilerKernelClass <tilingScheme.ABtile, tilingScheme.Btile, TP_DIM_AB, TP_INPUT_WINDOW_VSIZE_B/TP_DIM_AB, TP_DIM_B_LEADING, TT_DATA_B> TilerClassB
	typedef untilerKernelClass <tilingScheme.Atile, tilingScheme.Btile, TP_INPUT_WINDOW_VSIZE_A/TP_DIM_AB, TP_INPUT_WINDOW_VSIZE_B/TP_DIM_AB, TP_DIM_OUT_LEADING, GET_TT_OUT (TT_DATA_A, TT_DATA_B)> DetilerClassOut
	typedef :ref:`ConditionalWidget<doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1_conditional_widget>` <TP_ADD_TILING_A, TP_INPUT_WINDOW_VSIZE_A*sizeof (TT_DATA_A), :ref:`TilerClassA<doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1aa927b040f2bfa6de008112b4d2639982>`> TileAConditional
	typedef :ref:`ConditionalWidget<doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1_conditional_widget>` <TP_ADD_TILING_B, TP_INPUT_WINDOW_VSIZE_B*sizeof (TT_DATA_B), :ref:`TilerClassB<doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1a3b3d277afd5bdbb99b4f11ab334a0a65>`> TileBConditional
	typedef :ref:`ConditionalWidget<doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1_conditional_widget>` <TP_ADD_DETILING_OUT, TP_INPUT_WINDOW_VSIZE_A/TP_DIM_AB*TP_INPUT_WINDOW_VSIZE_B/TP_DIM_AB*sizeof (GET_TT_OUT (TT_DATA_A, TT_DATA_B)), :ref:`DetilerClassOut<doxid-classxf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult_1_1matrix__mult__graph_3_01_t_t___d_a_t_a___a_00_0fce35ccc745691ffd32ca9c43d8438dc_1ada5c62864a38c55d6da5c921083c3bfb>`> DetileOutConditional

	// fields

	port <input> inA[1]
	port <input> inB[1]
	port <output> out
	kernel m_matMult
	kernel untiler
	kernel tilerA
	kernel tilerB
	static constexpr MatMult::tilingStruct tilingScheme

