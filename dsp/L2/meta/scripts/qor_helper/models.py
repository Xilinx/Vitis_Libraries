#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
from sklearn.base import BaseEstimator, RegressorMixin, TransformerMixin, clone
from sklearn.model_selection import train_test_split
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
    def __init__(self, offset=0):
        self.offset = offset

    def fit(self, X, y=None):
        self.is_fitted_ = True
        return self
    
    def transform(self, x):
        return np.log(x + self.offset)
    
    def inverse_transform(self, x):
        return np.exp(x) - self.offset

class InvSqrtProbability(BaseEstimator, TransformerMixin):
    """
    A custom stateful transformer which divides one-hotted columns by the square root of their probability.
    """
    def __init__(self):
        self.inv_sqrt_probability_ = None

    def fit(self, X, y=None):
        probability = X.sum(axis=0) / len(X)
        self.inv_sqrt_probability_ = 1 / np.sqrt(probability)
        return self
    
    def transform(self, X):
        if self.inv_sqrt_probability_ is None:
            raise RuntimeError("InvSqrtProbability transformer has not been fitted yet.")
        return X * self.inv_sqrt_probability_

    def inverse_transform(self, X):
        return X / self.inv_sqrt_probability_

class InvertibleColumnTransformer(ColumnTransformer):
    """
    Debug ColumnTransformer which allows the vector embedding to be reversed."
    """
    def inverse_transform(self, X):
        parts = {}
        col_names = {}
        for name, transformer, cols in self.transformers_:
            if name == "remainder":
                continue
            idx = self.output_indices_[name]
            X_slice = X[:, idx]
            parts[name] = transformer.inverse_transform(X_slice)
            col_names[name] = cols
        # reassemble in transformer declaration order
        frames = []
        for name, _, cols in self.transformers_:
            if name == "remainder":
                continue
            frames.append(pd.DataFrame(parts[name], columns=col_names[name]))
        return pd.concat(frames, axis=1)


class BaseModelPipeline(BaseEstimator, RegressorMixin):  
    def __init__(self, model, model_params:dict, pca_variance=False, degree=False):
        self.model = model
        self.model_params = model_params
        self.init_columns(model_params)
        self.init_preprocessing(pca_variance, degree)

        self.y_process_pipe = Pipeline(steps = [
            ("log_transform", LogTransform()),   # will apply exp in reverse
            ("scale", StandardScaler())        
        ])

    def init_columns(self, model_params:dict):
        self.categorical_cols = model_params["categorical"]
        self.ordinal_cols = model_params["ordinal"]            # ordinal/continuous input features
        self.dependent_cols = model_params["dependent"]        # outcome variables
        self.independent_cols = self.categorical_cols + self.ordinal_cols

    def init_preprocessing(self, pca_variance=False, degree=False):
        categorical_transformer = Pipeline(steps = [
            ("one_hot", OneHotEncoder(sparse_output=False, handle_unknown="error")),
            ("div_by_sqrt_prob", InvSqrtProbability())
        ])
        ordinal_transformer = Pipeline(steps = [
            ("log_transform", LogTransform(1)), # offset required as zero-values in data.
        ])
        column_transformer = InvertibleColumnTransformer(
            transformers = [
                ("cat", categorical_transformer, self.categorical_cols),
                ("ord", ordinal_transformer, self.ordinal_cols),
            ]
        )
        preprocessing_steps = [
            ("column_transform", column_transformer),
            ("scale", StandardScaler())
            ]
        if pca_variance is not False:
            preprocessing_steps.extend([
                ("pca", PCA(n_components=pca_variance, svd_solver="full")),
                ])  # applies centering
        if degree is not False:
            preprocessing_steps.extend([
                ("poly_features", PolynomialFeatures(degree)),
                ])
        self.preprocessor = Pipeline(steps = preprocessing_steps)

    def fit(self, X:pd.DataFrame, y:pd.DataFrame):
        X_train, X_valid, y_train, y_valid = train_test_split(X, y, test_size=0.2, random_state=42)
        X_train_preprocessed = self.preprocessor.fit_transform(X_train)
        X_valid_preprocessed = self.preprocessor.transform(X_valid) # dont refit on valid data!
        X_full_preprocessed = self.preprocessor.fit_transform(X)

        self.models = {col: clone(self.model) for col in self.dependent_cols}
        self.y_process_pipes = {col: clone(self.y_process_pipe) for col in self.dependent_cols}
        self.n_samples_per_col = {}
        err_stdev = {}

        for col in self.dependent_cols:
            mask = y_train[col] > 0 # Each dependent column keeps its own rows, only removing relevant -1s.
            X_col = X_train_preprocessed[mask]
            y_col = self.y_process_pipes[col].fit_transform( y_train.loc[mask, [col]] )
            self.models[col].fit(X_col, y_col)

            # Computing confidence interval...
            mask = y_valid[col] > 0
            X_col = X_valid_preprocessed[mask]
            y_col = self.y_process_pipes[col].transform( y_valid.loc[mask, [col]] ) # dont refit on valid data!
            y_pred = self.models[col].predict(X_col).reshape(-1, 1)
            err_stdev[col] = np.std(y_pred - y_col)  # computed in model space.

            # Refit on all rows available for this column after computing confidence interval...
            mask = y[col] > 0
            X_col = X_full_preprocessed[mask]
            y_col = self.y_process_pipes[col].fit_transform( y.loc[mask, [col]] )
            self.models[col].fit(X_col, y_col)
            self.n_samples_per_col[col] = sum(mask)

        self.err_stdev = pd.Series(err_stdev)

    def preprocess(self, X:pd.DataFrame) -> np.ndarray:
        X_ordered = X[self.independent_cols]
        return self.preprocessor.transform(X_ordered)
      
    def predict(self, X:pd.DataFrame):
        X_preprocessed = self.preprocess(X)
        y_preds = {}
        for col in self.dependent_cols:
            y_pred = self.models[col].predict(X_preprocessed).reshape(-1, 1)
            y_pred = self.y_process_pipes[col].inverse_transform(y_pred)    # reverse the PCA, standard scale, and log transform.
            y_preds[col] = y_pred.ravel()
        return pd.DataFrame(y_preds)
      
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
        from sklearn.neural_network import MLPRegressor
        super().__init__(MLPRegressor(**kwargs), model_params, pca_variance)


def preprocess_dataframe(df, independent_cols, dependent_cols, config_translation, aie_mapper):
    df = df.rename(columns=config_translation)
    df = df[independent_cols + dependent_cols]
    df["AIE_VARIANT"] = df["AIE_VARIANT"].apply(lambda x: aie_mapper[x] if x in aie_mapper else x)
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
    parser.add_argument("--performance_report", action="store_true", default=False)

    args = parser.parse_args()
    ip_name = args.ip
    model_name = args.model
    training_csv_path = args.training_csv_path
    test_csv_path = args.test_csv_path
    performance_report = args.performance_report

    with open("data/model_params.json", 'r') as file:
        MODEL_PARAMS = json.load(file)
        model_params = MODEL_PARAMS[ip_name]
        no_iterate_params = MODEL_PARAMS["no_iterate_params"]

    with open("data/config_translation.json", "r") as file:
        CONFIG_TRANSLATION = json.load(file)
    
    categorical_params = model_params["categorical"]
    ordinal_params = model_params["ordinal"]
    independent_cols = categorical_params + ordinal_params
    dependent_cols = model_params["dependent"]
    restricted_params = model_params["restricted"] if "restricted" in model_params else {}

    sampler = ConfigSampler(ip_name, restricted_params, ordinal_params, categorical_params, no_iterate_params)

    df = pd.read_csv(training_csv_path)
    df = preprocess_dataframe(df, independent_cols, dependent_cols, CONFIG_TRANSLATION, sampler.aie_mapper).reset_index(drop=True)

    if performance_report:
        from sklearn.metrics import r2_score, mean_absolute_percentage_error

        def median_absolute_percentage_error(y_true, y_pred):
            percentage_errors = np.abs((y_true - y_pred) / y_true)
            return np.median(percentage_errors)       

        if test_csv_path:
            df_train = df
            df_test = pd.read_csv(test_csv_path)
            df_test = preprocess_dataframe(df_test, independent_cols, dependent_cols, CONFIG_TRANSLATION, sampler.aie_mapper)
        else:
            df_train, df_test = train_test_split(df, test_size=0.2, random_state=42)

        X_train = df_train[independent_cols]
        y_train = df_train[dependent_cols]
        X_test = df_test[independent_cols]
        y_test = df_test[dependent_cols]

        model = declare_model(model_name, model_params, pca_variance=0.999)
        model.fit(X_train, y_train)
        
        def get_performance_df(y_test, y_pred) -> pd.DataFrame:
            r2_scores, MAPEs, MdAPEs = dict(), dict(), dict()

            for col in dependent_cols:
                mask = y_test[col] > 0
                truth = np.array(y_test[col])[mask]
                pred = np.array(y_pred[col])[mask]

                r2_scores[col] = r2_score(truth, pred)
                MAPEs[col] = mean_absolute_percentage_error(truth, pred) * 100
                MdAPEs[col] = median_absolute_percentage_error(truth, pred) * 100

            concat = {
                "R-Squared": r2_scores, 
                "Mean Absolute Percentage Error": MAPEs,
                "Median Absolute Percentage Error": MdAPEs
                }
            return pd.DataFrame.from_dict(concat, orient="index")

        y_pred_test = model.predict(X_test)
        df_eval_test = get_performance_df(y_test, y_pred_test)
        
        # --- Confidence interval capture table ---
        # err_stdev is the residual stdev in each column's own y_process_pipe output space
        # (log_transform then StandardScaler), so bounds must be built in that
        # same space before inverse-transforming back to real units.
        capture_rows = {}
        for n_std in [0, 1, 2, 3]:
            row = {}
            for col in dependent_cols:
                mask = y_test[col] > 0
                y_pred_col_model_space = model.y_process_pipes[col].transform(y_pred_test[[col]])[mask]
                lower = model.y_process_pipes[col].inverse_transform(y_pred_col_model_space - n_std * model.err_stdev[col]).ravel()
                upper = model.y_process_pipes[col].inverse_transform(y_pred_col_model_space + n_std * model.err_stdev[col]).ravel()
                y_true = np.array(y_test[col])[mask]
                row[col] = f"{np.mean((y_true >= lower) & (y_true <= upper)) * 100:.1f}%"
            capture_rows[f"σ={n_std}"] = row
        df_capture = pd.DataFrame.from_dict(capture_rows, orient="index")


        print("*** PERFORMANCE REPORT ***")
        print(f"IP: {ip_name} | Model: {model_name}")
        print(f"Training CSV Path: {training_csv_path}")
        print(f"Test CSV Path: {test_csv_path}")
        print(f"# Train Samples: {model.n_samples_per_col}")
        print("Metrics:")
        print("On test data (This is tells us how good we actually are):")
        print(df_eval_test)
        print("\nConfidence Interval Capture Rate (Test Data):")
        print(df_capture)


    X = df[independent_cols]
    y = df[dependent_cols]

    model = declare_model(model_name, model_params, pca_variance=0.999)
    model.fit(X, y)
    print("model training successful")
    model.save_model(f"models/{ip_name}_{model_name}.pkl")