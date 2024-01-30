.. 
  .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _mlp_introduction:

**************************
MLP Introduction 
**************************

Multilayer perceptron (MLP) is the general term for a class of feedforward neural networks. 
The main feature of an MLP is that it consists of at least three layers of nodes: an input layer,
a hidden layer, and an output layer. A fully connected layer connects all neurons in two adjacent layers together. 

A fully connected neural network (FCN) addressed by this library is an MLP that only contains fully connected layers.
For batched inputs, the basic operations in each fully connected layer include a dense matrix matrix multiplication
and an activation function, for example, sigmoid function.The operations are chained together to carry out 
the filtering process addressed by the neural network.  *Figure 1* illustrates the chained operations 
involved in a fully connected neural network. The FCN in *Figure 1* consists of four layers, 
namely the input layer, the two hidden layers, and the output layer. 
There are 356 inputs, 30 neurons in the first hidden layer, 20 neurons in the second hidden layer and five outputs.
The activation function in each layer is sigmoid function, and the inputs are batched. 
The implementation of this neural network involves three chained operations,
meaning the results of the previous operation become the inputs of the next operation. 

The first operation **C1 = Sigm(IN1 * W1 + Bias1)** happens when data are filtered through 
from the input layer to the first hidden layer. 
The second operation **C2 = Sigm(C1*W2 + Bias2)** happens between the first hidden layer 
and the second hidden layer. The last operation **C3 = Sigm(C2 * W3 + Bias3)** happens
between the second hidden layer and the output layer. Here, **Sigm** denotes the sigmoid function. 
The input matrix **IN1** is formed by batching input vectors. The batch size is denoted by **batches**.
The weight matrices in each layer are denoted by **W1, W2, and W3**. The bias matrices are
denoted by **Bias1, Bias2, and Bias3**. The results matrices are used as input matrices, for example, **C1 and C2**.
This forms an operation chain.

.. figure:: /images/mlp_fcn.png
    :align: center
    :alt: A fully connected neural network example
    
    Figure 1. FCN example

The basic primitives in this release are implemented in class ``Fcn``. 
The activation function primitives provided by this release include ``relu``, ``sigmoid`` and ``tansig``.
