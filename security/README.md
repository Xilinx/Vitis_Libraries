# Vitis Security Library

Vitis Security Library is an open-sourced Vitis library written in C++ for accelerating security applications in a variety of use cases.

It now covers one level of acceleration: the primitive level. In this level, it provides optimized hardware implementation of most common relational security algorithms.
Such as, Synmmetric Block Cipher, Symmetirc Stream Cipher, Asymmetric Cryptography, Cipher Modes of Operations, Message Authentication Code, and Hash Function.

Since all the primitive code is developed in HLS C++ with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or assemble property logic.
Benchmarks of 4 different acceleration applications are also provided with the library for easy on-boarding and comparison.

[Comprehensive documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/security/index.html)

## Benchmark Result

A list of Vitis projects can be found `L1/benchmarks`. They are provided to help users to evaluate the performance of most critical primitives. For further detials, please refer to [benchmark result](https://docs.xilinx.com/r/en-US/Vitis_Libraries/security/benchmark.html).

## Library Contents

| Library Class    | Description | Layer |
|------------------|-------------|-------|
| aesEnc | implementation of AES block cipher encrpytion part | L1 |
| aesDec | implementation of AES block cipher decrpytion part | L1 |
| rsa | implementation of RSA encryption / decrpytion part | L1 |

| Library Function | Description | Layer |
|------------------|-------------|-------|
| desEncrypt | desEncrypt is the basic function for encrypt one block with one cipher key using DES | L1 |
| desDecrypt | desDecrypt is the basic function for decrypt one block with one cipher key using DES | L1 |
| des3Encrypt | des3Encrypt is the basic function for encrypt one block with three cipher keys using 3DES | L1 |
| des3Decrypt | des3Decrypt is the basic function for decrypt one block with three cipher keys using 3DES | L1 |
| desCbcEncrypt | desCbcEncrypt is CBC encryption mode with DES single block cipher | L1 |
| desCbcDecrypt | desCbcDecrypt is CBC decryption mode with DES single block cipher | L1 |
| aes128CbcEncrypt | aes128CbcEncrypt is CBC encryption mode with AES-128 single block cipher | L1 |
| aes128CbcDecrypt | aes128CbcDecrypt is CBC decryption mode with AES-128 single block cipher | L1 |
| aes192CbcEncrypt | aes192CbcEncrypt is CBC encryption mode with AES-192 single block cipher | L1 |
| aes192CbcDecrypt | aes192CbcDecrypt is CBC decryption mode with AES-192 single block cipher | L1 |
| aes256CbcEncrypt | aes256CbcEncrypt is CBC encryption mode with AES-256 single block cipher | L1 |
| aes256CbcDecrypt | aes256CbcDecrypt is CBC decryption mode with AES-256 single block cipher | L1 |
| aes128CcmEncrypt | aes128CcmEncrypt is CCM encryption mode with AES-128 single block cipher | L1 |
| aes128CcmDecrypt | aes128CcmDecrypt is CCM decryption mode with AES-128 single block cipher | L1 |
| aes192CcmEncrypt | aes192CcmEncrypt is CCM encryption mode with AES-192 single block cipher | L1 |
| aes192CcmDecrypt | aes192CcmDecrypt is CCM decryption mode with AES-192 single block cipher | L1 |
| aes256CcmEncrypt | aes256CcmEncrypt is CCM encryption mode with AES-256 single block cipher | L1 |
| aes256CcmDecrypt | aes256CcmDecrypt is CCM decryption mode with AES-256 single block cipher | L1 |
| desCfb1Encrypt | desCfb1Encrypt is CFB1 encryption mode with DES single block cipher | L1 |
| desCfb1Decrypt | desCfb1Decrypt is CFB1 decryption mode with DES single block cipher | L1 |
| aes128Cfb1Encrypt | aes128Cfb1Encrypt is CFB1 encryption mode with AES-128 single block cipher | L1 |
| aes128Cfb1Decrypt | aes128Cfb1Decrypt is CFB1 decryption mode with AES-128 single block cipher | L1 |
| aes192Cfb1Encrypt | aes192Cfb1Encrypt is CFB1 encryption mode with AES-192 single block cipher | L1 |
| aes192Cfb1Decrypt | aes192Cfb1Decrypt is CFB1 decryption mode with AES-192 single block cipher | L1 |
| aes256Cfb1Encrypt | aes256Cfb1Encrypt is CFB1 encryption mode with AES-256 single block cipher | L1 |
| aes256Cfb1Decrypt | aes256Cfb1Decrypt is CFB1 decryption mode with AES-256 single block cipher | L1 |
| desCfb8Encrypt | desCfb8Encrypt is CFB8 encryption mode with DES single block cipher | L1 |
| desCfb8Decrypt | desCfb8Decrypt is CFB8 decryption mode with DES single block cipher | L1 |
| aes128Cfb8Encrypt | aes128Cfb8Encrypt is CFB8 encryption mode with AES-128 single block cipher | L1 |
| aes128Cfb8Decrypt | aes128Cfb8Decrypt is CFB8 decryption mode with AES-128 single block cipher | L1 |
| aes192Cfb8Encrypt | aes192Cfb8Encrypt is CFB8 encryption mode with AES-192 single block cipher | L1 |
| aes192Cfb8Decrypt | aes192Cfb8Decrypt is CFB8 decryption mode with AES-192 single block cipher | L1 |
| aes256Cfb8Encrypt | aes256Cfb8Encrypt is CFB8 encryption mode with AES-256 single block cipher | L1 |
| aes256Cfb8Decrypt | aes256Cfb8Decrypt is CFB8 decryption mode with AES-256 single block cipher | L1 |
| desCfb128Encrypt | desCfb128Encrypt is CFB128 encryption mode with DES single block cipher | L1 |
| desCfb128Decrypt | desCfb128Decrypt is CFB128 decryption mode with DES single block cipher | L1 |
| aes128Cfb128Encrypt | aes128Cfb128Encrypt is CFB128 encryption mode with AES-128 single block cipher | L1 |
| aes128Cfb128Decrypt | aes128Cfb128Decrypt is CFB128 decryption mode with AES-128 single block cipher | L1 |
| aes192Cfb128Encrypt | aes192Cfb128Encrypt is CFB128 encryption mode with AES-192 single block cipher | L1 |
| aes192Cfb128Decrypt | aes192Cfb128Decrypt is CFB128 decryption mode with AES-192 single block cipher | L1 |
| aes256Cfb128Encrypt | aes256Cfb128Encrypt is CFB128 encryption mode with AES-256 single block cipher | L1 |
| aes256Cfb128Decrypt | aes256Cfb128Decrypt is CFB128 decryption mode with AES-256 single block cipher | L1 |
| aes128CtrEncrypt | aes128CtrEncrypt is CTR encryption mode with AES-128 single block cipher | L1 |
| aes128CtrDecrypt | aes128CtrDecrypt is CTR decryption mode with AES-128 single block cipher | L1 |
| aes192CtrEncrypt | aes192CtrEncrypt is CTR encryption mode with AES-192 single block cipher | L1 |
| aes192CtrDecrypt | aes192CtrDecrypt is CTR decryption mode with AES-192 single block cipher | L1 |
| aes256CtrEncrypt | aes256CtrEncrypt is CTR encryption mode with AES-256 single block cipher | L1 |
| aes256CtrDecrypt | aes256CtrDecrypt is CTR decryption mode with AES-256 single block cipher | L1 |
| desEcbEncrypt | desEcbEncrypt is ECB encryption mode with DES single block cipher | L1 |
| desEcbDecrypt | desEcbDecrypt is ECB decryption mode with DES single block cipher | L1 |
| aes128EcbEncrypt | aes128EcbEncrypt is ECB encryption mode with AES-128 single block cipher | L1 |
| aes128EcbDecrypt | aes128EcbDecrypt is ECB decryption mode with AES-128 single block cipher | L1 |
| aes192EcbEncrypt | aes192EcbEncrypt is ECB encryption mode with AES-192 single block cipher | L1 |
| aes192EcbDecrypt | aes192EcbDecrypt is ECB decryption mode with AES-192 single block cipher | L1 |
| aes256EcbEncrypt | aes256EcbEncrypt is ECB encryption mode with AES-256 single block cipher | L1 |
| aes256EcbDecrypt | aes256EcbDecrypt is ECB decryption mode with AES-256 single block cipher | L1 |
| aes128GcmEncrypt | aes128GcmEncrypt is GCM encryption mode with AES-128 single block cipher | L1 |
| aes128GcmDecrypt | aes128GcmDecrypt is GCM decryption mode with AES-128 single block cipher | L1 |
| aes192GcmEncrypt | aes192GcmEncrypt is GCM encryption mode with AES-192 single block cipher | L1 |
| aes192GcmDecrypt | aes192GcmDecrypt is GCM decryption mode with AES-192 single block cipher | L1 |
| aes256GcmEncrypt | aes256GcmEncrypt is GCM encryption mode with AES-256 single block cipher | L1 |
| aes256GcmDecrypt | aes256GcmDecrypt is GCM decryption mode with AES-2562 single block cipher | L1 |
| desOfbEncrypt | desOfbEncrypt is OFB encryption mode with DES single block cipher | L1 |
| desOfbDecrypt | desOfbDecrypt is OFB decryption mode with DES single block cipher | L1 |
| aes128OfbEncrypt | aes128OfbEncrypt is OFB encryption mode with AES-128 single block cipher | L1 |
| aes128OfbDecrypt | aes128OfbDecrypt is OFB decryption mode with AES-128 single block cipher | L1 |
| aes192OfbEncrypt | aes192OfbEncrypt is OFB encryption mode with AES-192 single block cipher | L1 |
| aes192OfbDecrypt | aes192OfbDecrypt is OFB decryption mode with AES-192 single block cipher | L1 |
| aes256OfbEncrypt | aes256OfbEncrypt is OFB encryption mode with AES-256 single block cipher | L1 |
| aes256OfbDecrypt | aes256OfbDecrypt is OFB decryption mode with AES-256 single block cipher | L1 |
| aes128XtsEncrypt | aes128XtsEncrypt is XTS encryption mode with AES-128 single block cipher | L1 |
| aes128XtsDecrypt | aes128XtsDecrypt is XTS decryption mode with AES-128 single block cipher | L1 |
| aes256XtsEncrypt | aes256XtsEncrypt is XTS encryption mode with AES-256 single block cipher | L1 |
| aes256XtsDecrypt | aes256XtsDecrypt is XTS decryption mode with AES-256 single block cipher | L1 |
| aes128Gmac | The GMAC using AES-128 block cipher | L1 |
| aes192Gmac | The GMAC using AES-192 block cipher | L1 |
| aes256Gmac | The GMAC using AES-256 block cipher | L1 |
| hmac | compute hmac value according to specific hash function and input data | L1 |
| chacha20 | CHACHA20 algorithm implementation | L1 |
| poly1305 | POLY1305 algorithm implementation | L1 |
| rc4 | RC4, also known as ARC4 algorithm implementation | L1 |
| md4 | MD4 algorithm implementation | L1 |
| md5 | MD5 algorithm implementation | L1 |
| sha1 | SHA-1 algorithm implementation | L1 |
| sha224 | SHA-224 algorithm implementation | L1 |
| sha256 | SHA-256 algorithm implementation | L1 |
| sha384 | SHA-384 algorithm implementation | L1 |
| sha512 | SHA-512 algorithm implementation | L1 |
| sha512_t | SHA-512/t algorithm implementation | L1 |
| sha3_224 | SHA-3/224 algorithm implementation | L1 |
| sha3_256 | SHA-3/256 algorithm implementation | L1 |
| sha3_384 | SHA-3/384 algorithm implementation | L1 |
| sha3_512 | SHA-3/512 algorithm implementation | L1 |
| shake128 | SHAKE-128 algorithm implementation | L1 |
| shake256 | SHAKE-256 algorithm implementation | L1 |
| blake2b | BLAKE2B algorithm implementation | L1 |
| keccak_256 | KECCAK-256 algorithm implementation | L1 |
| vdf_evaluate | Evaluate VDF implementation | L1 |
| verifyWesolowski | Verify VDF Wesolowski implementation | L1 |
| verifyPietrzak | Verify VDF Pietrzak implementation | L1 |
| ecdsa_nistp256 | ECDSA with curve nistp256 implementation | L1 |
| ecdsa_secp256k1 | ECDSA with curve secp256k1 implementation | L1 |


## Requirements

### Software Platform

This library is designed to work with Vitis 2022.2, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
[devtoolset-6](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/).

### PCIE Accelerator Card

Hardware modules are designed to work with 16nm Alveo cards. Benchmarks are best tuned for U250, and could be tailored for other devices.
* [Alveo U200](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted)
* [Alveo U250](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted)
* [Alveo U280](https://www.xilinx.com/products/boards-and-kits/alveo/u280.html#gettingStarted)

### Shell Environment

Setup the build environment using the Vitis and XRT scripts:

```console
    $ source <install path>/Vitis/2022.2/settings64.sh
    $ source /opt/xilinx/xrt/setup.sh
```
## Source Files and Application Development
Vitis libraries are organized into L1, L2, and L3 folders, each relating to a different stage of application development.

**L1** :
      Makefiles and sources in L1 facilitate HLS based flow for quick checks. Tasks at this level include:

* Check the functionality of an individual kernel (C-simulation)
* Estimate resource usage, latency, etc. (Synthesis)
* Run cycle accurate simulations (Co-simulation)
* Package as IP and get final resource utilization/timing details (Export RTL)
       
	**Note**:  Once RTL (or XO file after packaging IP) is generated, the Vivado flow is invoked for XCLBIN file generation if required.

**L2** :
       Makefiles and sources in L2 facilitate building XCLBIN file from various sources (HDL, HLS or XO files) of kernels with host code written in OpenCL/XRT framework targeting a device. This flow supports:

* Software emulation to check the functionality
* Hardware emulation to check RTL level simulation
* Build and test on hardware

**L3** :
       Makefiles and sources in L3 demonstrate applications developed involving multiple kernels in pipeline. These Makefiles can be used for executing tasks, as with the L2 Makefiles.
       
## Design Flows

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

Recommended design flows are decribed as follows:

### L1

L1 provides the basic primitives which cover the most common algorithms in security.

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ testbench (typically `algorithm_name.cpp`) prepares the input data, passes them to the design under test, then performs any output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including `CSIM` (high level simulation), `CSYNTH` (high level synthesis to RTL) and `COSIM` (cosimulation between software testbench and generated RTL), `VIVADO_SYN` (synthesis by Vivado), `VIVADO_IMPL` (implementation by Vivado). The flow is launched from the shell by calling `make` with variables set as in the example below:

```console
	$ . /opt/xilinx/xrt/setup.sh
	$ export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
	$ cd L1/tests/specific_algorithm/
	$ make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 PLATFORM=u250_xdma_201830_1 # Only run C++ simulation on U250 card
```

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency and throughput. The output files of interest can be located at the location of the test project where the path name is correlated with the algorithm. i.e. the callable function within the design under test.

To run the Vitis projects for benchmark evaluation and test, you may need the example below:

```console
	$ ./opt/xilinx/xrt/setup.sh
	$ export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
	$ cd L1/benchmarks/specific_algorithm/
	$ # Run hardware emulation for cycle-accurate simulation with the RTL-model
	$ make run TARGET=hw_emu DEIVCE=u250_xdma_201830_1
	$ # Run hardware to generate the desired xclbin binary
	$ make run TARGET=hw DEIVCE=u250_xdma_201830_1
	$ # Delete xclbin and host excutable program
	$ make cleanall
```

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2023, Advanced Micro Devices, Inc.

## Contribution/Feedback

Welcome! Guidelines to be published soon.

