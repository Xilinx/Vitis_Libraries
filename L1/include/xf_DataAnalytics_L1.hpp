/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file xf_DataAnalytics_L1.hpp
 * @brief Top-level header for Vitis Security Library.
 */

#ifndef _XF_DATA_ANALYTICS_L1_HPP_
#define _XF_DATA_ANALYTICS_L1_HPP_

#include "xf_DataAnalytics/classification/decision_tree.hpp"
#include "xf_DataAnalytics/classification/decision_tree_predict.hpp"
#include "xf_DataAnalytics/classification/decision_tree_train.hpp"
#include "xf_DataAnalytics/common/enums.hpp"
#include "xf_DataAnalytics/regression/gradient.hpp"
#include "xf_DataAnalytics/clustering/kmeansPredict.hpp"
#include "xf_DataAnalytics/regression/linearRegression.hpp"
#include "xf_DataAnalytics/classification/logisticRegression.hpp"
#include "xf_DataAnalytics/classification/naive_bayes.hpp"
#include "xf_DataAnalytics/common/SGD.hpp"
#include "xf_DataAnalytics/common/stream_local_processing.hpp"
#include "xf_DataAnalytics/classification/svm_predict.hpp"
#include "xf_DataAnalytics/common/table_sample.hpp"
#include "xf_DataAnalytics/common/utils.hpp"

#endif
