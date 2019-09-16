# Frequently Asked Questions

### What is Xilinx Snappy application?
Xilinx Snappy application is FPGA optimized high throughput data compression
algorithm.

### Is Xilinx Snappy application compatible with standard Google Snappy?
Yes

### What is the meaning of kernel with 8 engines?
Each engine process 1 Byte/cycle, 8 x 250MHz (AWS F1 board) results in 2GB/sec
speedup. It means 8 Bytes/cycle can be processed using 8 engines.

### The FPGA utilization seems to be on the lower side. Is it possible to increase the FPGA utilization to improve the performance?
Yes 

### What is overlapped execution?
The host to device communication can be hidden by overlapping kernel execution
time with I/O communication between host and device.

### What is the benefit of using two compute units?
A single Xilinx Snappy compute unit provides 1.87GB/s kernel throughput. Two
compute unit usage helps in extending the performance 2x1.87GB/s = 3.74GB/s 
