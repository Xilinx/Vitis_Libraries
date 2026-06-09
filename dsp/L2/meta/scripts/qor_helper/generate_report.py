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
import argparse
import json
import subprocess
import numpy as np
import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots

from models import *
from config_sampler import ConfigSampler


def get_git_hash():
    try:
        result = subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True)
        return result.stdout.strip()
    except Exception:
        return "unknown"


def render_header(ip, chosen_model, train_df, test_df, git_hash, param_cols=None):
    from datetime import datetime  # datetime not needed at module level
    date_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    n_train = len(train_df) if train_df is not None else "N/A"
    n_test  = len(test_df)  if test_df  is not None else "N/A"
    model_type = modelled_params = unmodelled_params = features_in = "N/A"
    if chosen_model:
        model_type = chosen_model.__class__.__name__ if chosen_model else "N/A"
        modelled_cols       = chosen_model.independent_cols
        unmodelled_cols     = [p for p in param_cols if p not in modelled_cols]
        modelled_cols_str   = ', '.join(modelled_cols)
        unmodelled_cols_str = ', '.join(unmodelled_cols) if unmodelled_cols else "None"
        features_processed  = chosen_model.model.n_features_in_
    return f"""
        <h1>QoR Model Report</h1>
        <table>
            <tr><td><b>IP</b></td><td>{ip}</td></tr>
            <tr><td><b>Model</b></td><td>{model_type}</td></tr>
            <tr><td><b>Input features (raw)</b></td><td>{modelled_cols_str}</td></tr>
            <tr><td><b>Non-modelled parameters</b></td><td>{unmodelled_cols_str}</td></tr>
            <tr><td><b>Input features (processed)</b></td><td>{features_processed}</td></tr>
            <tr><td><b>Training samples</b></td><td>{n_train}</td></tr>
            <tr><td><b>Test samples</b></td><td>{n_test}</td></tr>
            <tr><td><b>Generated</b></td><td>{date_str}</td></tr>
            <tr><td><b>Commit</b></td><td>{git_hash}</td></tr>
        </table>
    """


def render_distributions(param_cols, categorical_cols, passed_df=None, failed_func_df=None, no_sim_df=None, not_steady_df=None, test_df=None):
    # --- First pass: compute bins for each parameter ---
    bin_info = {}
    for param in param_cols:
        all_vals = []
        for df in [passed_df, failed_func_df, no_sim_df, not_steady_df, test_df]:
            if df is not None and param in df.columns:
                all_vals.extend(df[param].dropna().tolist())
        if not all_vals:
            bin_info[param] = {'bins': [], 'type': 'individual', 'unique_vals': []}
            continue

        unique_vals = sorted(set(all_vals))
        if param in categorical_cols or len(unique_vals) <= 16:
            bin_info[param] = {'bins': [str(v) for v in unique_vals], 'type': 'individual', 'unique_vals': unique_vals}
        else:
            bin_edges = np.histogram_bin_edges(all_vals, bins=min(len(unique_vals), 20))
            bins = [f"{bin_edges[j]:.2g}" for j in range(len(bin_edges) - 1)]
            bin_info[param] = {'bins': bins, 'type': 'continuous', 'bin_edges': bin_edges}

    column_widths = [max(len(bin_info[p]['bins']), len(p) // 4 + 2) for p in param_cols]
    total_width = sum(column_widths) * 45

    fig = make_subplots(rows=1, cols=len(param_cols), subplot_titles=param_cols,
                        column_widths=column_widths)

    # --- Second pass: add traces ---
    for i, param in enumerate(param_cols):
        info = bin_info[param]
        bins = info['bins']
        show_legend = (i == 0)

        if not bins:
            continue

        if info['type'] == 'individual':
            unique_vals = info['unique_vals']
            def _count(df, uv=unique_vals):
                if df is None or param not in df.columns:
                    return [0] * len(uv)
                vc = df[param].value_counts()
                return [int(vc.get(v, 0)) for v in uv]
        else:
            bin_edges = info['bin_edges']
            def _count(df, edges=bin_edges):
                if df is None or param not in df.columns:
                    return [0] * (len(edges) - 1)
                return np.histogram(df[param].dropna(), bins=edges)[0].tolist()

        passed_counts      = _count(passed_df)
        failed_func_counts = _count(failed_func_df)
        no_sim_counts      = _count(no_sim_df)
        not_steady_counts  = _count(not_steady_df)
        test_counts        = _count(test_df)

        if passed_df is None and failed_func_df is None:
            fig.add_trace(go.Bar(x=bins, y=no_sim_counts, name='Submitted',
                                 marker_color='steelblue', legendgroup='submitted',
                                 showlegend=show_legend), row=1, col=i+1)
        else:
            fig.add_trace(go.Bar(x=bins, y=passed_counts, name='Passed',
                                 marker_color='green', offsetgroup='train',
                                 legendgroup='passed', showlegend=show_legend), row=1, col=i+1)
            fig.add_trace(go.Bar(x=bins, y=failed_func_counts, name='Failed (func)',
                                 marker_color='gold', offsetgroup='train',
                                 legendgroup='failed_func', showlegend=show_legend), row=1, col=i+1)
            fig.add_trace(go.Bar(x=bins, y=not_steady_counts, name='Not steady state',
                                 marker_color='magenta', offsetgroup='train',
                                 legendgroup='not_steady', showlegend=show_legend), row=1, col=i+1)
            fig.add_trace(go.Bar(x=bins, y=no_sim_counts, name='No simulation',
                                 marker_color='red', offsetgroup='train',
                                 legendgroup='no_sim', showlegend=show_legend), row=1, col=i+1)

        if test_df is not None:
            fig.add_trace(go.Bar(x=bins, y=test_counts, name='Test',
                                 marker_color='royalblue', offsetgroup='test',
                                 legendgroup='test', showlegend=show_legend), row=1, col=i+1)

    fig.update_layout(barmode='stack', width=total_width, height=400)

    return f"<hr><h2>Section 1: Data Distributions</h2><div style='overflow-x:auto'>{fig.to_html(full_html=False, include_plotlyjs=False)}</div>"


def render_pattern_analysis(param_cols, passed_df, failed_func_df=None, no_sim_df=None, not_steady_df=None):
    # --- Build combined dataframe with outcome encoding ---
    dfs = []
    if passed_df is not None:
        df_p = passed_df[param_cols].copy()
        df_p['_outcome'] = 2
        dfs.append(df_p)
    if failed_func_df is not None:
        df_f = failed_func_df[param_cols].copy()
        df_f['_outcome'] = 1
        dfs.append(df_f)
    if not_steady_df is not None:
        df_ns = not_steady_df[param_cols].copy()
        df_ns['_outcome'] = 3
        dfs.append(df_ns)
    if no_sim_df is not None:
        df_n = no_sim_df[param_cols].copy()
        df_n['_outcome'] = 0
        dfs.append(df_n)
    combined = pd.concat(dfs).reset_index(drop=True)

    # --- Build dimensions, encoding low-cardinality params as discrete ---
    rng = np.random.default_rng(seed=42)
    dimensions = []
    for param in param_cols:
        if param not in combined.columns:
            continue
        unique_vals = sorted(combined[param].dropna().unique())
        dim = dict(label=param, values=combined[param].tolist())
        if len(unique_vals) <= 16:
            val_to_int = {v: i for i, v in enumerate(unique_vals)}
            combined[param] = combined[param].map(val_to_int)
            if (len(unique_vals) == 1):
                dim['values'] = combined[param].tolist()
            else:
                jittered = combined[param] + rng.normal(0, 0.01, size=len(combined))
                dim['values'] = jittered.tolist()
            dim['tickvals'] = list(range(len(unique_vals)))
            dim['ticktext'] = [str(v) for v in unique_vals]
        dimensions.append(dim)

    # -1 maps to white (hidden), 0=red, 1=gold, 2=green, 3=magenta
    colorscale = [[0, 'white'], [0.25, 'red'], [0.5, 'gold'], [0.75, 'green'], [1.0, 'magenta']]
    colors_all = combined['_outcome'].tolist()

    fig = go.Figure(go.Parcoords(
        line=dict(color=colors_all, colorscale=colorscale, cmin=-1, cmax=3),
        dimensions=dimensions
    ))
    fig.update_layout(height=800)

    div_id = 'parcoords-plot'
    colors_json = json.dumps(colors_all)

    plot_html = fig.to_html(full_html=False, include_plotlyjs=False,
                            div_id=div_id, config={'responsive': True})

    blurb = "<hr><h2>Section 2: Configuration Pattern Analysis</h2><p><i>Drag on any axis to filter configurations. Click and drag on a selection to move it. Double-click an axis to reset.</i></p>"

    checkboxes = f"""
    <div style='margin-bottom:10px'>
        <label style='margin-right:15px'><input type='checkbox' id='show_passed' checked onchange='updateParcoords()'> <span style='color:green'>Passed</span></label>
        <label style='margin-right:15px'><input type='checkbox' id='show_failed' checked onchange='updateParcoords()'> <span style='color:goldenrod'>Failed (func)</span></label>
        <label style='margin-right:15px'><input type='checkbox' id='show_notsteady' checked onchange='updateParcoords()'> <span style='color:magenta'>Not steady state</span></label>
        <label><input type='checkbox' id='show_nosim' checked onchange='updateParcoords()'> <span style='color:red'>No simulation</span></label>
    </div>
    <script>
    const _baseColors = {colors_json};
    function updateParcoords() {{
        const showPassed    = document.getElementById('show_passed').checked;
        const showFailed    = document.getElementById('show_failed').checked;
        const showNotSteady = document.getElementById('show_notsteady').checked;
        const showNosim     = document.getElementById('show_nosim').checked;
        const newColors = _baseColors.map(v => {{
            if (v === 2 && !showPassed)    return -1;
            if (v === 1 && !showFailed)    return -1;
            if (v === 3 && !showNotSteady) return -1;
            if (v === 0 && !showNosim)     return -1;
            return v;
        }});
        Plotly.restyle('{div_id}', {{'line.color': [newColors]}}, [0]);
    }}
    </script>"""

    return f"{blurb}{checkboxes}<div style='width:100%'>{plot_html}</div>"


def render_actual_vs_predicted(train_df, test_df, model_params, chosen_model):
    from sklearn.metrics import r2_score, mean_absolute_percentage_error

    def median_absolute_percentage_error(y_true, y_pred):
        percentage_errors = np.abs((y_true - y_pred) / y_true)
        return np.median(percentage_errors)

    dependent_cols   = model_params["dependent"]
    independent_cols = model_params["categorical"] + model_params["ordinal"]
    hover_cols       = [col for col in test_df.columns if col not in dependent_cols]

    X_train = train_df[independent_cols]
    y_train = train_df[dependent_cols]
    X_test  = test_df[independent_cols]
    y_test  = test_df[dependent_cols]

    baseline = declare_model("linear", model_params, pca_variance=0.999)
    baseline.fit(X_train, y_train)

    # --- Training metrics for chosen model (smoke test) ---
    y_train_pred = chosen_model.predict(X_train)
    train_rows = ''.join(
        f"<tr><td>{dep}</td>"
        f"<td>{r2_score(y_train[dep], y_train_pred[dep]):.3f}</td>"
        f"<td>{mean_absolute_percentage_error(y_train[dep], y_train_pred[dep]) * 100:.1f}%</td>"
        f"<td>{median_absolute_percentage_error(y_train[dep], y_train_pred[dep]) * 100:.1f}%</td></tr>"
        for dep in dependent_cols
    )
    train_table = f"""
        <p><b>{chosen_model.__class__.__name__} — Performance on Training Data (Overfit)</b></p>
        <table border='1' cellpadding='4' cellspacing='0'>
            <tr><th>Metric</th><th>R²</th><th>MAPE</th><th>MdAPE</th></tr>
            {train_rows}
        </table>"""

    colors = ['#636EFA', '#EF553B', '#00CC96', '#AB63FA']

    config_texts = [
        '<br>'.join(f'{c}: {test_df.iloc[i][c]}' for c in hover_cols)
        for i in range(len(test_df))
    ]

    n_deps    = len(dependent_cols)
    plot_size = 600
    models_labels = [
        (baseline,      'Linear Baseline Model on Test Data'),
        (chosen_model,  f'{chosen_model.__class__.__name__} Model on Test Data'),
    ]

    blurb = "<p><i>Axes are in transformed log space. Metrics are in original units.</i></p>"
    figures_html = []
    for model, row_title in models_labels:
        y_pred   = model.predict(X_test)
        y_test_t = np.log(y_test.values + 1)
        y_pred_t = np.log(y_pred.values + 1)

        subplot_titles = [
            f'{dep}<br>R²={r2_score(y_test[dep], y_pred[dep]):.3f}  '
            f'MAPE={mean_absolute_percentage_error(y_test[dep], y_pred[dep]) * 100:.1f}%  '
            f'MdAPE={median_absolute_percentage_error(y_test[dep], y_pred[dep]) * 100:.1f}%'
            for dep in dependent_cols
        ]

        fig = make_subplots(rows=1, cols=n_deps, subplot_titles=subplot_titles)
        for col_idx in range(1, n_deps + 1):
            dep       = dependent_cols[col_idx - 1]
            color     = colors[(col_idx - 1) % len(colors)]
            actual    = y_test_t[:, col_idx - 1]
            predicted = y_pred_t[:, col_idx - 1]
            lim       = [min(actual.min(), predicted.min()), max(actual.max(), predicted.max())]

            hover_texts = [
                f'{config_texts[i]}<br>'
                f'----------<br>'
                f'<b>Predicted:</b> {y_pred[dep].iloc[i]:.2f}<br>'
                f'<b>Actual:</b> {y_test[dep].iloc[i]:.2f}'
                for i in range(len(test_df))
            ]

            fig.add_trace(go.Scatter(
                x=actual, y=predicted, mode='markers',
                marker=dict(size=4, color=color), text=hover_texts,
                hoverinfo='text', showlegend=False
            ), row=1, col=col_idx)

            fig.add_trace(go.Scatter(
                x=lim, y=lim, mode='lines',
                line=dict(color='red', dash='dash'), showlegend=False
            ), row=1, col=col_idx)

        fig.update_xaxes(title_text='Actual')
        fig.update_yaxes(title_text='Predicted')
        fig.update_layout(width=n_deps * plot_size, height=plot_size, margin=dict(t=50))
        figures_html.append(f"<h3>{row_title}</h3>{fig.to_html(full_html=False, include_plotlyjs=False)}")   

    return f"<hr><h2>Section 3: Actual vs Predicted</h2>{train_table}{blurb}{''.join(figures_html)}"


def render_learning_curve(model_params, train_df, test_df, chosen_model, n_partitions, extrapolation_factor):
    from scipy.optimize import curve_fit

    dependent_cols   = model_params["dependent"]
    independent_cols = model_params["categorical"] + model_params["ordinal"]
    colors           = ['#636EFA', '#EF553B', '#00CC96', '#AB63FA']

    X_test = test_df[independent_cols]
    y_test = test_df[dependent_cols]

    n_total         = len(train_df)
    offset          = len(test_df)  # Our smallest train partition must be >= len(test_df).
    partition_sizes = [int(offset + (n_total - offset) * (i + 1) / n_partitions) for i in range(n_partitions)]

    mapes = {dep: [] for dep in dependent_cols}
    for size in partition_sizes:
        subset = train_df.iloc[:size]
        X_preprocessed = chosen_model.preprocessor.transform(subset[independent_cols])
        y_preprocessed = chosen_model.y_process_pipe.transform(subset[dependent_cols]) 
        chosen_model.model.fit(X_preprocessed, y_preprocessed)
        y_pred = chosen_model.predict(X_test)
        for dep in dependent_cols:
            mape = np.mean(np.abs((y_test[dep].values - y_pred[dep].values) / y_test[dep].values)) * 100
            mapes[dep].append(mape)

    def power_law(x, a, b, c):
        return a * np.power(x + c, -b)

    n_deps   = len(dependent_cols)
    fig      = make_subplots(rows=1, cols=n_deps, subplot_titles=dependent_cols)
    x_data   = np.array(partition_sizes)
    x_extrap = np.linspace(partition_sizes[0], extrapolation_factor * partition_sizes[-1], 300)

    for col_idx, dep in enumerate(dependent_cols, start=1):
        color  = colors[(col_idx - 1) % len(colors)]
        y_data = np.array(mapes[dep])

        fig.add_trace(go.Scatter(
            x=x_data.tolist(), y=y_data.tolist(),
            mode='markers+lines',
            marker=dict(size=8, color=color),
            line=dict(color=color),
            showlegend=False
        ), row=1, col=col_idx)

        try:
            bounds = ([0, 0, -partition_sizes[0] + 1], [np.inf, np.inf, np.inf])
            popt, _ = curve_fit(power_law, x_data, y_data, p0=[100, 0.5, 0],
                                bounds=bounds, maxfev=5000)
            y_fit = power_law(x_extrap, *popt)
            mask  = np.isfinite(y_fit) & (y_fit > 0)
            fig.add_trace(go.Scatter(
                x=x_extrap[mask].tolist(), y=y_fit[mask].tolist(),
                mode='lines',
                line=dict(color=color, dash='dash'),
                showlegend=False
            ), row=1, col=col_idx)
        except Exception:
            pass

    plot_size = 600
    fig.update_xaxes(title_text='Training samples')
    fig.update_yaxes(title_text='MAPE (%)', type='log')
    fig.update_layout(width=n_deps * plot_size, height=plot_size, margin=dict(t=50))

    return f"<hr><h2>Section 4: Test Performance against Training Dataset Size</h2>{fig.to_html(full_html=False, include_plotlyjs=False)}"


def write_html(sections, out_file):
    import plotly
    plotly_js = plotly.offline.get_plotlyjs()
    body = "\n".join(s for s in sections if s is not None)
    html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script>{plotly_js}</script>
</head>
<body>
{body}
</body>
</html>"""
    with open(out_file, "w") as f:
        f.write(html)


def main(args):
    with open("data/config_translation.json", "r") as f:
        config_translation = json.load(f)

    with open("data/model_params.json", "r") as f:
        all_model_params = json.load(f)
    model_params = all_model_params[args.ip]
    no_iterate_params = all_model_params["no_iterate_params"]

    categorical_cols = model_params["categorical"]
    ordinal_cols = model_params["ordinal"]
    dependent_cols = model_params["dependent"]
    restricted_params = model_params.get("restricted", {})

    sampler = ConfigSampler(args.ip, restricted_params, ordinal_cols, categorical_cols, no_iterate_params)
    param_cols = sampler.params
    aie_mapper = sampler.aie_mapper

    # --- Load submitted CSV ---
    raw_df = pd.read_csv(args.submitted_csv)
    raw_df.columns = raw_df.columns.str.upper()

    has_metrics = "FUNC" in raw_df.columns

    passed_df, failed_func_df, no_sim_df, not_steady_df, train_df, test_df = None, None, None, None, None, None

    if has_metrics:
        passed_raw      = raw_df[raw_df["FUNC"] == "1"].copy()
        failed_func_raw = raw_df[raw_df["FUNC"] == "0"].copy()
        no_sim_raw      = raw_df[raw_df["FUNC"] == "default"].copy()

        passed_all      = preprocess_dataframe(passed_raw,      sampler, dependent_cols, config_translation, filter_steady_state=False)
        failed_func_all = preprocess_dataframe(failed_func_raw, sampler, dependent_cols, config_translation, filter_steady_state=False)
        no_sim_df       = preprocess_dataframe(no_sim_raw,      sampler, [],             config_translation, filter_steady_state=False)

        passed_not_steady_mask      = (passed_all[dependent_cols]      == -1).any(axis=1)
        failed_func_not_steady_mask = (failed_func_all[dependent_cols] == -1).any(axis=1)

        passed_df      = passed_all[~passed_not_steady_mask]
        failed_func_df = failed_func_all[~failed_func_not_steady_mask]
        not_steady_df  = pd.concat([passed_all[passed_not_steady_mask],
                                    failed_func_all[failed_func_not_steady_mask]]).reset_index(drop=True)

        train_df = pd.concat([passed_df, failed_func_df]).sort_index().reset_index(drop=True)

        if args.test_csv:
            test_raw = pd.read_csv(args.test_csv)
            test_df  = preprocess_dataframe(test_raw, sampler, dependent_cols, config_translation, filter_steady_state=True)
        else:
            split    = int(0.8 * len(train_df))
            test_df  = train_df.iloc[split:].reset_index(drop=True)
            train_df = train_df.iloc[:split].reset_index(drop=True)
    else:
        no_sim_df = preprocess_dataframe(raw_df, sampler, dependent_cols, config_translation)

    # --- Load and refit model on train split ---
    chosen_model = None
    if args.model_file:
        chosen_model = load_model(args.model_file)
        chosen_model.fit(train_df[chosen_model.independent_cols], train_df[chosen_model.dependent_cols])

    # --- Build report ---
    git_hash = get_git_hash()
    sections = []
    sections.append(render_header(args.ip, chosen_model, train_df, test_df, git_hash, param_cols))
    sections.append(render_distributions(param_cols, categorical_cols, passed_df, failed_func_df, no_sim_df, not_steady_df, test_df))
    print("Rendered data distributions...")

    if train_df is not None:
        sections.append(render_pattern_analysis(param_cols, passed_df, failed_func_df, no_sim_df, not_steady_df))
        print("Rendered pattern analysis...")

    if train_df is not None and chosen_model is not None:
        sections.append(render_actual_vs_predicted(train_df, test_df, model_params, chosen_model))
        print("Rendered actual vs predicted...")

    if train_df is not None and chosen_model is not None and args.learning_curve:
        sections.append(render_learning_curve(model_params, train_df, test_df, chosen_model, args.n_partitions, args.extrapolation_factor))
        print("Rendered learning curve...")

    write_html(sections, args.out_file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", required=True)
    parser.add_argument("--submitted_csv", required=True)
    parser.add_argument("--test_csv", default=None)
    parser.add_argument("--model_file", default=None)
    parser.add_argument("--out_file", default="report.html")
    parser.add_argument("--learning_curve", action="store_true", default=False)
    parser.add_argument("--n_partitions", type=int, default=5)
    parser.add_argument("--extrapolation_factor", type=float, default=10.0)
    args = parser.parse_args()
    main(args)
