# 
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
import os
import sys
import numpy as np
from scipy.signal import hilbert
import matplotlib.pyplot as plt

# physical parameter
example_1_speed_sound = 1540.0 # m/s
example_1_focal_point_z = 0.02 # m
example_1_num_sample = 2048
example_1_num_upsample = 4
example_1_num_line = 41
example_1_directions = [0.0, 0.0, 0.0000077, 0.0]
example_1_start_positions = [0.0, 0.0, 0.0076923, 0.0]

def read_column_from_txt(file_path):
    column_data = []
    with open(file_path, 'r') as file:
        for line in file:
            value = float(line.strip())
            column_data.append(value)

    return np.array(column_data)

# image grid
def fun_gen_grid():
    x_start = -10 # mm
    x_end = 10 # mm
    x = np.linspace(x_start, x_end, example_1_num_line) / 1000

    z_start = example_1_start_positions[2]
    z_end = (example_1_num_sample * example_1_num_upsample - 1) * example_1_directions[2] + example_1_start_positions[2]
    z = np.linspace(z_start, z_end, example_1_num_sample*example_1_num_upsample)
    X, Z = np.meshgrid(x, z)

    return X, Z

# contour plot
def fun_gen_contour_plot(X, Z, hil_trans, contour_png_path):
    fig, ax = plt.subplots()

    envelop = 20 * np.log10(hil_trans)
    envelop = envelop - envelop.max()

    contour_levels = list(range(-60, 1, 6))
    contour_line = plt.contour(X, Z, envelop, levels=contour_levels)
    plt.autoscale()

    png_path = os.path.join(contour_png_path, "scanline_contour.png")
    plt.savefig(png_path,dpi = 100)

# contour plot
def fun_gen_gray_plot(X, Z, image, gray_png_path):
    fig, ax = plt.subplots()
    image = 20 * np.log10(image)
    image = image - image.max()
    plt.imshow(image, cmap='gray', aspect='auto', extent=[X[0, 0], X[0, -1], Z[0, 0], Z[-1, 0]], vmin=-60, vmax=0, origin='lower')
    plt.autoscale()

    png_path = os.path.join(gray_png_path, "scanline_gray.png")
    plt.savefig(png_path,dpi = 100)

def main(read_file_path, path_outputpng):
    read_data = read_column_from_txt(read_file_path)
    image = read_data.reshape(example_1_num_line, example_1_num_sample*example_1_num_upsample)
    image = image.T

    hil_trans = np.abs(hilbert(image, axis=0))

    X, Z = fun_gen_grid()

    fun_gen_contour_plot(X, Z, hil_trans, path_outputpng)
    fun_gen_gray_plot(X, Z, hil_trans, path_outputpng)

if __name__ == "__main__":
    script_path = os.path.abspath(os.path.dirname(__file__))

    if len(sys.argv) == 3:
        read_file_path = os.path.join(script_path, sys.argv[1])
        png_path = os.path.join(script_path, sys.argv[2])
    else:
        read_file_path = os.path.join(script_path, "xf_output_res.txt")
        png_path = script_path
    
    main(read_file_path, png_path)