.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

2024.1
------

- The following cases are deprecated and removed.

  - L2/tests/knn_sc
  - L3/tests/gunzip_csv_sc_test
  - L3/tests/re_sc_test

2023.2
------

There are some known issues for this release.

- The following cases fail the hardware run with 2023.1 AMD Vitis™ and XRT. Use 2022.2 Vitis and XRT.

  - L2/tests/knn_sc
  - L3/tests/gunzip_csv_sc_test

2023.1
------

There are some known issues for this release.

- The following kernels in L2/tests fail .the hardware run with 2023.1 Vitis and XRT. Use 2022.2 Vitis and XRT.

  - clustering/kmeans
  - classification/xGradientBoost  
  - classification/decisiontree
  - regression/linearRegressionSGDTrain  
  - regression/ridgeRegressionSGDTrain

2022.2
------

The 2022.2 release has the following addition:

* **String LIKE** API returns true if the string matches the supplied pattern, which works liking string `find` in c++. As expected, the NOT LIKE expression returns false if LIKE returns true.
* **String EQUAL** API returns true if the string completely match with the base string, which works liking string `compare` in c++. As expected, the NOT EQUAL expression returns false if EQUAL returns true.
* **JSONLine Loader** API is enhanced to support a more general data type, including nested field and list.

2022.1
------

The Data Analytics Library has the following addition in the 2022.1 release:

* **csv scanner** could be used to accelerate the extract, transform, and load process. It integrates the GZIP decompression, CSV parser, and filter module together to make them work in parallel. The ETL accelerator could work together with database to run queries on a large size of semi-structured and unstructured data.
* **Geospatial APIs.** Two major APIs in this family has been included: the Spatial Join and KNN. The former API inserts the columns from one feature table to another based on location or proximity, while the latter is often used to find the K nearest neighbors around the center point. They are both vital for spatial analysis and spatial data mining.

There are some known issues for this release.

* Log Analyer in L2 demo fails the hardware build with 2022.1 Vitis. Use 2021.2 Vitis.

2021.2
------

The 2021.2 release provides the CSV Parser:

* **CSV Parser** could parse comma-separated value files and generate an object stream which could easily be connected with DataFrame APIs. CSV is a common used storage format in Date Lake. The CSV parser could be used to accelerate the data extract process. 

2021.1
------

The 2021.1 release provides Two-Gram text analytics:

* Two Gram Predicate (TGP) is a search of the inverted index with a term of two characters. For a dataset that established an inverted index, it can find the matching ID in each record in the inverted index.


2020.2
------

The Data Analytics Library has the following additions in the 2020.2 release:

* **Text Processing APIs.** Two major APIs in this family has been included: the *regular expression match* and *geo-IP lookup*. The former API can be used to extract content from unstructured data like logs, while the latter is often used in processing web logs, to annotate with geographic information by IP address. A demo tool that converts Apache HTTP server log in batch into JSON file is provided with the library.
* **DataFrame APIs.** DataFrame is widely popular in-memory data abstraction in the data analytics domain; the DataFrame write and read APIs should enable data analytics kernel developers to store temporal data or interact with open-source software using `Apache Arrow`__ DataFrame more easily.
* **Tree Ensemble Method.** *Random forest* is extended to include regression. *Gradient boost tree*, based on boosting method, is added to support both classification and regression. Support for *XGBoost on classification and regression* is also included to exploit the second order derivative of loss function and regularization.

__ http://arrow.apache.org/

2020.1
------

The 2020.1 release provides a range of HLS primitives for:

* Decision Tree
* Random Forest
* Logistic Regression
* Linear SVM
* Naive Bayes
* Linear Least Square Regression
* LASSO Regression
* Ridge Regression
* K-Means
* Stochastic Gradient Descent Optimizer
* L-BFGS Optimizer
