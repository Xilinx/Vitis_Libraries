# Compaction Kernel 
This kernel is the main work horse for offloading compaction job to FPGA from CPU. A compaction refers to the process of merging multiple sorted input K-V files 
into one single sorted output. The main job is to sort different input streams of overlapping/non-overlapping keys, as well as other jobs like deleting duplicates or shadowing values.

An example compaction job could be the following, suppose we have two sorted input files, which contain the unique keys (the corresponding values are ommited):

* file1:  bb  ddd 

* file2:  aa  cc ee f hhhh

The compaction output should be a single file containing keys:

* file_out: aa bb cc ddd ee f hhhh

## Kernel Architecture
### Input
The kernel is a two input, one output compaction kernel which will sort the incoming values from two input streams. The values from each stream is assumed to be sorted already and has at least one key. 
Each input stream is composed of three ports: **_streamID_**, **_metaInfo_** and **_dataBuff_**. The steramID port is a single unsigned int value representing the index of this
input stream. The metaInfo port contains array of 32 bit unsigned values, where the first value represents the total number of keys for that input and subsequence values represent the number of bytes for each key from that file. The dataBuff port contains input key bytes concatenated 
with alignment or space.

Since the input keys are of various sizes, all input keys are first tokenized and turned into one or multiple tokens. Each **_token_** contains a fixed number of key bytes 
and a flag bit marking the end of token(s) for current key. The number of key bytes is defined in **_config.h_** by variable **_BufWidthSet_**. **_utils.h_** contains the token definition
**_keyDecodeWord_** as well as the tokenize function **_KeyDecode_**.

In cases where multiple input files need compaction, the host can call the kernel multiple times and each time reuse the results stored in DDR for previous calls. The **_KeyDecode_** 
function can also read from results stored previously in DDR memory and use it as a new input stream. To use this mode, a streamID of 0xFF can be provided to indicate the input stream
is read from previous results.

The output of the **_KeyDecode_** function is a hls stream of **_KeyDecodeWord_** struct type. There will be two decoders in parallel, each reading either a new input stream from host
or previous compaction results stored in DDR. The output will go to the comparison module.

### Comparison
The comparison module is defined in comparison.h and its main function is **_KeyCompare_**. It takes a token from each of the two input streams, compare the values and output the smaller one.
It will finish emitting the rest of the tokens from the same key and compare the next pair. There is an internal memory array storing the tokens that are not selected for output for the 
next pair of key comparison. For example,

* file1:  bb  ddd 

* file2:  aa  cc ee f hhhh

Suppose each token is set to carry two bytes of data, then the two input streams shall become, note the 1 and 0 in front means if this token is the last token for current key or not.

* stream1:  1|bb  0|dd  1|d0 

* stream2:  1|aa  1|cc  1|ee  1|f0  0|hh  1|hh

The first comparison will write 1|aa to output and store 1|bb into the internal memory array. The next comparison will happen between 1|bb from the internal array and 1|cc from input stream and 1|bb will be emmited. In turn 1|cc shall be stored in the internal memory array. On the next cycle a new comparison happens between 0|dd and 1|cc, again 1|cc is emitted and 0|dd is stored. On next cycle when we compare 0|dd and 1|ee, 0|dd shall be emitted and 1|ee is stored. The token stream of that key however, is not finished, so on the next cycle the next token from input stream 1|d0 is put on to the output. After that since there is no more tokens/keys available for stream1, the rest of stream2 keys go to the output, 1|ee 1|f0 0|hh 1|hh.

* **_NOTE_**: the current comparison implementation may be further optimized for hls by removing recurring conditions tests and loop restructure.

### Output 
The compaction output will be a single merged key stream with two ports: one port is the meta information about this stream and the data port will contain the key bytes. Specifically, the **_StreamWrite_** module will do the actual serialization of the output key stream. The meta data memory region will be a series of 32 bit unsigned integers, with the first value representing the total key count in this stream. Subsequently, for each key an unsigned 32 bit word is written with bit[7:0] (the lowest byte) represent the source file index for this key, and bit[23:16] (the 3rd byte) is the number of tokens this key has. Note this information is formed in the comparison module. The data port contains the tokens for each key. For the host to utilzie the compaction result, the lowest byte from the meta stream can be used. 
