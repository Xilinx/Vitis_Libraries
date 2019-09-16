.. index:: pair: namespace; xf::linear_algebra::blas
.. _doxid-namespacexf_1_1linear__algebra_1_1blas:

namespace xf::linear_algebra::blas
==================================

.. toctree::
	:hidden:

Overview
~~~~~~~~










.. index:: pair: function; processUpSbMatStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a954b1bce5720e5d344c6d113c19981a7:
.. index:: pair: function; processLoSbMatStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a151f2444626a97809d5e9f6ca6d28876:
.. index:: pair: function; forwardSbMatStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a724421c7ae56fd8854a8c70be802c8e5:
.. index:: pair: function; mergeGbMatStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1afcf00dfd51e0211317c6dca75b724ba2:
.. index:: pair: function; readUpSbMat2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a2f567d5e89ba8f333b0d11b861428cba:
.. index:: pair: function; readLoSbMat2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1af1de8f0dfad5a3097732f72e3e51fa88:
.. index:: pair: function; readVec2GbStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aac15db8a3d7546e1cf7f1b66dbb2544a:
.. index:: pair: function; readGbMat2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a4879e4430c6014619045c3f6e892a972:
.. index:: pair: function; readTbMat2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a6747be392623be3685d9720614f123c9:
.. index:: pair: function; readVec2TbStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ad17f98f106d111d84506324fa494cf23:
.. index:: pair: function; readSymUp2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a74588e4466d0bc977ce25fcda25cfb94:
.. index:: pair: function; mergeSymUpMat
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1adc64d99b1f94aecbc54b68edb548ffd3:
.. index:: pair: function; readSymLo2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a53e824699103a47b646b7789b0a421b2:
.. index:: pair: function; mergeSymLoMat
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1af28cc33fda8c604e40337f13b7236c4f:
.. index:: pair: function; readSpmUp2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a31e525dfc12d9bcfcaaa674907da5c1f:
.. index:: pair: function; readSpmLo2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aacd151535afc939e00965a142f7f9d37:
.. index:: pair: function; transpSymUpMatBlocks
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a1cffa56cc05a4a4e0b94155057a02a2f:
.. index:: pair: function; transpSymLoMatBlocks
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a61a446c9f4280cb33e3bcc7721111e1f:
.. index:: pair: function; transpMatBlocks
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1affb1496cebb654781e1db3f71008a111:
.. index:: pair: function; fwdMatBlocks
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ab826dd24fdc7d87f0ba23c028f38146e:
.. index:: pair: function; transpMemWordBlocks
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a371247cbacd11bc14c88e8b4cb56f49f:
.. index:: pair: function; transpMemBlocks
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aa587308c59f1e6617d210fa1375ff09c:
.. index:: pair: function; duplicateStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a5cb91e4f099e05e5ebcc1b9588d67da7:
.. index:: pair: function; splitStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ad449cb772b7b5afe898fda1c3f31faeb:
.. index:: pair: function; combineStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a3cb793be1c60214b0da185abe3e04711:
.. index:: pair: function; mem2stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aaf1824fd6502db676b0bfce97d233db2:
.. index:: pair: function; stream2mem
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a56e8ae76249d44d1c1f2d5288ad12e91:




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
	void :ref:`sbmSuper2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a1a3044957a69e10be52a38e12f73c96d>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`sbmSub2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a7fa5ede53334360121342c756cbe1274>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`gbm2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a68143f3cf719d0c1419622c5bb1ea555>`(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2GbMatStream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a69fef4d3a5fd2e704c94d7d3024d45e9>`(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readTbMat2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readVec2TbStream(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`tbmSuper2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1ae3da7c03cd80db55a51a498c22079ce2>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void :ref:`tbmSub2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a88850487baf47635a45f7bbc630f47dc>`(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TbUpMatStream<doxid-namespacexf_1_1linear__algebra_1_1blas_1ae7499f0f9c75c3842239c758155bbcd3>`(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TbLoMatStream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a52398b3f9b40b9c6cd149eb77d40bba2>`(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`gem2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1afbba4a95c789b0cc7d613eb0b100e705>`(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2GemStream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a0aed84d333c3d762deb7e081c8f00726>`(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

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
	void :ref:`symUp2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a44f6cef37546e91ba7ad853d88a4533a>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`symLo2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1ab99b1bd6a155a9fe7f11c59df85a4584>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`spmUp2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a82fecd7e39d57c68d729be4aecd2a197>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`spmLo2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a11825b8bdf64152a90d4f89cf339f86d>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2SymStream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a56fd2f6abf9e6237c4dfc7e42007617d>`(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

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
	void :ref:`trmUp2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a2726ba59d9ae971a1a5e74b966f4482e>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`trmLo2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a24a7a0c9380a8d06c1be6e5c2dbf4e8f>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`tpmUp2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a518a24e3393238916be6b4eb9ad1e7fa>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`tpmLo2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1a9c2f0e4fed02c8faf35bd46fcd7434e2>`(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TrmUpStream<doxid-namespacexf_1_1linear__algebra_1_1blas_1aa4d0bcf824058ca8b55034787019dbc1>`(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`vec2TrmLoStream<doxid-namespacexf_1_1linear__algebra_1_1blas_1abb9e836baa437e3b8092d1fba39056ad>`(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <unsigned int t_NumStreams, typename t_DataType>
	void duplicateStream(unsigned int p_n, hls::stream<t_DataType>& p_inputStream, hls::stream<t_DataType> p_streams [t_NumStreams]);

	template  <unsigned int t_NumStreams, typename t_DataType>
	void splitStream(unsigned int p_n, hls::stream<WideType<t_DataType, t_NumStreams>>& p_wideStream, hls::stream<WideType<t_DataType, 1>> p_stream [t_NumStreams]);

	template  <unsigned int t_NumStreams, typename t_DataType>
	void combineStream(unsigned int p_n, hls::stream<WideType<t_DataType, 1>> p_stream [t_NumStreams], hls::stream<WideType<t_DataType, t_NumStreams>>& p_wideStream);

	template  <typename t_DataType>
	void mem2stream(unsigned int p_n, t_DataType* p_in, hls::stream<t_DataType>& p_out);

	template  <typename t_DataType>
	void stream2mem(unsigned int p_n, hls::stream<t_DataType>& p_in, t_DataType* p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`readVec2Stream<doxid-namespacexf_1_1linear__algebra_1_1blas_1aed318603cc8ebb717d1125a882ee0113>`(t_DataType* p_in, unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out);

	template  <typename t_DataType, unsigned int t_ParEntries>
	void :ref:`writeStream2Vec<doxid-namespacexf_1_1linear__algebra_1_1blas_1ab875e116bdda28387f38e2b9ccbf8c0c>`(hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, unsigned int p_n, t_DataType* p_out);

	} // namespace blas
.. _details-doxid-namespacexf_1_1linear__algebra_1_1blas:

Detailed Documentation
~~~~~~~~~~~~~~~~~~~~~~



Global Functions
----------------

sbmSuper2Stream
################

.. index:: pair: function; sbmSuper2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a1a3044957a69e10be52a38e12f73c96d:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void sbmSuper2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

sbmSuper2Stream function that moves symmetric banded matrix with super diagonals from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- t_ParBlocks

		- number of t_ParEntries, p_n must be multiple t_ParEntries * t_ParBlocks

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_k

		- number of superdiagonals

	*
		- p_a

		- a p_n x p_n symmetric banded matrix with on-chip column-major storage and corresponding 0 paddings

	*
		- p_out

		- output stream, which is row-aligned with 0 paddings along subdiagonals

sbmSub2Stream
##############

.. index:: pair: function; sbmSub2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a7fa5ede53334360121342c756cbe1274:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void sbmSub2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

sbmSub2Stream function that moves symmetric banded matrix with sub diagonals from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- t_ParBlocks

		- number of t_ParEntries, p_n must be multiple t_ParEntries * t_ParBlocks

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_k

		- number of subdiagonals

	*
		- p_a

		- a p_n x p_n symmetric banded matrix with on-chip column-major storage and corresponding 0 paddings

	*
		- p_out

		- output stream, which is row-aligned with 0 paddings along subdiagonals

gbm2Stream
###########

.. index:: pair: function; gbm2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a68143f3cf719d0c1419622c5bb1ea555:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void gbm2Stream(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

gbm2Stream function that moves symmetric banded matrix with from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- t_ParBlocks

		- number of t_ParEntries, p_n must be multiple t_ParEntries * t_ParBlocks

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_kl

		- number of subdiagonals

	*
		- p_ku

		- number of superdiagonals

	*
		- p_a

		- a p_m x p_n symmetric banded matrix with on-chip column-major storage and corresponding 0 paddings

	*
		- p_out

		- output stream, which is row-aligned with 0 paddings along subdiagonals

vec2GbMatStream
################

.. index:: pair: function; vec2GbMatStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a69fef4d3a5fd2e704c94d7d3024d45e9:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2GbMatStream(unsigned int p_n, unsigned int p_kl, unsigned int p_ku, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

vec2SbMatStream function that moves vector from memory to stream that matches the sbMat2Stream outputs



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_ku

		- number of superdiagonals

	*
		- p_kl

		- number of subdiagonals

	*
		- p_x

		- vector input

	*
		- p_out

		- output stream, which matches the outputs of gbMat2Stream or sbMat2Stream

tbmSuper2Stream
################

.. index:: pair: function; tbmSuper2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ae3da7c03cd80db55a51a498c22079ce2:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void tbmSuper2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

tbmSuper2Stream function that moves triangular banded matrix with super diagonals from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- t_ParBlocks

		- number of t_ParEntries, p_n must be multiple t_ParEntries * t_ParBlocks

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_k

		- number of superdiagonals

	*
		- p_a

		- a p_n x p_n triangular banded matrix with on-chip column-major storage and corresponding 0 paddings

	*
		- p_out

		- output stream, which is row-aligned with 0 paddings along subdiagonals

tbmSub2Stream
##############

.. index:: pair: function; tbmSub2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a88850487baf47635a45f7bbc630f47dc:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_ParBlocks = 1>
	void tbmSub2Stream(unsigned int p_n, unsigned int p_k, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

tbmSub2Stream function that moves triangular banded matrix with sub diagonals from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- t_ParBlocks

		- number of t_ParEntries, p_n must be multiple t_ParEntries * t_ParBlocks

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_k

		- number of subdiagonals

	*
		- p_a

		- a p_n x p_n triangular banded matrix with on-chip column-major storage and corresponding 0 paddings

	*
		- p_out

		- output stream, which is row-aligned with 0 paddings along subdiagonals

vec2TbUpMatStream
##################

.. index:: pair: function; vec2TbUpMatStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ae7499f0f9c75c3842239c758155bbcd3:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TbUpMatStream(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

vec2TbUpMatStream function that moves vector from memory to stream that matches the sbMat2Stream outputs



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_k

		- number of super/sub-diagonals

	*
		- p_x

		- vector input

	*
		- p_out

		- output stream

vec2TbLoMatStream
##################

.. index:: pair: function; vec2TbLoMatStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a52398b3f9b40b9c6cd149eb77d40bba2:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TbLoMatStream(unsigned int p_n, unsigned int p_k, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

vec2TbLoMatStream function that moves vector from memory to stream that matches the sbMat2Stream outputs



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_k

		- number of sub-diagonals

	*
		- p_x

		- vector input

	*
		- p_out

		- output stream

gem2Stream
###########

.. index:: pair: function; gem2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1afbba4a95c789b0cc7d613eb0b100e705:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void gem2Stream(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

gem2Stream function that moves row-major matrix from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_m

		- number of rows in a matrix

	*
		- p_n

		- number of cols in a matrix

	*
		- p_in

		- a p_m x p_n matrix with on-chip row-major storage

	*
		- p_out

		- output stream

vec2GemStream
##############

.. index:: pair: function; vec2GemStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a0aed84d333c3d762deb7e081c8f00726:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2GemStream(unsigned int p_m, unsigned int p_n, t_DataType* p_in, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

vec2GemStream function that moves vector from memory to stream that matches the gem2Stream outputs



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_m

		- number of rows in a matrix

	*
		- p_n

		- number of cols in a matrix

	*
		- p_in

		- vector input

	*
		- p_out

		- output stream

symUp2Stream
#############

.. index:: pair: function; symUp2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a44f6cef37546e91ba7ad853d88a4533a:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void symUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

symUp2Stream function that moves super-symmetric matrix from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallel processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- point to a p_n x p_n symmetric matrix

	*
		- p_out

		- output stream

symLo2Stream
#############

.. index:: pair: function; symLo2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ab99b1bd6a155a9fe7f11c59df85a4584:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void symLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

symLo2Stream function that moves sub-symmetric matrix from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallel processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- point to a p_n x p_n symmetric matrix

	*
		- p_out

		- output stream

spmUp2Stream
#############

.. index:: pair: function; spmUp2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a82fecd7e39d57c68d729be4aecd2a197:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void spmUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

spmUp2Stream function that moves packed super-symmetric matrix from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallel processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- point to a p_n x p_n symmetric matrix

	*
		- p_out

		- output stream

spmLo2Stream
#############

.. index:: pair: function; spmLo2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a11825b8bdf64152a90d4f89cf339f86d:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void spmLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

spmLo2Stream function that moves packed sub-symmetric matrix from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallel processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- point to a p_n x p_n symmetric matrix

	*
		- p_out

		- output stream

vec2SymStream
##############

.. index:: pair: function; vec2SymStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a56fd2f6abf9e6237c4dfc7e42007617d:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2SymStream(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

vec2SymStream function that moves vector from memory to stream that matches the symatrix matrix data mover outputs



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_x

		- vector input

	*
		- p_out

		- output stream

trmUp2Stream
#############

.. index:: pair: function; trmUp2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a2726ba59d9ae971a1a5e74b966f4482e:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void trmUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

trmUp2Stream function that read the super-triangular matrix from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- the number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- memory location of a p_n x p_n symmetric matrix

	*
		- p_out

		- the streams of matrix entries

trmLo2Stream
#############

.. index:: pair: function; trmLo2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a24a7a0c9380a8d06c1be6e5c2dbf4e8f:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void trmLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

trmLo2Stream function that read the sub-tridiagonal matrix with from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- the number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- memory location of a p_n x p_n symmetric matrix

	*
		- p_out

		- the streams of matrix entries

tpmUp2Stream
#############

.. index:: pair: function; tpmUp2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a518a24e3393238916be6b4eb9ad1e7fa:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void tpmUp2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

tpmUp2Stream function that read the packed super-triangular matrix from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- the number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- memory location of a p_n x p_n symmetric matrix

	*
		- p_out

		- the streams of matrix entries

tpmLo2Stream
#############

.. index:: pair: function; tpmLo2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a9c2f0e4fed02c8faf35bd46fcd7434e2:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void tpmLo2Stream(unsigned int p_n, t_DataType* p_a, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

tpmLo2Stream function that read the packed sub-symmetric matrix with from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- the number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a symmetric matrix

	*
		- p_a

		- memory location of a p_n x p_n symmetric matrix

	*
		- p_out

		- the streams of matrix entries

vec2TrmUpStream
################

.. index:: pair: function; vec2TrmUpStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aa4d0bcf824058ca8b55034787019dbc1:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TrmUpStream(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

vec2TrmUpStream function that moves vector from memory to stream that matches the trmUp2Stream/tpmUp2Stream outputs



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_x

		- vector input

	*
		- p_out

		- output stream

vec2TrmLoStream
################

.. index:: pair: function; vec2TrmLoStream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1abb9e836baa437e3b8092d1fba39056ad:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void vec2TrmLoStream(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

vec2TrmLoStream function that moves vector from memory to stream that matches the trmLo2Stream/tpmLo2Stream outputs



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of rows/cols in a square matrix

	*
		- p_x

		- vector input

	*
		- p_out

		- output stream

readVec2Stream
###############

.. index:: pair: function; readVec2Stream
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aed318603cc8ebb717d1125a882ee0113:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void readVec2Stream(t_DataType* p_in, unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_out)

readVec2Stream function that moves vector from memory to stream



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of entries in a vectpr

	*
		- p_in

		- vector input

	*
		- p_out

		- output stream

writeStream2Vec
################

.. index:: pair: function; writeStream2Vec
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ab875e116bdda28387f38e2b9ccbf8c0c:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries>
	void writeStream2Vec(hls::stream<WideType<t_DataType, t_ParEntries>>& p_in, unsigned int p_n, t_DataType* p_out)

writeStream2Vec function that moves vector from stream to vector



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the matrix entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the matrix

	*
		- p_n

		- number of entries in a vectpr

	*
		- p_in

		- vector stream input

	*
		- p_out

		- vector output memory

