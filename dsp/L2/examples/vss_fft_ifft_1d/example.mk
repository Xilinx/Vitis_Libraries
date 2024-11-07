#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

vss:
	make -f ${DSPLIB_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d.mk clean vss HELPER_CUR_DIR=./ HELPER_ROOT_DIR=${DSPLIB_ROOT_DIR} PARAMS_CFG=my_params.cfg

example_xclbin:
	v++ -c -DNSTREAM=4 -DPOINT_SIZE=4096 -DNITER=4 -t hw_emu --platform ${PLATFORM} --save-temps   -I ${DSPLIB_ROOT_DIR}//xf_dsp/L1/include/hw -k s2mm_wrapper -o s2mm_wrapper.xo ${DSPLIB_ROOT_DIR}/L1/tests/hw/s2mm/s2mm.cpp
	v++ -c -DNSTREAM=4 -DPOINT_SIZE=4096 -DNITER=4 -t hw_emu --platform ${PLATFORM} --save-temps   -I ${DSPLIB_ROOT_DIR}//xf_dsp/L1/include/hw -k mm2s_wrapper -o mm2s_wrapper.xo ${DSPLIB_ROOT_DIR}/L1/tests/hw/mm2s/mm2s.cpp
	v++ -l -g -t hw_emu --platform ${PLATFORM} --config system.cfg  -o kernel_pkg.xsa mm2s_wrapper.xo s2mm_wrapper.xo vss_fft_ifft_1d/vss_fft_ifft_1d.vss

example_host:
	$(XILINX_VITIS)/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-g++ -o host.elf host.cpp --sysroot=$(SYSROOT)  -I$(SYSROOT)/usr/include/xrt -I/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label   -I -I${XILINX_VITIS}/aietools/include  -D __PS_ENABLE_AIE__  -I ${DSPLIB_ROOT_DIR}/L2/include/aie -I ${DSPLIB_ROOT_DIR}/xf_dsp/L1/include/aie -I ${DSPLIB_ROOT_DIR}//xf_dsp/L1/src/aie -I ${DSPLIB_ROOT_DIR}/xf_dsp/L1/tests/aie/inc -I ${DSPLIB_ROOT_DIR}//xf_dsp/L1/tests/aie/src -I PROJECT -I ${DSPLIB_ROOT_DIR}/xf_dsp/L1/include/hw -DPOINT_SIZE=4096  -DNITER=4   -DSSR=4 -DTT_DATA=cint32  -pthread -L$(SYSROOT)/usr/lib -Wl,--as-needed -lxilinxopencl -lxrt_coreutil  -L ${XILINX_VITIS}/aietools/lib/aarch64.o  -ladf_api_xrt

example_sd_card:
	emconfigutil --platform ${PLATFORM} --od ./
	v++ -t hw_emu --platform ${PLATFORM} -o kernel.xclbin -p kernel_pkg.xsa libadf.a --package.defer_aie_run --package.out_dir package_hw_emu --package.rootfs ${SYSROOT}/../../rootfs.ext4 --package.kernel_image ${SYSROOT}/../../Image --package.boot_mode sd    --package.sd_file run_script.sh   --package.sd_file host.elf   --package.sd_file emconfig.json --package.sd_file data/input_front.txt --package.sd_file data/ref_output.txt

example_run:
	./package_hw_emu/launch_hw_emu.sh -no-reboot -run-app run_script.sh
	grep "TEST PASSED, RC=0" ./package_hw_emu/qemu_output.log || exit 1


all: vss example_xclbin example_host example_sd_card example_run

help:
	echo "Makefile Usage:"
	echo "  make -f example.mk all PLATFORM=<path/to/platform> DSPLIB_ROOT_DIR=<path/to/dsp/library/root>"
	echo "      Command to generate the full vss according to the parameters defined in my_params.cfg. Required arguments are PLATFORM and DSPLIB_ROOT_DIR"
	echo ""
	