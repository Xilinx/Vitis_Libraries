#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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
import numpy as np  
  

def read_matrix_from_file(filename, rows, cols, niter, order='column-major', complex_data=False):  
    """  
    Reads data from a text file to construct a list of matrices of given dimensions,  
    filling them in either column-major or row-major order. Supports complex data input.  
  
    Parameters:  
    filename (str): The path to the text file.  
    rows (int): The number of rows in each matrix.  
    cols (int): The number of columns in each matrix.  
    niter (int): The number of matrices to construct.  
    order (str): The order for filling the matrices, either 'column-major' or 'row-major'.  
    complex_data (bool): Whether the data is complex. If true, each line contains real and imaginary parts.  
  
    Returns:  
    list of np.ndarray: A list of matrices constructed from the text file data.  
    """  
    with open(filename, 'r') as file:  
        data = [float(num) for line in file for num in line.split()]  
    
    # Calculate expected size based on data type  
    expected_entries = rows * cols * niter * (2 if complex_data else 1)  
    if len(data) != expected_entries:  
        raise ValueError(f"The total number of entries ({len(data)}) does not match the expected size ({expected_entries}).")  
  
    matrices = []
    idx = 0
    for _ in range(niter):
        if complex_data:  
            # Create complex matrix  
            matrix = np.zeros((rows, cols), dtype=complex)  
        else:  
            # Create real matrix  
            matrix = np.zeros((rows, cols))  

        for col in range(cols):  
            for row in range(rows):  
                if complex_data:  
                    # Extract real and imaginary parts  
                    real = data[idx]  
                    imag = data[idx + 1]  
                    matrix[row, col] = complex(real, imag)  
                    idx += 2  
                else:  
                    matrix[row, col] = data[idx]  
                    idx += 1  

        if order == 'row-major':  
            matrix = matrix.T  

        matrices.append(matrix)

    return matrices

def conjugate_transpose_matrices(matrices):
    """
    Compute the complex conjugate transpose (Hermitian) of a list of matrices.

    :param matrices: List of np.ndarray matrices to conjugate transpose.
    :return: List of conjugate transposed matrices.
    """
    return [np.conj(matrix).T for matrix in matrices]

def is_orthogonal(matrices, order='column-major', tol=1e-10):  
    """  
    Checks if the rows or columns of each matrix in a list are orthogonal, including complex matrices.  
  
    Parameters:  
    matrices (list of np.ndarray): A list of matrices to be checked.  
    order (str): Either 'row-major' to check row orthogonality or 'column-major' to check column orthogonality.  
    tol (float): Tolerance for floating point comparison.  
  
    Returns:  
    bool: True if the selected vectors (rows or columns) in all matrices are orthogonal, otherwise False.  
    """  
    for matrix in matrices:
        if order == 'row-major':  
            # Check orthogonality of rows  
            vectors = matrix  
        elif order == 'column-major':  
            # Check orthogonality of columns  
            vectors = matrix.T  
        else:  
            raise ValueError("Order must be either 'row-major' or 'column-major'.")  
        
        num_vectors = vectors.shape[0]  
        
        for i in range(num_vectors):  
            for j in range(i + 1, num_vectors):  
                # Use conjugate transpose for complex vectors
                dot_product = np.dot(vectors[i], np.conj(vectors[j]))  
                if not np.isclose(dot_product, 0, atol=tol):  
                    return False  
    return True  


def is_orthonormal(matrices, order='column-major', tol=1e-10):
    """
    Check if a matrix is orthonormal based on row or column orthogonality.

    Parameters:
    matrix (np.ndarray): The matrix to check.
    order (str): 'row-major' for row orthogonality, 'column-major' for column orthogonality.
    tolerance (float): The tolerance for the orthonormality check.

    Returns:
    bool: True if the matrix is orthonormal, False otherwise.
    """
    for matrix in matrices:
        if order == 'row-major':
            # Check row orthogonality: A * A^* should be the identity matrix
            product = np.dot(matrix, np.conj(matrix).T)
        elif order == 'column-major':
            # Check column orthogonality: A^* * A should be the identity matrix
            product = np.dot(np.conj(matrix).T, matrix)

        else:
            raise ValueError("Order must be either 'row-major' or 'column-major'.")

        # Create the identity matrix of appropriate size
        identity_matrix = np.eye(matrix.shape[0] if order == 'row-major' else matrix.shape[1])
        if np.allclose(product, identity_matrix, atol=tol) == False:
            return False
    return True

def is_triangular(matrices, triangular_type='upper', tol=1e-10):  
    """  
    Checks if matrices are triangular (upper or lower) within a tolerance for zeros.
  
    Parameters:  
    matrices (list of np.ndarray): List of matrices to check.
    triangular_type (str): 'upper' or 'lower'.
    tol (float): Tolerance for zero comparison.
  
    Returns:  
    bool: True if all matrices are of the specified triangular type within tolerance, otherwise False.
    """  
    for matrix in matrices:
        if matrix.shape[0] != matrix.shape[1]:  
            raise ValueError("The matrix must be square to check for triangular type.")  
        
        if triangular_type == 'upper':  
            # Check for upper triangular matrix  
            for i in range(1, matrix.shape[0]):  
                for j in range(0, i):  
                    if abs(matrix[i, j]) > tol:  
                        return False  
        elif triangular_type == 'lower':  
            # Check for lower triangular matrix  
            for i in range(0, matrix.shape[0]): 
                for j in range(i + 1, matrix.shape[1]):  
                    if abs(matrix[i, j]) > tol:  
                        return False  
        else:  
            raise ValueError("triangular_type must be either 'upper' or 'lower'.")     
    return True     


def is_identity_matrix(matrices, tol=1e-10):  
    """  
    Checks if a matrix is an identity matrix.  
  
    Parameters:  
    matrix (np.ndarray): The input matrix to be checked.  
    tol (float): Tolerance for floating point comparison.  
  
    Returns:  
    bool: True if the matrix is an identity matrix, otherwise False.  
    """  
    for matrix in matrices:
        # Check if the matrix is square  
        if matrix.shape[0] != matrix.shape[1]:  
            return False  
        
        # Create an identity matrix of the same size  
        identity_matrix = np.eye(matrix.shape[0])  
        
        # Check if the matrix is close to the identity matrix  
        if np.allclose(matrix, identity_matrix, atol=tol) == False:
            return False
    return True


def multiply_and_compare(matrices1, matrices2, result_matrices, matrices3=None, tol=0.1):  
    """  
    Multiplies two or three matrices and checks if the result matches another matrix.  
  
    Parameters:  
    matrices1 (list of np.ndarray): List of first matrix operands.  
    matrices2 (list of np.ndarray): List of second matrix operands.  
    result_matrices (list of np.ndarray): List of matrices to compare against the product of matrices.  
    matrices3 (list of np.ndarray, optional): List of third matrix operands. If None, only the first two matrices are multiplied.  
    tol (float): Tolerance for floating point comparison.  
  
    Returns:  
    bool: True if the product matches the result_matrix within the given tolerance, otherwise False.  
    """  
    for k, matrix1 in enumerate(matrices1):
        # Check if the first two matrices can be legally multiplied  
        if matrix1.shape[1] != matrices2[k].shape[0]:  
            raise ValueError("The number of columns of the first matrix must equal the number of rows of the second matrix.") 

        # Perform multiplication of the first two matrices  
        product = np.dot(matrix1, matrices2[k])  
        
        # If a third matrix is provided, multiply it as well  
        if matrices3 is not None:  
            if product.shape[1] != matrices3[k].shape[0]:  
                raise ValueError("The number of columns of the product matrix must equal the number of rows of the third matrix.")  
            product = np.dot(product, matrices3[k])  

        if not np.allclose(product, result_matrices[k], atol=tol):
            return False
        
    return True 



def report_validation(check, test_name, model_ut="Ref"):
    file_path="./logs/validation.txt"
    with open(file_path, 'a') as file:
        # Write a string to the file
        if check == True:
            file.write(f"{model_ut} Test Passed : "+test_name + "\n")
        else:  
            file.write(f"ERROR: {model_ut} Test Failed : "+test_name + "\n")


def report_warning(check, test_name, model_ut="Ref"):
    file_path="./logs/validation.txt"
    with open(file_path, 'a') as file:
        # Write a string to the file
        if check == True:
            file.write(f"{model_ut} Test Passed : "+test_name + "\n")
        else:  
            file.write(f"WARNING: {model_ut} Test Failed : "+test_name + "\n")