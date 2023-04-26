## Vitis Tests for L2 kernels

This folder contains basic test for each of L2 level kernels. 
They are meant to discover simple regression errors. 

More details for each kernel are available in the [full documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/quantitative_finance/index.html).

### Deprecation Notification 
**Below L2 APIs have known issues and will be deprecated soon. If you are still using these L2 APIs, please reach us through [forum](https://support.xilinx.com).**
* L2/tests/M76Engine - hw build failure on u250 platform
* L2/tests/PortfolioOptimisation - hw build failure 
* L2/tests/MCEuropeanHestonGreeksEngine - hw build failure on u50 and u200 platform
* L2/tests/MCAmericanEngineMultiKernel - hw build failure on u50 platform
* L2/tests/Quadrature - hw build failure on u200 platform
* L2/tests/MCAmericanEngine - hw build failure on u50 platform



