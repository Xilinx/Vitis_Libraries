# Testcase Running Process

Before running testcase,  
- the xclbinPath in config.json should be changed to the absolute path  
- XRT and XRM environment settings should be sourced  

## example:  
    source /opt/xilinx/xrt/setup.sh  
    source /opt/xilinx/xrm/setup.sh  
    cd testcase 
    make build TARGET=hw 
    vim config.json and change xclbinPath to full path of xclbin
    make run TARGET=hw 

For more details of the testcases, please ref to [Vitis Graph Library Documentation](https://xilinx.github.io/Vitis_Libraries/graph/2020.1/index.html)
    

