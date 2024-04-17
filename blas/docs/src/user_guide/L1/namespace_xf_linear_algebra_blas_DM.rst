.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, namespace, blas, DM
   :description: Vitis BLAS library namespace xf::blas.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. index:: pair: namespace; xf::blas
.. _doxid-namespacexf_1_1blas:

namespace xf::blas
==================

.. toctree::
	:hidden:

Overview
~~~~~~~~










.. index:: pair: function; processUpSbMatStream
.. _doxid-namespacexf_1_1blas_1a2495ad4636a060f0f92cd4712b9eb134:
.. index:: pair: function; processLoSbMatStream
.. _doxid-namespacexf_1_1blas_1abe623682c98911ce8600c365a4ad2919:
.. index:: pair: function; forwardSbMatStream
.. _doxid-namespacexf_1_1blas_1ad892bcbfe5fa6b16b61a7bf472d2ff07:
.. index:: pair: function; mergeGbMatStream
.. _doxid-namespacexf_1_1blas_1a4594e681e13c06578314ccc90c2c5b00:
.. index:: pair: function; readUpSbMat2Stream
.. _doxid-namespacexf_1_1blas_1a044eba4914aac22d380c1428c5338968:
.. index:: pair: function; readLoSbMat2Stream
.. _doxid-namespacexf_1_1blas_1a77b3365af5bf03fc29aad7fdcb1814cf:
.. index:: pair: function; readVec2GbStream
.. _doxid-namespacexf_1_1blas_1aad6e6d12ab5acf9cc165f20bc7c5f23e:
.. index:: pair: function; readGbMat2Stream
.. _doxid-namespacexf_1_1blas_1a9c88af4c7d5d98e4b810662943154e9d:
.. index:: pair: function; readTbMat2Stream
.. _doxid-namespacexf_1_1blas_1a771db4976c2966e613ead25fa0171fec:
.. index:: pair: function; readVec2TbStream
.. _doxid-namespacexf_1_1blas_1a8865288f181f8759c39ac26fe0aff365:
.. index:: pair: function; readSymUp2Stream
.. _doxid-namespacexf_1_1blas_1a8d1d53ee3c834461d3beedb1fed26785:
.. index:: pair: function; mergeSymUpMat
.. _doxid-namespacexf_1_1blas_1a507acec27d0062f5d6d3dc80c837b599:
.. index:: pair: function; readSymLo2Stream
.. _doxid-namespacexf_1_1blas_1a3758f608e05ef82f2d6578ce3bd2228b:
.. index:: pair: function; mergeSymLoMat
.. _doxid-namespacexf_1_1blas_1a64baddf22a0035c1fae702308764e96a:
.. index:: pair: function; readSpmUp2Stream
.. _doxid-namespacexf_1_1blas_1a36561b958b55357d2e4c19d106b8c31c:
.. index:: pair: function; readSpmLo2Stream
.. _doxid-namespacexf_1_1blas_1a71a7316c46f3ba5fe04cf909e8316856:
.. index:: pair: function; transpSymUpMatBlocks
.. _doxid-namespacexf_1_1blas_1a8eef5e97f9f9c40535611a6635f930c6:
.. index:: pair: function; transpSymLoMatBlocks
.. _doxid-namespacexf_1_1blas_1a270da4b2ac1d08c7a26dcf71976bb7c9:
.. index:: pair: function; transpMatBlocks
.. _doxid-namespacexf_1_1blas_1a2da98c176ee4cce3d60b867d5aa00a99:
.. index:: pair: function; fwdMatBlocks
.. _doxid-namespacexf_1_1blas_1afe180bccc70ee2f75ae10b3053684c64:
.. index:: pair: function; transpMemWordBlocks
.. _doxid-namespacexf_1_1blas_1a223db28e7995e5c7c7d875c3ce21eb9d:
.. index:: pair: function; transpMemBlocks
.. _doxid-namespacexf_1_1blas_1ab548ae6d02f6c8243f985e67c1cc44d9:
.. index:: pair: function; duplicateStream
.. _doxid-namespacexf_1_1blas_1a5406424905a2f2adabbafcb85c598e96:
.. index:: pair: function; splitStream
.. _doxid-namespacexf_1_1blas_1a11db1c4e77eecd52f182289c896d69f7:
.. index:: pair: function; combineStream
.. _doxid-namespacexf_1_1blas_1ac8dbc7309c42298377129396b21847dc:
.. index:: pair: function; mem2stream
.. _doxid-namespacexf_1_1blas_1a3b7ea344b23df0528ef362b7a5286ba1:
.. index:: pair: function; stream2mem
.. _doxid-namespacexf_1_1blas_1afbae1bb07299d85a4a4f6103cceac24e:




.. ref-code-block:: cpp
	:class: overview-code-block

	
	namespace blas {

	// global functions

	template  <typename t_DataType, unsigned int t_ParEntries, int t_LastRowIdx = 0>
	void processUpSbMatStream(unsigned int p_n, unsigned int p_k, hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void processLoSbMatStream(unsigned int p_n, unsigned int p_k, hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void forwardSbMatStream(unsigned int p_n, unsigned int p_k, hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void mergeGbMatStream(unsigned int p_n, unsigned int p_ku, unsigned int p_kl, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inUp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inLo, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readUpSbMat2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outUp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outLo);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readLoSbMat2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outUp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outLo);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readVec2GbStream(unsigned int p_n, unsigned int p_ku, unsigned int p_kl, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outUp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outLo);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readGbMat2Stream(unsigned int p_n, unsigned int p_ku, unsigned int p_kl, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outUp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outLo);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`sbmSuper2Stream<doxid-namespacexf_1_1blas_1abfb09fea27fb695144c04f4f18c7d4ef>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`sbmSub2Stream<doxid-namespacexf_1_1blas_1aba13a321f3ff48e4396f8c33fa4edfc7>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`gbm2Stream<doxid-namespacexf_1_1blas_1a5c5604aeb187880886cb360b70d0b89b>`(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2GbMatStream<doxid-namespacexf_1_1blas_1ae176c81315fd4d4f0e7857c86803c934>`(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readTbMat2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readVec2TbStream(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`tbmSuper2Stream<doxid-namespacexf_1_1blas_1a65e9f8f28f2f85754bdd3b47ffb8ff67>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`tbmSub2Stream<doxid-namespacexf_1_1blas_1a68da86871fff3c145e2b03c65d01f7aa>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TbUpMatStream<doxid-namespacexf_1_1blas_1aa664123ae830997774fe06bae1504849>`(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TbLoMatStream<doxid-namespacexf_1_1blas_1ae6d7d08cd7582d060b0dfc7b071f6f8e>`(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`gem2Stream<doxid-namespacexf_1_1blas_1a9366b8a509eab67e58496e48f5e7db1c>`(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2GemStream<doxid-namespacexf_1_1blas_1a5de93316f912eb02ba71742d9ead9460>`(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readSymUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outSymUpTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outForward);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void mergeSymUpMat(unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inSymUpTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inForward, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readSymLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outSymUpTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outForward);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void mergeSymLoMat(unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inSymUpTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_inForward, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readSpmUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outSymUpTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outForward);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readSpmLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outSymUpTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outTransp, hls::stream<WideType<t_DataType, t_ParEntries>>& p_outForward);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`symUp2Stream<doxid-namespacexf_1_1blas_1abfe112bea0c006e3c2e7dd1b3bac2ffc>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`symLo2Stream<doxid-namespacexf_1_1blas_1a610dea18a22934abfaeeca444bc638c0>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`spmUp2Stream<doxid-namespacexf_1_1blas_1a5ac564d9116d2ae939ce5573227b03b1>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`spmLo2Stream<doxid-namespacexf_1_1blas_1a367f272df208c12b957ebb39e4e9fc3e>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2SymStream<doxid-namespacexf_1_1blas_1a79f1e23a9f1dec5d5ffeb43e9f5f9451>`(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void transpSymUpMatBlocks(unsigned int p_blocks, hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void transpSymLoMatBlocks(unsigned int p_blocks, hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void transpMatBlocks(unsigned int p_blocks, hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void fwdMatBlocks(unsigned int p_blocks, hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_MemWidth, unsigned int t_Rows, unsigned int t_Cols>
	void transpMemWordBlocks(unsigned int p_blocks, hls::stream<WideType<t_DataType, t_MemWidth>>& p_in, hls::stream<WideType<t_DataType, t_MemWidth>>& p_out);

	template  <typename t_DataType, unsigned int t_MemWidth, unsigned int t_Rows, unsigned int t_Cols>
	void transpMemBlocks(unsigned int p_blocks, hls::stream<WideType<t_DataType, t_MemWidth>>& p_in, hls::stream<WideType<t_DataType, t_MemWidth>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`trmUp2Stream<doxid-namespacexf_1_1blas_1aec02cf5c5a545e4e6c3621224cb902b5>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`trmLo2Stream<doxid-namespacexf_1_1blas_1a558f0dec2d2f5b9d457953e24103894c>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`tpmUp2Stream<doxid-namespacexf_1_1blas_1ad9fe287119a5f1b1b1376f9bf75fed7e>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`tpmLo2Stream<doxid-namespacexf_1_1blas_1a8614b7080d43a00fa270f42a0dab6b63>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TrmUpStream<doxid-namespacexf_1_1blas_1aae2a668d542da20fb929364274ba54e7>`(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TrmLoStream<doxid-namespacexf_1_1blas_1adae99af045be5874080b1a5687d767b1>`(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <unsigned int t_NumStreams, typename t_DataType>
	void duplicateStream(unsigned int p_n, hls::stream<t_DataType>& p_inputStream, hls::stream<t_DataType> p_streams [t_NumStreams]);

	template  <unsigned int t_NumStreams, typename t_DataType>
	void splitStream(unsigned int p_n, hls::stream<WideType<t_DataType, t_NumStreams>>& p_wideStream, hls::stream<WideType<t_DataType, 1>> p_stream [t_NumStreams]);

	template  <unsigned int t_NumStreams, typename t_DataType>
	void combineStream(unsigned int p_n, hls::stream<WideType<t_DataType, 1>> p_stream [t_NumStreams], hls::stream<WideType<t_DataType, t_NumStreams>>& p_wideStream);

	template  <typename t_DataType, typename t_DesDataType = t_DataType>
	void mem2stream(unsigned int p_n, t_DataType* p_in, hls::stream<t_DesDataType>& p_out);

	template  <typename t_DataType, typename t_DesDataType = t_DataType>
	void stream2mem(unsigned int p_n, hls::stream<t_DataType>& p_in, t_DesDataType* p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`readVec2Stream<doxid-namespacexf_1_1blas_1aa423828b33277a4b1c048e936816f30f>`(t_DataType* p_in, unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`writeStream2Vec<doxid-namespacexf_1_1blas_1a4f103e41ef6239a3608653e2f5b08da1>`(hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, unsigned int p_n, t_DataType* p_out);

	} // namespace blas
.. _details-doxid-namespacexf_1_1blas:

Detailed Documentation
~~~~~~~~~~~~~~~~~~~~~~


Global Functions
----------------

sbmSuper2Stream
################

.. index:: pair: function; sbmSuper2Stream
.. _doxid-namespacexf_1_1blas_1abfb09fea27fb695144c04f4f18c7d4ef:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void sbmSuper2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The sbmSuper2Stream function that moves the symmetric banded matrix with super diagonals from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- t_ParBlocks

		- Number of t_ParEntries; p_n must be multiple t_ParEntries * t_ParBlocks.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_k

		- Number of superdiagonals.

	*
		- p_a

		- a p_n x p_n symmetric banded matrix with on-chip column-major storage and corresponding 0 paddings.

	*
		- p_out

		- Output stream, which is row-aligned with 0 paddings along subdiagonals.

sbmSub2Stream
##############

.. index:: pair: function; sbmSub2Stream
.. _doxid-namespacexf_1_1blas_1aba13a321f3ff48e4396f8c33fa4edfc7:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void sbmSub2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The sbmSub2Stream function that moves the symmetric banded matrix with sub diagonals from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- t_ParBlocks

		- Number of t_ParEntries; p_n must be multiple t_ParEntries * t_ParBlocks.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_k

		- Number of subdiagonals.

	*
		- p_a

		- A p_n x p_n symmetric banded matrix with on-chip column-major storage and corresponding 0 paddings.

	*
		- p_out

		- Output stream, which is row-aligned with 0 paddings along subdiagonals.

gbm2Stream
###########

.. index:: pair: function; gbm2Stream
.. _doxid-namespacexf_1_1blas_1a5c5604aeb187880886cb360b70d0b89b:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void gbm2Stream(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The gbm2Stream function that moves the symmetric banded matrix with from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- t_ParBlocks

		- Number of t_ParEntries; p_n must be multiple t_ParEntries * t_ParBlocks.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_kl

		- Number of subdiagonals.

	*
		- p_ku

		- Number of superdiagonals.

	*
		- p_a

		- A p_m x p_n symmetric banded matrix with on-chip column-major storage and corresponding 0 paddings.

	*
		- p_out

		- Output stream, which is row-aligned with 0 paddings along subdiagonals.

vec2GbMatStream
################

.. index:: pair: function; vec2GbMatStream
.. _doxid-namespacexf_1_1blas_1ae176c81315fd4d4f0e7857c86803c934:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2GbMatStream(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The vec2SbMatStream function that moves the vector from memory to stream that matches the sbMat2Stream outputs.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_ku

		- Number of superdiagonals.

	*
		- p_kl

		- Number of subdiagonals.

	*
		- p_x

		- Vector input.

	*
		- p_out

		- Output stream, which matches the outputs of gbMat2Stream or sbMat2Stream.

tbmSuper2Stream
################

.. index:: pair: function; tbmSuper2Stream
.. _doxid-namespacexf_1_1blas_1a65e9f8f28f2f85754bdd3b47ffb8ff67:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void tbmSuper2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The tbmSuper2Stream function that moves the triangular banded matrix with super diagonals from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- t_ParBlocks

		- Number of t_ParEntries, p_n must be multiple t_ParEntries * t_ParBlocks.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_k

		- Number of superdiagonals.

	*
		- p_a

		- A p_n x p_n triangular banded matrix with on-chip column-major storage and corresponding 0 paddings.

	*
		- p_out

		- Output stream, which is row-aligned with 0 paddings along subdiagonals.

tbmSub2Stream
##############

.. index:: pair: function; tbmSub2Stream
.. _doxid-namespacexf_1_1blas_1a68da86871fff3c145e2b03c65d01f7aa:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void tbmSub2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The tbmSub2Stream function that moves the triangular banded matrix with sub diagonals from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- t_ParBlocks

		- Number of t_ParEntries; p_n must be multiple t_ParEntries * t_ParBlocks.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_k

		- Number of subdiagonals.

	*
		- p_a

		- A p_n x p_n triangular banded matrix with on-chip column-major storage and corresponding 0 paddings.

	*
		- p_out

		- Output stream, which is row-aligned with 0 paddings along subdiagonals.

vec2TbUpMatStream
##################

.. index:: pair: function; vec2TbUpMatStream
.. _doxid-namespacexf_1_1blas_1aa664123ae830997774fe06bae1504849:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TbUpMatStream(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The vec2TbUpMatStream function that moves the vector from memory to stream that matches the sbMat2Stream outputs.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_k

		- Number of super/sub-diagonals.

	*
		- p_x

		- Vector input.

	*
		- p_out

		- Output stream.

vec2TbLoMatStream
##################

.. index:: pair: function; vec2TbLoMatStream
.. _doxid-namespacexf_1_1blas_1ae6d7d08cd7582d060b0dfc7b071f6f8e:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TbLoMatStream(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The vec2TbLoMatStream function that moves the vector from memory to stream that matches the sbMat2Stream outputs.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_k

		- Number of sub-diagonals.

	*
		- p_x

		- Vector input.

	*
		- p_out

		- Output stream.

gem2Stream
###########

.. index:: pair: function; gem2Stream
.. _doxid-namespacexf_1_1blas_1a9366b8a509eab67e58496e48f5e7db1c:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void gem2Stream(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The gem2Stream function that moves the row-major matrix from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_m

		- Number of rows in a matrix.

	*
		- p_n

		- Number of cols in a matrix.

	*
		- p_in

		- A p_m x p_n matrix with on-chip row-major storage.

	*
		- p_out

		- Output stream.

vec2GemStream
##############

.. index:: pair: function; vec2GemStream
.. _doxid-namespacexf_1_1blas_1a5de93316f912eb02ba71742d9ead9460:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2GemStream(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The vec2GemStream function that moves the vector from memory to stream that matches the gem2Stream outputs.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_m

		- Number of rows in a matrix.

	*
		- p_n

		- Number of cols in a matrix.

	*
		- p_in

		- Vector input.

	*
		- p_out

		- Output stream.

symUp2Stream
#############

.. index:: pair: function; symUp2Stream
.. _doxid-namespacexf_1_1blas_1abfe112bea0c006e3c2e7dd1b3bac2ffc:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void symUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The symUp2Stream function that moves the super-symmetric matrix from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallel processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Point to a p_n x p_n symmetric matrix.

	*
		- p_out

		- Output stream.

symLo2Stream
#############

.. index:: pair: function; symLo2Stream
.. _doxid-namespacexf_1_1blas_1a610dea18a22934abfaeeca444bc638c0:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void symLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The symLo2Stream function that moves the sub-symmetric matrix from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallel processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Point to a p_n x p_n symmetric matrix.

	*
		- p_out

		- Output stream.

spmUp2Stream
#############

.. index:: pair: function; spmUp2Stream
.. _doxid-namespacexf_1_1blas_1a5ac564d9116d2ae939ce5573227b03b1:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void spmUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The spmUp2Stream function that moves the packed super-symmetric matrix from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallel processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Point to a p_n x p_n symmetric matrix.

	*
		- p_out

		- Output stream.

spmLo2Stream
#############

.. index:: pair: function; spmLo2Stream
.. _doxid-namespacexf_1_1blas_1a367f272df208c12b957ebb39e4e9fc3e:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void spmLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The spmLo2Stream function that moves the packed sub-symmetric matrix from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallel processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Point to a p_n x p_n symmetric matrix.

	*
		- p_out

		- Output stream.

vec2SymStream
##############

.. index:: pair: function; vec2SymStream
.. _doxid-namespacexf_1_1blas_1a79f1e23a9f1dec5d5ffeb43e9f5f9451:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2SymStream(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The vec2SymStream function that moves the vector from memory to stream that matches the symatrix matrix data mover outputs.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_x

		- Vector input.

	*
		- p_out

		- Output stream.

trmUp2Stream
#############

.. index:: pair: function; trmUp2Stream
.. _doxid-namespacexf_1_1blas_1aec02cf5c5a545e4e6c3621224cb902b5:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void trmUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The trmUp2Stream function that read the super-triangular matrix from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- The number of parallelly processed entries in the matrix

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Memory location of a p_n x p_n symmetric matrix.

	*
		- p_out

		- The streams of matrix entries.

trmLo2Stream
#############

.. index:: pair: function; trmLo2Stream
.. _doxid-namespacexf_1_1blas_1a558f0dec2d2f5b9d457953e24103894c:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void trmLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The trmLo2Stream function that read the sub-tridiagonal matrix with from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- The number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Memory location of a p_n x p_n symmetric matrix.

	*
		- p_out

		- The streams of matrix entries.

tpmUp2Stream
#############

.. index:: pair: function; tpmUp2Stream
.. _doxid-namespacexf_1_1blas_1ad9fe287119a5f1b1b1376f9bf75fed7e:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void tpmUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The tpmUp2Stream function that read the packed super-triangular matrix from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- The number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Memory location of a p_n x p_n symmetric matrix.

	*
		- p_out

		- The streams of matrix entries.

tpmLo2Stream
#############

.. index:: pair: function; tpmLo2Stream
.. _doxid-namespacexf_1_1blas_1a8614b7080d43a00fa270f42a0dab6b63:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void tpmLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The tpmLo2Stream function that read the packed sub-symmetric matrix with from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- The number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a symmetric matrix.

	*
		- p_a

		- Memory location of a p_n x p_n symmetric matrix.

	*
		- p_out

		- The streams of matrix entries.

vec2TrmUpStream
################

.. index:: pair: function; vec2TrmUpStream
.. _doxid-namespacexf_1_1blas_1aae2a668d542da20fb929364274ba54e7:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TrmUpStream(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The vec2TrmUpStream function that moves the vector from memory to stream that matches the trmUp2Stream/tpmUp2Stream outputs.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_x

		- Vector input.

	*
		- p_out

		- Output stream.

vec2TrmLoStream
################

.. index:: pair: function; vec2TrmLoStream
.. _doxid-namespacexf_1_1blas_1adae99af045be5874080b1a5687d767b1:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TrmLoStream(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The vec2TrmLoStream function that moves the vector from memory to stream that matches the trmLo2Stream/tpmLo2Stream outputs.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of rows/cols in a square matrix.

	*
		- p_x

		- Vector input.

	*
		- p_out

		- Output stream.

readVec2Stream
###############

.. index:: pair: function; readVec2Stream
.. _doxid-namespacexf_1_1blas_1aa423828b33277a4b1c048e936816f30f:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readVec2Stream(t_DataType* p_in, unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

The readVec2Stream function that moves the vector from memory to stream.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of entries in a vector.

	*
		- p_in

		- vector input

	*
		- p_out

		- Output stream.

writeStream2Vec
################

.. index:: pair: function; writeStream2Vec
.. _doxid-namespacexf_1_1blas_1a4f103e41ef6239a3608653e2f5b08da1:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void writeStream2Vec(hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, unsigned int p_n, t_DataType* p_out)

The writeStream2Vec function that moves the vector from stream to vector.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- The data type of the matrix entries.

	*
		- t_ParEntries

		- Number of parallelly processed entries in the matrix.

	*
		- p_n

		- Number of entries in a vector.

	*
		- p_in

		- Vector stream input.

	*
		- p_out

		- Vector output memory.

