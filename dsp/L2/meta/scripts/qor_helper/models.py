#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
import os
import numpy as np
import pandas as pd
import joblib
import json
from sklearn.compose import ColumnTransformer
from sklearn.preprocessing import OneHotEncoder, OrdinalEncoder, StandardScaler
from sklearn.base import BaseEstimator, RegressorMixin

from pathlib import Path
import sys
sys.path.append(str(Path.cwd().parent))
import metadata_api

with open("data/column_dict.json", 'r') as file:
    COLUMN_DICT = json.load(file)
    COLUMN_DICT = {k:set(v) for k,v in COLUMN_DICT.items()}

with open("data/config_translation.json", "r") as file:
    CONFIG_TRANSLATION = json.load(file)

class BaseModelPipeline(BaseEstimator, RegressorMixin):  
    def __init__(self, model=None, column_mapper:dict=None, cat_encoding:str="one_hot", scaler=StandardScaler):
        self.model = model
        self.column_mapper = column_mapper
        self.cat_encoding = cat_encoding
        if self.column_mapper:
            self.set_columns(column_mapper)
            self.set_preprocessing(cat_encoding, scaler)

    def set_columns(self, column_mapper:dict):
        self.categorical_cols = column_mapper["categorical"]
        self.ordinal_cols = column_mapper["ordinal"]            # ordinal/continuous input features
        self.dependent_cols = column_mapper["dependent"]        # outcome variables
        self.independent_cols = self.categorical_cols + self.ordinal_cols

    def set_preprocessing(self, cat_encoding:str, scaler):
        transformers = []
        if cat_encoding == "one_hot":
            transformers.append(("cat_cols_one_hot", OneHotEncoder(sparse_output=False, handle_unknown='ignore'), self.categorical_cols))
        elif cat_encoding == "ordinal":
            transformers.append(("cat_cols_ordinal", OrdinalEncoder(handle_unknown='use_encoded_value', unknown_value=-1), self.categorical_cols))
        else:
            raise ValueError("cat_encoding must be one of ['one_hot', 'ordinal'].")
        if scaler:
            transformers.append(("independent_scale", scaler(), self.ordinal_cols))
        self.preprocessor = ColumnTransformer(transformers, remainder="passthrough")

    def fit(self, X:pd.DataFrame, y):
        self.preprocessor.fit(X)
        X_preprocessed = self.preprocess(X)
        y_preprocessed = y[self.dependent_cols]
        self.model.fit(X_preprocessed, y_preprocessed)

    def preprocess(self, X:pd.DataFrame) -> np.ndarray:
        X_ordered = X[self.independent_cols]
        return self.preprocessor.transform(X_ordered)
      
    def predict(self, X):  
        X_preprocessed = self.preprocess(X)
        y_pred = self.model.predict(X_preprocessed)
        return pd.DataFrame(y_pred, columns=self.dependent_cols).astype(int)
      
    def save_model(self, filepath:str):
        """Save the model to a file."""
        os.makedirs( str(Path(filepath).parent), exist_ok=True )
        joblib.dump(self, filepath)
        print(f"Model saved to {filepath}")


class LinearRegressionPipeline(BaseModelPipeline):
    def __init__(self, column_mapper:dict=None, cat_encoding:str="one_hot", scaler=StandardScaler):
        from sklearn.linear_model import LinearRegression
        super().__init__(LinearRegression(), column_mapper, cat_encoding, scaler)


class PolynomialRegressionPipeline(LinearRegressionPipeline):
    def __init__(self, column_mapper:dict=None, cat_encoding:str="one_hot", scaler=StandardScaler, **kwargs):
        from sklearn.preprocessing import PolynomialFeatures
        super().__init__(column_mapper, cat_encoding, scaler)
        self.poly_feat_encoder = PolynomialFeatures(**kwargs)

    def preprocess(self, X:pd.DataFrame) -> np.ndarray:
        X_preprocessed = super().preprocess(X)
        return self.poly_feat_encoder.fit_transform(X_preprocessed) # ? unfortunately non-trivial to include this in preprocessor (it does not work with named columns)


class SVRPipeline(BaseModelPipeline):
    def __init__(self, column_mapper:dict=None, cat_encoding:str="one_hot", scaler=StandardScaler, **kwargs):
        from sklearn.svm import SVR
        super().__init__(None, column_mapper, cat_encoding, scaler)
        if column_mapper:
            self.models = [SVR(**kwargs) for _ in range(len(column_mapper["dependent"]))]

    def fit(self, X, y):
        X_preprocessed = self.preprocessor.fit_transform(X)
        y_preprocessed = y[self.dependent_cols]
        for i, model in enumerate(self.models):
            model.fit(X_preprocessed, y_preprocessed.iloc[:,i])

    def predict(self, X) -> pd.DataFrame:
        X_preprocessed = self.preprocess(X)
        y_preds = [model.predict(X_preprocessed) for model in self.models]
        y_pred = np.array(y_preds).T
        return pd.DataFrame(y_pred, columns=self.dependent_cols).astype(int)


class RandomForestPipeline(BaseModelPipeline):  
    def __init__(self, column_mapper:dict=None, cat_encoding:str="one_hot", scaler=StandardScaler, **kwargs):  
        from sklearn.ensemble import RandomForestRegressor  
        super().__init__(RandomForestRegressor(**kwargs), column_mapper, cat_encoding, scaler)


class MLPPipeline(BaseModelPipeline):
    def __init__(self, column_mapper:dict=None, cat_encoding:str="one_hot", scaler=StandardScaler, **kwargs):
        from sklearn.neural_network import MLPRegressor
        super().__init__(MLPRegressor(**kwargs), column_mapper, cat_encoding, scaler)


def load_model(filepath:str):
    """Load model from a file."""
    return joblib.load(filepath)

def declare_model(model_name, *args, **kwargs):
    if model_name == "linear":
        return LinearRegressionPipeline(*args, **kwargs)
    elif model_name == "polynomial":
        return PolynomialRegressionPipeline(degree=2, *args, **kwargs)
    elif model_name == "svr":
        return SVRPipeline(*args, **kwargs)
    elif model_name == "random_forest":
        return RandomForestPipeline(*args, **kwargs)
    else:
        raise ValueError("Invalid model name")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--ip")
    parser.add_argument("--model")
    parser.add_argument("--training_csv_path")

    args = parser.parse_args()
    ip_name = args.ip
    model_name = args.model
    training_csv_path = args.training_csv_path

    train_df = pd.read_csv(training_csv_path)
    train_df = train_df.rename(columns=CONFIG_TRANSLATION)  # ensure correctly labelled columns
    col_set = {v for k,v in CONFIG_TRANSLATION.items()}
    keep_cols = list(set(train_df.columns) & col_set)
    train_df = train_df[keep_cols]
    train_df.rename(columns=CONFIG_TRANSLATION, inplace=True)  # ensure correctly labelled columns
    train_df = train_df[train_df['Latency'] != -1]   # remove non-functional cases
    train_df = train_df.dropna()

    col_names = set(train_df.columns)
    ip = metadata_api.IP(ip_name, {})
    column_mapper = {
        "categorical": sorted(COLUMN_DICT["categorical"] & col_names & set(ip.params)),   # the order of params must be consistent across initializations.
        "ordinal": sorted(COLUMN_DICT["ordinal"] & col_names & set(ip.params)),
        "dependent": sorted(COLUMN_DICT["dependent"] & col_names)
    }
    independent_cols = column_mapper["categorical"] + column_mapper["ordinal"]
    dependent_cols = column_mapper["dependent"]

    X = train_df[independent_cols]
    y = train_df[dependent_cols]
    
    model = declare_model(model_name, column_mapper)
    
    model.fit(X, y)
    print("model training successful")
    model.save_model(f"models/{ip_name}_{model_name}.pkl")