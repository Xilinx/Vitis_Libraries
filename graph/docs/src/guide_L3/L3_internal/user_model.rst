.. 
   .. Copyright © 2020–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: graph, running flow, asynchronous
   :description: A series of classes are provided that represent the various graph models that are supported. These classes provide all the methods that are required to run that graph model on a given HW device.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

****************
Running Examples
****************

A series of classes are provided that represent the various graph models that are supported.
These classes provide all the methods that are required to run that graph model on a given HW device.

Basic Flow
**********

The hardware deployments and API executions have been separated. All the hardware deployment related information should be defined in the struct of xf::graph::L3::Handle::singleOP. 
To run an example there are 8 basic steps:

* Instantiate an instance of struct xf::graph::L3::Handle::singleOP
* Instantiate an instance of class xf::graph::L3::Handle
* Deploy hardwares by using the instance of struct singleOP
* Instantiate an instance of class xf::graph::L3::Graph and load data
* Load Graph instance to hardwares
* Run the model (as many times as required)
* Release the Handle instance in order to recycle hardware resources
* Release the Graph instance and buffers in order to avoid memory leaks


Example
*******
.. code-block:: c++

    #include "xf_graph_L3.hpp"
    #include <cstring>

    #define DT float

    //----------------- Setup shortestPathFloat thread -------------
    std::string opName;
    std::string kernelName;
    int requestLoad;
    std::string xclbinPath;
    int deviceNeeded;

    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName.c_str();
    op0.setKernelName((char*)kernelName.c_str());
    op0.requestLoad = requestLoad;
    op0.xclbinPath = xclbinPath;
    op0.deviceNeeded = deviceNeeded;

    xf::graph::L3::Handle handle0;
    handle0.addOp(op0);
    handle0.setUp();

    //----------------- Readin Graph data ---------------------------
    uint32_t numVertices;
    uint32_t numEdges;
    xf::graph::Graph<uint32_t, DT> g("CSR", numVertices, numEdges);

    //----------------- Load Graph ----------------------------------
    (handle0.opsp)->loadGraph(g);

    //---------------- Run L3 API -----------------------------------
    bool weighted = 1;
    uint32_t nSource = 1;
    uint32_t sourceID = 10;
    uint32_t length = ((numVertices + 1023) / 1024) * 1024;
    DT** result;
    uint32_t** pred;
    result = new DT*[nSource];
    pred = new uint32_t*[nSource];
    for (int i = 0; i < nSource; ++i) {
        result[i] = xf::graph::L3::aligned_alloc<DT>(length);
        pred[i] = xf::graph::L3::aligned_alloc<uint32_t>(length);
    }
    auto ev = xf::graph::L3::shortestPath(handle0, nSource, &sourceID, weighted, g, result, pred);
    int ret = ev.wait();

    //--------------- Free and delete -----------------------------------
    handle0.free();
    g.freeBuffers();

    for (int i = 0; i < nSource; ++i) {
        free(result[i]);
        free(pred[i]);
    }
    delete[] result;
    delete[] pred;
  

Asynchronous Execution
**********************

Each API in Graph L3 is implemented in asynchronous mode. So they can receive multi-requests at the same time and if hardware resources are sufficient, different Graph L3 APIs can be executed at the same time. The L3 framework can fully use the hardware resources and achieve high throughput scheduling.   


Example of using multiple requests
----------------------------------
.. code-block:: c++

    #include "xf_graph_L3.hpp"
    #include <cstring>

    #define DT float

    //----------------- Setup shortestPathFloat thread -------------
    std::string opName0;
    std::string kernelName0;
    int requestLoad0;
    std::string xclbinPath0;
    int deviceNeeded0;

    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName0.c_str();
    op0.setKernelName((char*)kernelName0.c_str());
    op0.requestLoad = requestLoad0;
    op0.xclbinPath = xclbinPath0;
    op0.deviceNeeded = deviceNeeded0;

    handle0.addOp(op0)
    handle0.setUp();

    //----------------- Setup pageRank thread -------------
    std::string opName1;
    std::string kernelName1;
    int requestLoad1;
    std::string xclbinPath1;
    int deviceNeeded1;

    xf::graph::L3::Handle::singleOP op1;
    op1.operationName = (char*)opName1.c_str();
    op1.setKernelName((char*)kernelName1.c_str());
    op1.requestLoad = requestLoad1;
    op1.xclbinPath = xclbinPath1;
    op1.deviceNeeded = deviceNeeded1;

    handle1.addOp(op1);
    handle1.setUp();

    //----------------- Readin Graph data ---------------------------
    uint32_t numVertices0;
    uint32_t numEdges0;
    xf::graph::Graph<uint32_t, DT> g0("CSR", numVertices0, numEdges0);
    uint32_t numVertices1;
    uint32_t numEdges1;
    xf::graph::Graph<uint32_t, DT> g1("CSR", numVertices1, numEdges1);

    //----------------- Load Graph ----------------------------------
    (handle0.opsp)->loadGraph(g0);
    (handle1.oppg)->loadGraph(g1);

    //---------------- Run L3 API -----------------------------------
    auto ev1 = xf::graph::L3::shortestPath(handle0, nSource1, &sourceID1, weighted1, g0, result1, pred1);
    auto ev2 = xf::graph::L3::shortestPath(handle0, nSource2, &sourceID2, weighted2, g0, result2, pred2);
    auto ev3 = xf::graph::L3::shortestPath(handle0, nSource3, &sourceID3, weighted3, g0, result3, pred3);
    auto ev4 = xf::graph::L3::shortestPath(handle0, nSource4, &sourceID5, weighted4, g0, result4, pred4);
    auto ev5 = xf::graph::L3::pageRankWeight(handle1, alpha, tolerance, maxIter, g1, pagerank);
    int ret1 = ev1.wait();
    int ret2 = ev2.wait();
    int ret3 = ev3.wait();
    int ret4 = ev4.wait();
    int ret5 = ev5.wait();

    //--------------- Free and delete -----------------------------------
    handle0.free();
    g0.freeBuffers();
    g1.freeBuffers();



