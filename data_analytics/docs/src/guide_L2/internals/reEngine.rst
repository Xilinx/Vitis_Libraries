.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

************************************
Regular Expression Engine (reEngine)
************************************

Overview
========

At the kernel level, the L2 ``reEngineKernel`` aims to provide an ability that a huge amount of logs could be matched efficiently under a single regular expression pattern where the L1 regex-VM matched only one message at a time. Thus, the ``reEngineKernel`` would be very suitable for analyzing a partiuclar logs, like `Apache2 (httpd)`_, `Apache_Error`_, `Nginx`_, or `Syslogs`_.

.. _`Apache2 (httpd)`: https://docs.fluentd.org/parser/apache2

.. _`Apache_Error`: https://docs.fluentd.org/parser/apache_error

.. _`Nginx`: https://docs.fluentd.org/parser/nginx

.. _`Syslogs`: https://docs.fluentd.org/parser/syslog


User Guide
==========

The limitations come from both the kernel side and regex-VM, because your ``reEngineKernel`` is built upon a L1 regex-VM. The kernel side limitations are only explained here; for those derived from the primitive,  refer to :ref:`guide-regex-VM`.

As the VM approach in L1 requires a certain size of on-chip RAMs inherently, the ``reEngineKernel`` is naturally bounded by RAMs. Thus, you cannot allocate an unlimited size of buffer for the ``cfg_buff`` configuration buffer, ``msg_buff`` message buffer, ``len_buff`` length buffer, or ``out_buff``result buffer.

Choosing the size for each buffer wisely is extremely critical for the overall throughput and XCLBIN buildability. Find a recommendation in ``re_engine_kernel.hpp`` in path ``L2/tests/text/reEngine/kernel``. Insufficient size for these parameters will directly cause the input regular expression pattern to not be handled by the regex-VM, and the reEngine, of course.

reEngine Usage
--------------

Before instantiating the reEngine, you have to pre-compile your regular expression using the software compiler provided in L1. First, check if the pattern is supported by the current implementation of the hardware VM. The compiler will give a ``XF_UNSUPPORTED_OPCODE`` error code if the pattern is not supported. A ``ONIG_NORMAL`` pass code along with the configurations (including instruction list, bit-set map, etc.) will be given if the input is a valid pattern. Then, you should pass these configurations, the input messages, along with its corresponding lengths in bytes under the format defined above to the ``reEngineKernel`` to trigger the matching process. The ``reEngineKernel`` will automatically be responsible for splitting them to individual inputs and feeding them into the buffers required by L1 regex-VM, also collecting the results after the matching process is done and placing them into an output result buffer correspondingly.

As mentioned in :ref:`guide-regex-VM`, the bit-set map, instruction buffer, message buffer, and offset buffer are needed in the hardware VM, and are not handled by itself. It is not necessary for you to deal with the details related to L1 at L2. Thus, these buffers will be automatically allocated in ``reEngineKernel`` according to the template parameters given. 

**Code Example**

The following section gives a usage example for using reEngine in a C++ based HLS design.

First, introduce the format of three buffer as the inputs of ``reEngineKernel`` here:

**cfg_buff**

.. image:: /images/cfgbuff_format.png
   :alt: Configuration Buffer Format
   :width: 80%
   :align: center

**msg_buff**

.. image:: /images/msgbuff_format.png
   :alt: Message Buffer Format
   :width: 80%
   :align: center

**len_buff**

.. image:: /images/lenbuff_format.png
   :alt: Length Buffer Format
   :width: 80%
   :align: center

To use the regex-VM, you need to:

1. Compile the software regular expression compiler by running the ``make`` command in path ``L1/tests/text/regex_vm/re_compile``.

2. Include the ``xf_re_compile.h`` header in path ``L1/include/sw/xf_data_analytics/text`` and the ``oniguruma.h`` header in path ``L1/tests/text/regex_vm/re_compile/lib/include``.

.. code-block:: cpp

    #include "oniguruma.h"
    #include "xf_re_compile.h"

3. Compile your regular expression by calling ``xf_re_compile``.

.. code-block:: cpp

    // Number of instructions tranlated from the pattern
    unsigned int instr_num = 0;
    // Number of character classes in the pattern
    unsigned int cclass_num = 0;
    // Number of capturing groups in the pattern
    unsigned int cpgp_num = 0;
    // Bit set map
    unsigned int* bitset = new unsigned int[8 * CCLASS_NM];
    // Configuration buffer
    uint64_t* cfg_buff = aligned_alloc<uint64_t>(INSTRUC_SIZE);
    // Suppose 1k bytes is long enough for names of each capturing group
    uint8_t* cpgp_name_val = aligned_alloc<uint8_t>(1024);
    // Suppose the number of capturing groups is less than 20
    uint32_t* cpgp_name_offt = aligned_alloc<uint32_t>(20);
    // Leave 2 64-bit space for configuration headers
    int r = xf_re_compile(pattern, bitset, cfg_buff + 2, &instr_num, &cclass_num, &cpgp_num, cpgp_name_val, cpgp_name_offt);

    // Print a name table for all of the capturing groups
    printf("Name Table\n");
    for (int i = 0; i < cpgp_num; i++) {
        printf("Group-%d: ", i);
        for (int j = 0; j < cpgp_name_offt[i + 1] - cpgp_name_offt[i]; j++) {
            printf("%c", cpgp_name_val[j + cpgp_name_offt[i]]);
        }
        printf("\n");
    }

4. Check the return value to see if it is a valid pattern and supported by the hardware VM. ``ONIG_NORMAL`` is returned if the pattern is valid, and ``XF_UNSUPPORTED_OPCODE`` is returned if it is not supported currently.

.. code-block:: cpp

    if (r != XF_UNSUPPORTED_OPCODE && r == ONIG_NORMAL) {
        // Prepare the buffers and call reEngine for acceleration here
    }

5. Once the regular expression is verified as a supported pattern, you can prepare the input buffers and get the results by:

.. code-block:: cpp
    // Function for writing one line of log to the corresponding buffers
    int writeOneLine (uint64_t* msg_buff, uint16_t* len_buff, unsigned int& offt, unsigned int& msg_nm, std::string& line) {
        typedef union {
            char c_a[8];
            uint64_t d;
        } uint64_un;
        unsigned int sz = line.size();
        if (sz > 4088) {
            printf("Message length exceeds the max limitation\n");
            return 0;
        }
        if ((zs + 7) / 8 + offt > MAX_MSG_SZ || msg_nm > MAX_MSG_NM) {
            printf("Input log size exceeds supported max size\n");
            return -1;
        } else {
            // transform the input char sequence into individual 64-bit blocks and put them into msg_buff
            for (unsigned int i = 0; i < (sz + 7) / 8; i++) {
                uint64_un out;
                for (unsigned int j = 0; j < 8; j++) {
                    if (i * 8 + j < sz) {
                        out.c_a[j] = line[i * 8 + j];
                    } else {
                        out.c_a[j] = ' ';
                    }
                }
                msg_buff[offt++] = out.d;
            }
            // save the length of current line in bytes
            len_buff[msg_nm++] = sz;
            return 0;
        }
    }

.. code-block:: cpp

    // Header for reEngine
    #include "re_engine_kernel.hpp"
    // Header for reading log file as std::string
    #include <iostream>
    #include <fstream>
    #include <string.h>

    // Total number of configuration blocks
    // leave 2 blocks for configuration header
    unsigned int cfg_nm = 2 + instr_num;
    // Message buffer (64-bit width for full utilizing the 2 memory ports of BRAMs)
    uint64_t* msg_buff = aligned_alloc<uint64_t>(MAX_MSG_SZ);
    // Length buffer
    uint16_t* len_buff = aligned_alloc<uint16_t>(MAX_MSG_NM);
    // Append bit-set map to the tail of instruction list
    for (unsigned int i = 0; i < cclass_num * 4; i++) {
        uint64_t tmp = bitset[i * 2 + 1];
        tmp = tmp << 32;
        tmp += bitset[i * 2];
        cfg_buff[cfg_nm++] = tmp;
    }
    // Set configuration header accordingly
    typedef union {
        struct {
            uint32_t instr_nm;
            uint16_t cc_nm;
            uint16_t gp_nm;
        } head_st;
        uint64_t d;
    } cfg_info;
    cfg_info cfg_h;
    cfg_h.head_st.instr_nm = instr_num;
    cfg_h.head_st.cc_nm = cclass_num;
    cfg_h.head_st.gp_nm = cpgp_num;
    cfg_buff[0] = cfg_nm;
    cfg_buff[1] = cfg_h.d;
    // String of each line in the log
    std::string line;
    // We provide a 5k line apache log
    std::ifstream log_file(log_data/access_5k.log);
    if (log_file.is_open()) {
        // Read the apache log line-by-line
        while (getline(log_file, line)) {
            if (line.size() > 0) {
                if (writeOneLine(msg_buff, len_buff, offt, msg_nm, line) != 0) {
                    return -1;
                }
            }
        }
        // Set the header of message buffer (number of message blocks in 64-bit)
        msg_buff[0] = offt;
        // Set the header of length buffer (concatenate the first 2 blocks, it presents the total number of messages in msg_buff)
        len_buff[0] = msg_nm / 65536;
        len_buff[1] = msg_nm % 65536;
    } else {
        printf("Opening input log file failed.\n");
        return -1;
    }
    // Result buffer
    uint32_t* out_buff = aligned_alloc<uint32_t>((cpgp_num + 1) * msg_nm);
    // Call reEngine
    reEngineKernel(reinterpret_cast<ap_uint<64>*>(cfg_buff), reinterpret_cast<ap_uint<64>*>(msg_buff), reinterpret_cast<ap_uint<16>*>(len_buff), reinterpret_cast<ap_uint<32>*>(out_buff));
    
The match flag and offset addresses for each capturing group are presented in ``out_buff`` with the format shown in the following figure:

**out_buff**

.. image:: /images/outbuff_format.png
   :alt: Result Buffer Format
   :width: 80%
   :align: center


Implemention
============

Unlike common L1 primitives, the hardware regex-VM is not using a stream-based interface due to the characteristic of the virtual machine (VM) approach. Thus, the dataflow tricks utilized in the kernel level cannot be like the one commonly used in the L2 implementation. Detailed explanations are provided here.

For the common stream-based dataflow, you want the interfaces between modules as FIFOs, and this is the reason why you find that the interfaces of L1 primitives are usually defined as ``hls::stream``. By implementing the interface as FIFOs, these connected modules works as systolic array when dataflow region applied to them. A consumer in the stream-based dataflow region goes on only if the producer before it gives a data to its input FIFO. Thus, it is not necessary for you to switch the module on or off manually.

However, for those primitives with buffer interfaces like regex-VM, it comes to a ping-pong buffer structure when the dataflow pragma is applied to it. Because you have no empty signal as FIFO provided in the buffer-based dataflow region, you have to control the modules manually to avoid malfunctioning on the pipeline. This can be explained as follows; suppose you have an input log which the messages within it needs `N` rounds to be all fed into the buffers of each PU in ``reEngineKernel``:

+-----------------------+-----------+-----------+-----------+-------+-------------+-----------+-------------+
| Operation             | Round 0   | Round 1   | Round 2   | ...   | Round N - 1 | Round N   | Round N + 1 |
+-----------------------+-----------+-----------+-----------+-------+-------------+-----------+-------------+
| Feeding buffers       | Yes       | Yes       | Yes       | ...   | Yes         | No        | No          |
+-----------------------+-----------+-----------+-----------+-------+-------------+-----------+-------------+
| Executing matcher     | No        | Yes       | Yes       | ...   | Yes         | Yes       | No          |
+-----------------------+-----------+-----------+-----------+-------+-------------+-----------+-------------+
| Collecting results    | No        | No        | Yes       | ...   | Yes         | Yes       | Yes         |
+-----------------------+-----------+-----------+-----------+-------+-------------+-----------+-------------+

You will not have a `N + 2` round, as the whole pipeline finished right after round `N + 1`.

.. NOTE::
    This kernel implementation is very similar to the working pattern of the common pipelined host as provided in the other libraries; take this as a possible dataflow solution for integrating those primitives with buffer interfaces to L2 kernels. By doing so, you can achieve a reasonable acceleration ratio on hardware with the price of sacrificing double buffer storage.

Profiling
=========

The hardware resource utilizations of reEngine (the one given in L2 test as an example on the Alveo U200) is shown in the following table (performance optimized version at **FMax = 200 MHz**).

+----------------+--------+-----------+--------+--------+--------+
| Item           |  LUT   |   REG     |  BRAM  | URAM   | DSP    |
+----------------+--------+-----------+--------+--------+--------+
| reEngine       | 499292 |  341196   | 792    | 576    |  36    |
+                +--------+-----------+--------+--------+--------+
| (U200)         | 54.94% |  17.27%   | 70.53% | 60.00% | 0.53%  |
+----------------+--------+-----------+--------+--------+--------+

The number of PUs on each SLR is listed in the following table:

+---------+---------------+
| SLR     | Number of PUs |
+---------+---------------+
| 0       | 5             |
+---------+---------------+
| 1       | 2             |
+---------+---------------+
| 2       | 5             |
+---------+---------------+

Therefore, the kernel throughput should be:

**Throughput = 12 * 387 Mb/s = 4.64 Gb/s**
