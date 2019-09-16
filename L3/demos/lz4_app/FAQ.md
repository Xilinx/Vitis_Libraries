# Frequently Asked Questions

### What is Xilinx LZ4 application?
Xilinx LZ4 application is FPGA optimized high throughput data compression
algorithm.

### Is Xilinx LZ4 application compatible with standard LZ4?
Yes

### What is the meaning of kernel with 8 engines?
Each engine process 1 Byte/cycle, 8 x 300MHz (Xilinx Alveo U200 board) results in 2.4GB/sec
speedup. It means 8 Bytes/cycle can be processed using 8 engines.

### The FPGA utilization seems to be on the lower side. Is it possible to increase the FPGA utilization to improve the performance?
Yes 

### What is overlapped execution?
The host to device communication can be hidden by overlapping kernel execution
time with I/O communication between host and device.

### What is the benefit of using two compute units?
It helps in improving the performance to get better throughput
