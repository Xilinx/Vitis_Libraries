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
from config_sampler import ConfigSampler
from sklearn.compose import ColumnTransformer
from sklearn.preprocessing import OneHotEncoder, StandardScaler, PolynomialFeatures
from sklearn.decomposition import PCA
from sklearn.pipeline import Pipeline
from sklearn.base import BaseEstimator, RegressorMixin, TransformerMixin
from pathlib import Path

class DebugTransform(BaseEstimator, TransformerMixin):
    """
    A custom dummy transformer whose sole purpose is to report the current shape of the data for debug purposes
    """
    def __init__(self, message_:str=""):
        self.message_ = message_

    def fit(self, X, y=None):
        return self
    
    def transform(self, x):
        print(f"{self.message_}{x.shape}")
        return x

class LogTransform(BaseEstimator, TransformerMixin):
    """
    A custom stateless transformer which takes the log of the data going forward and exp of data backwards.
    """
    def fit(self, X, y=None):
        return self
    
    def transform(self, x):
        return np.log(x+1)
    
    def inverse_transform(self, x):
        return np.exp(x)-1

class InvSqrtProbability(BaseEstimator, TransformerMixin):
    """
    A custom stateful transformer which divides one-hotted columns by the square root of their probability.
    """
    def __init__(self):
        self.inv_sqrt_probability_ = None

    def fit(self, X, y=None):
        probability = X.sum() / len(X)
        self.inv_sqrt_probability_ = 1 / np.sqrt(probability)
        return self
    
    def transform(self, X):
        if self.inv_sqrt_probability_ is None:
            raise RuntimeError("InvSqrtProbability transformer has not been fitted yet.")
        return X * self.inv_sqrt_probability_

# class IsPwr2(BaseEstimator, TransformerMixin):
#     """
#     A custom stateless transformer which transforms a number into a 1 if a power of 2 and 0 if not.
#     """
#     def fit(self, X, y=None):
#         return self
    
#     def transform(self, x):
#         return np.logical_and((x > 0), (x & (x - 1)) == 0).astype(int)  # bitwise check

class BaseModelPipeline(BaseEstimator, RegressorMixin):  
    def __init__(self, model, model_params:dict, pca_variance=False, degree=False):
        self.model = model
        self.model_params = model_params
        self.init_columns(model_params)
        self.init_preprocessing(pca_variance, degree)

        self.y_process_pipe = Pipeline(steps = [
            ("log_transform", LogTransform()),   # will apply exp in reverse
            ("scale", StandardScaler()),
            ("pca", PCA(n_components=None, svd_solver="full"))
        ])

    def init_columns(self, model_params:dict):
        self.categorical_cols = model_params["categorical"]
        self.ordinal_cols = model_params["ordinal"]            # ordinal/continuous input features
        self.dependent_cols = model_params["dependent"]        # outcome variables
        self.independent_cols = self.categorical_cols + self.ordinal_cols

    def init_preprocessing(self, pca_variance=False, degree=False):
        categorical_transformer = Pipeline(steps = [
            # ("debug_0", DebugTransform("categorical shape at entry: ")),
            ("one_hot", OneHotEncoder(sparse_output=False, handle_unknown="error")),
            # ("debug_1", DebugTransform("categorical shape after one_hot: ")),
            ("div_by_sqrt_prob", InvSqrtProbability())
        ])
        ordinal_transformer = Pipeline(steps = [
            # ("debug_2", DebugTransform("ordinal shape at entry: ")),
            ("log_transform", LogTransform()),
            ("scale", StandardScaler())
        ])
        # ordinal_pwr2_transformer = Pipeline(steps = [
        #     ("is_power_2", IsPwr2()),
        #     ("one_hotter", categorical_transformer)
        # ])
        column_transformer = ColumnTransformer(
            transformers = [
                ("cat", categorical_transformer, self.categorical_cols),
                ("ord", ordinal_transformer, self.ordinal_cols),
                # ("ord_pwr2", ordinal_pwr2_transformer, self.ordinal_cols)
            ]
        )
        preprocessing_steps = [
            ("column_transform", column_transformer),
            # ("debug_3", DebugTransform("X shape after column transform: ")),
            ]
        if pca_variance is not False:
            preprocessing_steps.extend([
                ("pca", PCA(n_components=pca_variance, svd_solver="full")),
                # ("debug_4", DebugTransform("X shape after PCA: ")),
                ])  # applies centering
        if degree is not False:
            preprocessing_steps.extend([
                ("poly_features", PolynomialFeatures(degree)),
                # ("debug_5", DebugTransform("X shape after polynomial features: ")),
                ])
        self.preprocessor = Pipeline(steps = preprocessing_steps)

    def preprocess(self, X:pd.DataFrame) -> np.ndarray:
        X_ordered = X[self.independent_cols]
        return self.preprocessor.transform(X_ordered)

    def fit(self, X:pd.DataFrame, y:pd.DataFrame):
        X_preprocessed = self.preprocessor.fit_transform(X)
        y_preprocessed = self.y_process_pipe.fit_transform(y[self.dependent_cols])
        self.model.fit(X_preprocessed, y_preprocessed)

        # Computing confidence intervals...
        y_pred = self.predict(X)
        y_pred_log = np.log(y_pred + 1)
        y_true_log = np.log(y + 1)
        residuals = y_pred_log - y_true_log
        self.err_stdev = np.std(residuals, axis=0)

    def preprocess(self, X:pd.DataFrame) -> np.ndarray:
        X_ordered = X[self.independent_cols]
        return self.preprocessor.transform(X_ordered)
      
    def predict(self, X:pd.DataFrame):  
        X_preprocessed = self.preprocess(X)
        y_pred = self.model.predict(X_preprocessed)
        y_pred = self.y_process_pipe.inverse_transform(y_pred)  # reverse the PCA, standard scale, and log transform.
        y_pred = pd.DataFrame(y_pred, columns=self.dependent_cols)
        return y_pred
      
    def save_model(self, filepath:str):
        """Save the model to a file."""
        os.makedirs( str(Path(filepath).parent), exist_ok=True )
        joblib.dump(self, filepath)
        print(f"Model saved to {filepath}")


class LinearRegressionPipeline(BaseModelPipeline):
    def __init__(self, model_params:dict, **kwargs):
        from sklearn.linear_model import LinearRegression
        super().__init__(LinearRegression(), model_params, **kwargs)

class RidgeRegressionPipeline(BaseModelPipeline):
    def __init__(self, model_params:dict, **kwargs):
        from sklearn.linear_model import Ridge
        super().__init__(Ridge(solver="lsqr", alpha=0.1), model_params, **kwargs)

class PolynomialRegressionPipeline(RidgeRegressionPipeline):
    def __init__(self, model_params:dict, **kwargs):
        super().__init__(model_params, **kwargs)


class SVRPipeline(BaseModelPipeline):
    def __init__(self, model_params:dict, pca_variance=False, **kwargs):
        from sklearn.svm import SVR
        from sklearn.multioutput import MultiOutputRegressor
        base_svr = SVR(kernel="poly", C=100, epsilon=0.1, verbose=True, coef0=0)
        multi_svr = MultiOutputRegressor(base_svr)
        super().__init__(multi_svr, model_params, pca_variance)


class RandomForestPipeline(BaseModelPipeline):  
    def __init__(self, model_params:dict, pca_variance=False, **kwargs):  
        from sklearn.ensemble import RandomForestRegressor  
        super().__init__(RandomForestRegressor(**kwargs), model_params, pca_variance)


class MLPPipeline(BaseModelPipeline):
    def __init__(self, model_params:dict, pca_variance=False, **kwargs):
        from sklearn.preprocessing import PolynomialFeatures
        from sklearn.neural_network import MLPRegressor
        super().__init__(MLPRegressor(**kwargs), model_params, pca_variance)


def preprocess_dataframe(df, sampler, dependent_cols, config_translation, filter_steady_state=True):
    df.columns = df.columns.str.upper()
    df = df.rename(columns=config_translation)
    df = df[sampler.params + dependent_cols].copy()
    if filter_steady_state:
        for col in dependent_cols:
            df = df[df[col] > 0]
    df["AIE_VARIANT"] = df["AIE_VARIANT"].apply(lambda x: sampler.aie_mapper[x] if x in sampler.aie_mapper else x)
    df = df.dropna()
    config_dict = df.to_dict(orient="records")
    valid_mask = [sampler.validate_config(config) for config in config_dict]
    df = df[valid_mask]
    return df

def load_model(filepath:str):
    """Load model from a file."""
    return joblib.load(filepath)

def declare_model(model_name, *args, **kwargs):
    if model_name == "linear":
        return LinearRegressionPipeline(*args, **kwargs)
    elif model_name == "polynomial":
        return PolynomialRegressionPipeline(degree=3, *args, **kwargs)
    elif model_name == "svr":
        return SVRPipeline(*args, **kwargs)
    elif model_name == "random_forest":
        return RandomForestPipeline(*args, **kwargs)
    elif model_name == "neural_network":
        return MLPPipeline( #loss="poisson", 
                            solver="adam", 
                            hidden_layer_sizes=(256,196,128,128,128,128,96,64,48,32,16),
                            learning_rate="adaptive",
                            # alpha=1,
                            tol=0.00001,
                            n_iter_no_change=100,
                            max_iter=int(1e4),
                            early_stopping=True,
                            verbose=True,
                            *args, **kwargs)
    else:
        raise ValueError("Invalid model name")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("-ip")
    parser.add_argument("-model")
    parser.add_argument("-training_csv_path")
    parser.add_argument("--test_csv_path", default="")

    args = parser.parse_args()
    ip_name = args.ip
    model_name = args.model
    training_csv_path = args.training_csv_path
    test_csv_path = args.test_csv_path

    with open("data/model_params.json", 'r') as file:
        MODEL_PARAMS = json.load(file)
        model_params = MODEL_PARAMS[ip_name]
        no_iterate_params = MODEL_PARAMS["no_iterate_params"]

    with open("data/config_translation.json", "r") as file:
        CONFIG_TRANSLATION = json.load(file)
    
    categorical_params = model_params["categorical"]
    ordinal_params = model_params["ordinal"]
    dependent_params = model_params["dependent"]
    restricted_params = model_params["restricted"] if "restricted" in model_params else {}

    sampler = ConfigSampler(ip_name, restricted_params, ordinal_params, categorical_params, no_iterate_params)

    df = pd.read_csv(training_csv_path)
    df = preprocess_dataframe(df, sampler, dependent_params, CONFIG_TRANSLATION).reset_index(drop=True)
    print("Training dataframe:")
    print(df)

    independent_cols = model_params["categorical"] + model_params["ordinal"]
    dependent_cols = model_params["dependent"]
    X = df[independent_cols]
    y = df[dependent_cols]

    model = declare_model(model_name, model_params, pca_variance=0.999)
    model.fit(X, y)
    print("model training successful")
    model.save_model(f"models/{ip_name}_{model_name}.pkl")