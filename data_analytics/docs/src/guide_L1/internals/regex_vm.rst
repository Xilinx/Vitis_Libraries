.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-regex-VM:

*********************************************
Regular Expression Virtual Machine (regex-VM)
*********************************************

Overview
========

The regex-VM aims at the work of converting unstructured texts, like log files, into structured ones. Therefore, a state-of-art high-throughput matching algorithm for comparing one string with many patterns, like that used in hyperscan, cannot work well in your target context. A VM-based approach is chosen, as it allows you to offer drop-in replacement in popular data transformation tools with regex often written in dialect of Perl, Python, or Ruby.

The regex-VM consists of two parts: a software compiler written in C and a hardware virtual machine (VM) in C++.

1. Software compiler: Compiles any regular expression given into an instruction list along with the corresponding bit-set map, number of instructions/character-classes/capturing-groups, and the name of each capturing group (if specified in input pattern).

2. Hardware VM: Takes the outputs from the compiler mentioned above to construct a practical matcher to match the string given in a message buffer and emits a 2-bit match flag indicating whether the input string is matched with the pattern or an internal stack overflow has happened. Futhermore, if the input string is matched, the offset addresses for each capturing group is provided in the output offset buffer; you can find the sub-strings of interest by picking them out from the whole input string according to the information given in that buffer.


User Guide
==========

The regex-VM is implemented with a VM approach, in which the pattern is translated into a series of instructions of a specialized VM, and the string being matched for drives the instruction jumps. The VM instructions are reused with the compiler from the popular `Oniguruma`_ library, which is the foundation of current Ruby `regex implementation`_, as your base.

.. _`Oniguruma`: https://github.com/kkos/oniguruma.git

.. _`regex implementation`: https://github.com/k-takata/Onigmo

Regex-VM Coverage
-----------------

Due to limited resources and a strict release timeline, in the current release, only the most frequently used OPs are provided in the whole OP list given by the Oniguruma library to support common regular expressions. This can be shown in the following table:

+-----------------------------+----------------+
| OP list                     | Supported      |
+-----------------------------+----------------+
| FINISH                      | NO             |
+-----------------------------+----------------+
| END                         | YES            |
+-----------------------------+----------------+
| STR_1                       | YES            |
+-----------------------------+----------------+
| STR_2                       | YES            |
+-----------------------------+----------------+
| STR_3                       | YES            |
+-----------------------------+----------------+
| STR_4                       | YES            |
+-----------------------------+----------------+
| STR_5                       | YES            |
+-----------------------------+----------------+
| STR_N                       | YES            |
+-----------------------------+----------------+
| STR_MB2N1                   | NO             |
+-----------------------------+----------------+
| STR_MB2N2                   | NO             |
+-----------------------------+----------------+
| STR_MB2N3                   | NO             |
+-----------------------------+----------------+
| STR_MB2N                    | NO             |
+-----------------------------+----------------+
| STR_MB3N                    | NO             |
+-----------------------------+----------------+
| STR_MBN                     | NO             |
+-----------------------------+----------------+
| CCLASS                      | YES            |
+-----------------------------+----------------+
| CCLASS_MB                   | NO             |
+-----------------------------+----------------+
| CCLASS_MIX                  | NO             |
+-----------------------------+----------------+
| CCLASS_NOT                  | YES            |
+-----------------------------+----------------+
| CCLASS_MB_NOT               | NO             |
+-----------------------------+----------------+
| CCLASS_MIX_NOT              | NO             |
+-----------------------------+----------------+
| ANYCHAR                     | YES            |
+-----------------------------+----------------+
| ANYCHAR_ML                  | NO             |
+-----------------------------+----------------+
| ANYCHAR_STAR                | YES            |
+-----------------------------+----------------+
| ANYCHAR_ML_STAR             | NO             |
+-----------------------------+----------------+
| ANYCHAR_STAR_PEEK_NEXT      | NO             |
+-----------------------------+----------------+
| ANYCHAR_ML_STAR_PEEK_NEXT   | NO             |
+-----------------------------+----------------+
| WORD                        | NO             |
+-----------------------------+----------------+
| WORD_ASCII                  | NO             |
+-----------------------------+----------------+
| NO_WROD                     | NO             |
+-----------------------------+----------------+
| NO_WORD_ASCII               | NO             |
+-----------------------------+----------------+
| WORD_BOUNDARY               | NO             |
+-----------------------------+----------------+
| NO_WORD_BOUNDARY            | NO             |
+-----------------------------+----------------+
| WORD_BEGIN                  | NO             |
+-----------------------------+----------------+
| WORD_END                    | NO             |
+-----------------------------+----------------+
| TEXT_SEGMENT_BOUNDARY       | NO             |
+-----------------------------+----------------+
| BEGIN_BUF                   | YES            |
+-----------------------------+----------------+
| END_BUF                     | YES            |
+-----------------------------+----------------+
| BEGIN_LINE                  | YES            |
+-----------------------------+----------------+
| END_LINE                    | YES            |
+-----------------------------+----------------+
| SEMI_END_BUF                | NO             |
+-----------------------------+----------------+
| CHECK_POSITION              | NO             |
+-----------------------------+----------------+
| BACKREF1                    | NO             |
+-----------------------------+----------------+
| BACKREF2                    | NO             |
+-----------------------------+----------------+
| BACKREF_N                   | NO             |
+-----------------------------+----------------+
| BACKREF_N_IC                | NO             |
+-----------------------------+----------------+
| BACKREF_MULTI               | NO             |
+-----------------------------+----------------+
| BACKREF_MULTI_IC            | NO             |
+-----------------------------+----------------+
| BACKREF_WITH_LEVEL          | NO             |
+-----------------------------+----------------+
| BACKREF_WITH_LEVEL_IC       | NO             |
+-----------------------------+----------------+
| BACKREF_CHECK               | NO             |
+-----------------------------+----------------+
| BACKREF_CHECK_WITH_LEVEL    | NO             |
+-----------------------------+----------------+
| MEM_START                   | YES            |
+-----------------------------+----------------+
| MEM_START_PUSH              | YES            |
+-----------------------------+----------------+
| MEM_END_PUSH                | NO             |
+-----------------------------+----------------+
| MEM_END_PUSH_REC            | NO             |
+-----------------------------+----------------+
| MEM_END                     | YES            |
+-----------------------------+----------------+
| MEM_END_REC                 | NO             |
+-----------------------------+----------------+
| FAIL                        | YES            |
+-----------------------------+----------------+
| JUMP                        | YES            |
+-----------------------------+----------------+
| PUSH                        | YES            |
+-----------------------------+----------------+
| PUSH_SUPER                  | NO             |
+-----------------------------+----------------+
| POP                         | YES            |
+-----------------------------+----------------+
| POP_TO_MARK                 | YES            |
+-----------------------------+----------------+
| PUSH_OR_JUMP_EXACT1         | YES            |
+-----------------------------+----------------+
| PUSH_IF_PEEK_NEXT           | NO             |
+-----------------------------+----------------+
| REPEAT                      | YES            |
+-----------------------------+----------------+
| REPEAT_NG                   | NO             |
+-----------------------------+----------------+
| REPEAT_INC                  | YES            |
+-----------------------------+----------------+
| REPEAT_INC_NG               | NO             |
+-----------------------------+----------------+
| EMPTY_CHECK_START           | NO             |
+-----------------------------+----------------+
| EMPTY_CHECK_END             | NO             |
+-----------------------------+----------------+
| EMPTY_CHECK_END_MEMST       | NO             |
+-----------------------------+----------------+
| EMPTY_CHECK_END_MEMST_PUSH  | NO             |
+-----------------------------+----------------+
| MOVE                        | NO             |
+-----------------------------+----------------+
| STEP_BACK_START             | YES            |
+-----------------------------+----------------+
| STEP_BACK_NEXT              | NO             |
+-----------------------------+----------------+
| CUT_TO_MARK                 | NO             |
+-----------------------------+----------------+
| MARK                        | YES            |
+-----------------------------+----------------+
| SAVE_VAL                    | NO             |
+-----------------------------+----------------+
| UPDATE_VAR                  | NO             |
+-----------------------------+----------------+
| CALL                        | NO             |
+-----------------------------+----------------+
| RETURN                      | NO             |
+-----------------------------+----------------+
| CALLOUT_CONTECTS            | NO             | 
+-----------------------------+----------------+
| CALLOUT_NAME                | NO             |
+-----------------------------+----------------+

Therefore, the supported atomic regular expressions and their corresponding descriptions should be:

+-------------------+------------------------------------------------------------------------------------------------------+
| Regex             | Description                                                                                          |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``^``             | Asserts position at the start of a line.                                                             |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``$``             | Asserts position at the end of a line.                                                               |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\A``            | Asserts position at start of the string.                                                             |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\z``            | Asserts position at the end of the string.                                                           |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\ca``           | Matches the control sequence ``CTRL+A``.                                                             |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\C``            | Matches one data unit, even in UTF mode (best avoided).                                              |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\c\\``          | Matches the control sequence ``CTRL+\``.                                                             |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\s``            | Matches any whitespace character (equal to ``[\r\n\t\f\v ]``).                                       |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\S``            | Matches any non-whitespace character (equal to ``[^\r\n\t\f\v ]``).                                  |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\d``            | Matches a digit (equal to ``[0-9]``.)                                                                |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\D``            | Matches any character that's not a digit (equal to ``[^0-9]``).                                      |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\h``            | Matches any horizontal whitespace character (equal to ``[[:blank:]]``).                              |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\H``            | Matches any character that is not a horizontal whitespace character.                                 |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\w``            | Matches any word character (equal to ``[a-zA-Z0-9_]``).                                              |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\W``            | Matches any non-word character (equal to ``[^a-zA-Z0-9_]``).                                         |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\^``            | Matches the character ``^`` literally.                                                               |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\$``            | Matches the character ``$`` literally.                                                               |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\N``            | Matches any non-newline character.                                                                   |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\g'0'``         | Recurses the 0th subpattern.                                                                         |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\o{101}``       | Matches the character ``A`` with index with ``101(oct)``.                                            |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\x61``          | Matches the character ``a (hex 61)`` literally.                                                      |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\x{1 2}``       | Matches ``1 (hex)`` or ``2 (hex)``.                                                                  |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``\17``           | Matches the character ``oct 17`` literally.                                                          |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``abc``           | Matches the ``abc`` literally.                                                                       |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``.``             | Matches any character (except for line terminators).                                                 |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``|``             | Alternative.                                                                                         |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``[^a]``          | Match a single character not present in the following list.                                          |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``[a-c]``         | Matches ``a``, ``b``, or ``c``.                                                                      |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``[abc]``         | Matches ``a``, ``b``, or ``c``.                                                                      |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``[:upper:]``     | Matches a uppercase letter ``[A-Z]``.                                                                |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a?``            | Matches the ``a`` zero or one time (**greedy**).                                                     |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a*``            | Matches ``a`` between zero and unlimited times (**greedy**).                                         |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a+``            | Matches ``a`` between one and unlimited times (**greedy**).                                          |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a??``           | Matches ``a`` between zero and one times (**lazy**).                                                 |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a*?``           | Matches ``a`` between zero and unlimited times (**lazy**).                                           |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a+?``           | Matches ``a`` between one and unlimited times (**lazy**).                                            |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a{2}``          | Matches ``a`` exactly two times.                                                                     |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a{0,}``         | Matches ``a`` between zero and unlimited times.                                                      |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``a{1,2}``        | Matches ``a`` one or two times.                                                                      |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``{,}``           | Matches ``{,}`` literally.                                                                           |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``(?#blabla)``    | Comment ``blabla``.                                                                                  |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``(a)``           | Capturing group; matches ``a`` literally.                                                            |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``(?<name1> a)``  | Named capturing group ``name1``; matches ``a`` literally.                                            |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``(?:)``          | Non-capturing group.                                                                                 |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``(?i)``          | Match the remainder of the pattern with the following effective flags: gmi (i modifier: insensitive).|
+-------------------+------------------------------------------------------------------------------------------------------+
| ``(?<!a)z``       | Matches any occurrence of ``z`` that is not preceded by ``a`` (negative look-behind).                |
+-------------------+------------------------------------------------------------------------------------------------------+
| ``z(?!a)``        | Match any occurrence of ``z`` that is not followed by ``a`` (negative look-ahead).                    |
+-------------------+------------------------------------------------------------------------------------------------------+

.. ATTENTION::
    1. Supported encoding method in current release is ASCII (extended ASCII codes are excluded).
    2. Nested repetition is not supported.

Regex-VM Usage
--------------

Before instantiating the hardware VM, you have to pre-compile your regular expression using the software compiler mentioned above first to check if the pattern is supported by the hardware VM. The compiler will give a ``XF_UNSUPPORTED_OPCODE` error code`` if the pattern is not supported. A ``ONIG_NORMAL`` pass code along with the configurations (including instruction list, bit-set map, etc.) will be given if the input is a valid pattern. Then, you should pass these configurations and the input message with its corresponding length in bytes to the hardware VM to trigger the matching process. The hardware VM will judge whether the input message is matched and provide the offset addresses for each capturing the group in an offset buffer.

It is important to know that only the internal stack buffer is hold in the hardware VM; allocate memories for bit-set map, instruction buffer, message buffer accordingly, and offset buffer respectively outside the hardware instantiation.

For the internal stack, its size is decided by the template parameter of the hardware VM. Because the storage resource it uses is URAM, the ``STACK_SIZE`` should better be set to be a multiple of 4096 for not wasting the space of individual URAM block. Moreover, it is critical to choose the internal stack size wisely as the hardware VM will overflow if the size is too small or no URAMs will be available on board for you to instantiate more PUs to improve the throughput.

**Code Example**

The following section gives a usage example for using regex-VM in C++ based HLS design.

To use the regex-VM you need to:

1. Compile the software regular expression compiler by running the ``make`` command in path ``L1/tests/text/regex_vm/re_compile``.

2. Include the ``xf_re_compile.h`` header in path ``L1/include/sw/xf_data_analytics/text`` and the ``oniguruma.h`` header in path ``L1/tests/text/regex_vm/re_compile/lib/include``.

.. code-block:: cpp

    #include "oniguruma.h"
    #include "xf_re_compile.h"

3. Compile your regular expression by calling ``xf_re_compile``.

.. code-block:: cpp

    int r = xf_re_compile(pattern, bitset, instr_buff, instr_num, cclass_num, cpgp_num, NULL, NULL);

4. Check the return value to see if its a valid pattern and supported by hardware VM. ``ONIG_NORMAL`` is returned if the pattern is valid, and ``XF_UNSUPPORTED_OPCODE`` is returned if it is not currently supported.

.. code-block:: cpp

    if (r != XF_UNSUPPORTED_OPCODE && r == ONIG_NORMAL) {
        // calling hardware VM here for acceleration
    }

5. Once the regular expression is verified as a supported pattern, you can call hardware VM to match any message you want by:

.. code-block:: cpp

    // for data types used in VM
    #include "ap_int.h"
    // header for hardware VM implementation
    #include "xf_data_analytics/text/regexVM.hpp"

    // allocate memory for bit-set map
    unsigned int bitset[8 * cclass_num];
    // allocate memory for instruction buffer (derived from software compiler)
    uint64_t instr_buff[instr_num];
    // allocate memory for message
    ap_uint<32> msg_buff[MESSAGE_SIZE];
    // set up input message buffer according to input string
    unsigned str_len = strlen((const char*)in_str);
    for (int i = 0; i < (str_len + 3) / 4;  i++) {
        for (int k = 0; k < 4; k++) {
            if (i * 4 + k < str_len) {
                msg_buff[i].range((k + 1) * 8 - 1, k * 8) = in_str[i * 4 + k];
            } else {
                // pad white-space at the end
                msg_buff[i].range((k + 1) * 8 - 1, k * 8) = ' ';
            }
        }
    }
    // allocate memory for offset addresses for each capturing group
    uint16_t offset_buff[2 * (cpgp_num + 1)];
    // initialize offset buffer
    for (int i = 0; i < 2 * CAP_GRP_NUM; i++) {
        offset_buff[i] = -1;
    }
    ap_uint<2> match = 0;
    // call for hardware acceleration (basic hardware VM implementation)
    xf::data_analytics::text:regexVM<STACK_SIZE>((ap_uint<32>*)bitset, (ap_uint<64>*)instr_buff, msg_buff, str_len, match, offset_buff);
    // or call for hardware acceleration (performance optimized hardware VM implementation)
    xf::data_analytics::text:regexVM_opt<STACK_SIZE>((ap_uint<32>*)bitset, (ap_uint<64>*)instr_buff, msg_buff, str_len, match, offset_buff);

The match flag and offset addresses for each capturing group are presented in ``match`` and ``offset_buff``, respectively with the format shown in the following tablesS.

Truth table for the 2-bit output ``match`` flag of hardware VM:

+-------+-------------------------+
| Value | Description             |
+-------+-------------------------+
| 0     | mismatched              |
+-------+-------------------------+
| 1     | matched                 |
+-------+-------------------------+
| 2     | internal stack overflow |
+-------+-------------------------+
| 3     | reserved for future use |
+-------+-------------------------+

Arrangement of the offset ``offsetBuff`` buffer:

+---------+---------------------------------------------+
| Address | Description                                 |
+---------+---------------------------------------------+
| 0       | Start position of the whole matched string  |
+---------+---------------------------------------------+
| 1       | End position of the whole matched string    |
+---------+---------------------------------------------+
| 2       | Start position of the first capturing group |
+---------+---------------------------------------------+
| 3       | End position of the first capturing group   |
+---------+---------------------------------------------+
| 4       | Start position of the second capturing group|
+---------+---------------------------------------------+
| 5       | End position of the second capturing group  |
+---------+---------------------------------------------+
| ...     | ...                                         |
+---------+---------------------------------------------+

Implemention
============

If you go into the details of the implementation of the hardware VM, you might find even the basic version of hardware VM is significantly different from the one in Oniguruma, let alone the performance optimized one. Thus, this section is especially for developers who want to add more OPs to the VM by themselves or who are extremely interested in the design.

The first thing you want to conquer will be the software compiler. Once you have a full understanding of a specific OP in Oniguruma, you have to add it to the corresponding instruction with the format acceptable for the hardware VM. The 64-bit instruction format for communication between the software compiler and hardware VM can be explained like this:

.. image:: /images/instruction_format.png
   :alt: Instruction Format
   :width: 80%
   :align: center

Then, if the OP you want to add is related to a jump/push operation on the OP address, the absolute address must be provided at the first while-loop in the source code of the software compiler for calculation of the address which will be put into the instruction list later. The rest of the information related to this OP and the calculated address should be packed into one instruction at the second while-loop. So far, the software compiler part is done.

Location of the source of the software compiler: ``L1/src/sw/xf_re_compile.c``

Finally, add the corresponding logic to the hardware VM based on your understanding of the OP, and test it accordingly. Once the test passed, you might start optimizing the implementation which is extremely challenging and tricky.

The following have been done for optimizing the hardware VM:

1. Simplify the internal logic for each OP you added as mush as you can.

2. Merge the newly added OP into another, if possible, to let them share the same logic.

3. Offload runtime calculations to a software compiler for pre-calculation, if possible, to improve the runtime performance.

4. Separate the data flow and control flow; do pre-fetch and post-store operations to improve memory access efficiency.

5. Resolve the read-and-write dependency of the on-chip RAMs by caching the intermediate data in registers to avoid unnecessary accesses.

6. Execute a predict (second) instruction in each iteration to accelerate the process under specific circumstances (performance optimized version executes two instructions/three cycles).

.. NOTE::
    For the following scenarios, the predict instruction will not be executed:

    1. Read/write the internal stack simultaneously.

    2. OP for the second instruction is any_char_star, pop_to_mark, or mem_start_push.

    3. Jump on OP address happened in first instruction.

    4. Read/write the offset buffer simultaneously.

    5. Pointer for the input string moves in the first instruction and the second instruction goes into the OP which needs character comparision.

    6. Write the offset buffer simultaneously.


Profiling
=========

The hardware resource utilization of hardware VM is shown in the following table (performance optimized version at FMax = 352 MHz).

+----------------+-------+------+--------+--------+------+-----+-----+
| Primitive      | CLB   |  LUT |   FF   |  BRAM  | URAM | DSP | SRL |
+----------------+-------+------+--------+--------+------+-----+-----+
| hardware VM    | 305   | 1690 |  973   |    0   |  4   |  0  | 0   |
+----------------+-------+------+--------+--------+------+-----+-----+

