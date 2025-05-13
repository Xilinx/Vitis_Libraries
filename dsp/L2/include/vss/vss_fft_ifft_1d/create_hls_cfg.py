#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
import sys
import configparser
import string


def hls_params(cur_dir, root_dir, kernel, params_file, tmpl_file):
    parser = configparser.ConfigParser()
    with open(f"{cur_dir}/{params_file}") as stream:
        parser.read_string(
            "[top]\n" + stream.read()
        )  # python's configparser complains about headerless configurations in cfg files. This is a trick to get around the issue
    hls_config_dict = dict(parser.items("APP_PARAMS"))
    suffix = "_config.cfg"
    cfg_file = kernel + suffix
    kernel_dict = {"ROOT_DIR": str(root_dir), "KERNEL_NAME": str(kernel)}
    full_dict = {**kernel_dict, **hls_config_dict}
    print(full_dict)

    with open(f"{root_dir}/L2/include/vss/vss_fft_ifft_1d/{tmpl_file}", "r") as fr:
        t = fr.read()
    with open(f"{cur_dir}/{cfg_file}", "w") as f:
        f.write(string.Template(t).substitute(**full_dict))


CUR_DIR = sys.argv[1]
ROOT_DIR = sys.argv[2]
HLS_KERNEL_NAME = sys.argv[3]
PARAMS_FILE = sys.argv[4]
if len(sys.argv) > 5:
    TMPL_FILE = sys.argv[5]
else:
    TMPL_FILE = "hls_config.tmpl"

hls_params(CUR_DIR, ROOT_DIR, HLS_KERNEL_NAME, PARAMS_FILE, TMPL_FILE)
