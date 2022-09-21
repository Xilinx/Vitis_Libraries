#
# Copyright 2019-2021 Xilinx, Inc.
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
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical load_cfg_and_scan_*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR1 [get_cells -hierarchical filter_ongoing_*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical write_table_out_*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_U0*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_47*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR1 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_48*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR1 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_49*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR1 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_50*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR1 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_51*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR1 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_52*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR1 [get_cells -hierarchical build_probe_core_wrapper_1_3_61_17_128_64_64_24_64_53*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_U0*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_61*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_62*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_63*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_64*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_65*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_66*] -clear_locs
add_cells_to_pblock pblock_dynamic_SLR0 [get_cells -hierarchical adapt_size_8_128_64_64_64_3_256_67*] -clear_locs
