## Ultrasound Library - Level 1 (L1)

The level 1 of Vitis Ultrasound Library contains the aie kernels. For more details information, please reference to L1 User Guide in the document for usage and design information.

## Overview of APIs
The L1 APIs is a set of APIs which are created to create a closest mapping possible between *NumPy* and the *C++* SIMD APIs of the AI Engine. The APIs resulted are a more comprehensive and easy interface for the user to create its own application using numpy-like interfaces. 
The APIs can be divided in two main groups:

1. Element-wise operations between vectors and matrices. Those operations varies between basic operations (i.e. sum, multiplication etc...) to more complex ones (i.e. reciprocal, sign etc...);
2. Vector managing and creation. Those operations are intended to be used by the user to create or modify the dimension of the vectorial operands. Two example of those operations are *Tile* and *Ones*.

Follow the details of L1 kernel available in the library:

### Kernel name: absV

```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void absV(adf::input_buffer<T>& __restrict in, adf::output_buffer<T>& __restrict out);

```

Element-Wise absolute values of a vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
### Kernel name: cosV


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void cosV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);
```

Element-Wise cosine values of a vector. Elements must be expressed in radians with the range [0...2k$\pi$] 

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: diffMV


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void diffMV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise difference between the of the row of a matrix and the values of a vector. The number of the column of the matrix and the entry of the vector must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first matrix to be passed to the kernel.
	- `in2`: elements of the array to be passed to the kernel.
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.

### Kernel name: diffSV


```c++
template<typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffSV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise difference between a scalar and the values of a vector. For every iteration (expressed by `LEN`) we need to pass 4 times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: scalar element to be passed to the kernel.
	- `in2`: elements of the array to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: diffVS

```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void diffVS(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise difference between the values of a vector and a scalar. For every iteration (expressed by `LEN`) we need to pass 4 times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `in2`: scalar element to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
### Kernel name: divVS


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void divVSSpeedOfSound(adf::input_buffer<T>& in1, adf::output_buffer<T>& out);
```
Element-Wise division between the values of a vector and a scalar (SpeedOfSound). For every iteration (expressed by `LEN`) we need to pass 4 times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `in2`: scalar element to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
### Kernel name: equalS


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM, const unsigned SCALAR>
void equalS(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);
```

Check whether the element of an array are equal to a specific number. An array of 0s or 1s is returned. 1 means that the element at that specific position is equal to the scalar, otherwise 0 is returned.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `SCALAR`: scalar to which the elements of the array are compared to;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
### Kernel name: lessOrEqualThanS


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM, const unsigned SCALAR>
void lessOrEqualThanS(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);
```

Check whether the element of an array are less or equal to a specific number. An array of 0s or 1s is returned. 1 means that the element at that specific position is less or equal to the scalar, otherwise 0 is returned.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `SCALAR`: scalar to which the elements of the array are compared to;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: mulMM


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulMM(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise multiplication of two matrixes. The first matrix and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first matrix to be passed to the kernel.
	- `in2`: elements of the second matrix to be passed to the kernel.
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.

### Kernel name: mulVS


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVS(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise multiplication between the values of a vector and a scalar. For every iteration (expressed by `LEN`) we need to pass 4 times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `in2`: scalar element to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: mulVV


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise multiplication of two vectors. The first vector and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first vector to be passed to the kernel.
	- `in2`: elements of the second vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: norm_axis_1


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void norm_axis_1(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);
```

Perform row wise the euclidean norm of a matrix of the columns. Because for every row returns a number, the result is a vector of values which represents for every row the magnitude of the euclidean norm.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: ones


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void ones(adf::output_buffer<T>& __restrict out);
```

Return a vector of with all entry set to 1. 

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: outer


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void outer(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Perform the outer product (also named cross-product or vector-product) between two vectors. The result of this operation is a matrix which rows are the number of the entry of the first vector and the column the number of the entry of the second one.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first vector to be passed to the kernel.
	- `in2`: elements of the second vector to be passed to the kernel.
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.

### Kernel name: reciprocalV


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void reciprocalV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);
```

Element-wise inverse operation of the entry of the vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: sqrtV

```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void sqrtV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);
```

Element-wise square root operation of the entry of the vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: squareV

```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void squareV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);
```

Element-wise square operation of the entry of the vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: sum_axis_1


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void sum_axis_1(adf::input_buffer<T>& in1, adf::output_buffer<T>& out);
```

Perform row wise the reduce add of a matrix of the columns. Because for every row returns a number, the result is a vector of values which represents for every row the magnitude of the reduce add operation.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: sumMM


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void sumMM(adf::input_buffer<T>& in1, adf::input_buffer<T>& in2, adf::output_buffer<T>& out);
```

Element-Wise sum of two matrixes. The first matrix and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first matrix to be passed to the kernel.
	- `in2`: elements of the second matrix to be passed to the kernel.
	- `out`: elements of the result of the operation to be passed from the kernel.

### Kernel name: sumVS


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void sumVSStream(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise addition between the values of a vector and a scalar. For every iteration (expressed by `LEN`) we need to pass 4 times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `in2`: scalar element to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: sumVV


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void sumVV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);
```

Element-Wise addition of two vectors. The first vector and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first vector to be passed to the kernel.
	- `in2`: elements of the second vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

### Kernel name: tileV


```c++
template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void tileVApo(adf::output_buffer<T>& __restrict out);
```

This kernel create a vector and returns it `LEN` times.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2024, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
