#!/usr/bin/env python3
"""
Compute metrics from feedback_dataset.csv

Metrics computed:
1. Time-to-first-like - How many auditions until first like
2. Like rate - Proportion of likes to total feedback
3. Pairwise agreement - % of (liked, disliked) pairs ranked correctly by MLP
4. Rolling prediction error - Mean |prediction - rating| over sliding window

Supports both old (mlpPrediction) and new (mlpGenomePrediction, mlpAudioPrediction) column formats.
"""

import argparse
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from itertools import product


DEFAULT_CSV = Path.home() / "Library/Application Support/PresetPreferenceGenerator/feedback_dataset.csv"


def load_data(csv_path: Path) -> pd.DataFrame:
    """Load and validate the feedback CSV."""
    if not csv_path.exists():
        raise FileNotFoundError(f"CSV not found: {csv_path}")
    
    df = pd.read_csv(csv_path)
    
    required = ['rating', 'sampleIndex']
    missing = [c for c in required if c not in df.columns]
    if missing:
        raise ValueError(f"Missing columns: {missing}")
    
    # Handle column name compatibility
    has_dual = 'mlpGenomePrediction' in df.columns and 'mlpAudioPrediction' in df.columns
    has_legacy = 'mlpPrediction' in df.columns
    
    if not has_dual and not has_legacy:
        raise ValueError("Missing prediction columns: need either 'mlpPrediction' or 'mlpGenomePrediction'/'mlpAudioPrediction'")
    
    return df


def get_prediction_columns(df: pd.DataFrame) -> list[str]:
    """Return list of available prediction column names."""
    cols = []
    if 'mlpGenomePrediction' in df.columns:
        cols.append('mlpGenomePrediction')
    if 'mlpAudioPrediction' in df.columns:
        cols.append('mlpAudioPrediction')
    if 'mlpPrediction' in df.columns and not cols:
        cols.append('mlpPrediction')
    return cols


def time_to_first_like(df: pd.DataFrame) -> int | None:
    """Return sampleIndex of first like, or None if no likes."""
    likes = df[df['rating'] == 1.0]
    if likes.empty:
        return None
    return int(likes['sampleIndex'].min())


def like_rate(df: pd.DataFrame) -> tuple[int, int, float]:
    """Return (like_count, total_count, rate)."""
    likes = int((df['rating'] == 1.0).sum())
    total = len(df)
    rate = likes / total if total > 0 else 0.0
    return likes, total, rate


def pairwise_agreement(df: pd.DataFrame, pred_col: str) -> tuple[int, int, float]:
    """
    Return (correct_pairs, total_pairs, rate).
    For all (liked, disliked) pairs, check if MLP predicted liked > disliked.
    """
    if pred_col not in df.columns:
        return 0, 0, 0.0
    
    likes = df[df['rating'] == 1.0][pred_col].values
    dislikes = df[df['rating'] == 0.0][pred_col].values
    
    if len(likes) == 0 or len(dislikes) == 0:
        return 0, 0, 0.0
    
    correct = 0
    total = 0
    for like_pred, dislike_pred in product(likes, dislikes):
        total += 1
        if like_pred > dislike_pred:
            correct += 1
    
    rate = correct / total if total > 0 else 0.0
    return correct, total, rate


def rolling_prediction_error(df: pd.DataFrame, pred_col: str, window: int = 10) -> pd.Series:
    """Return Series of rolling mean |prediction - rating|."""
    if pred_col not in df.columns:
        return pd.Series([0] * len(df))
    error = (df[pred_col] - df['rating']).abs()
    return error.rolling(window=window, min_periods=1).mean()


def baseline_comparison(df: pd.DataFrame, pred_col: str, n: int = 20) -> dict:
    """Compare first n vs last n samples."""
    if len(df) < 2 * n:
        n = len(df) // 2
    
    first = df.head(n)
    last = df.tail(n)
    
    if pred_col not in df.columns:
        pred_col = 'rating'  # Fallback
    
    return {
        'n': n,
        'first': {
            'like_rate': like_rate(first),
            'pairwise': pairwise_agreement(first, pred_col),
            'mean_error': (first[pred_col] - first['rating']).abs().mean() if pred_col in first.columns else 0
        },
        'last': {
            'like_rate': like_rate(last),
            'pairwise': pairwise_agreement(last, pred_col),
            'mean_error': (last[pred_col] - last['rating']).abs().mean() if pred_col in last.columns else 0
        }
    }


def plot_rolling_error(df: pd.DataFrame, output_path: Path, pred_cols: list[str], window: int = 10):
    """Generate rolling prediction error chart for all prediction columns."""
    plt.figure(figsize=(10, 5))
    
    colors = {'mlpGenomePrediction': '#2563eb', 'mlpAudioPrediction': '#dc2626', 'mlpPrediction': '#2563eb'}
    labels = {'mlpGenomePrediction': 'Genome MLP', 'mlpAudioPrediction': 'Audio MLP', 'mlpPrediction': 'MLP'}
    
    for col in pred_cols:
        rolling = rolling_prediction_error(df, col, window)
        color = colors.get(col, '#2563eb')
        label = labels.get(col, col)
        plt.plot(df['sampleIndex'], rolling, linewidth=2, color=color, label=label)
        plt.fill_between(df['sampleIndex'], rolling, alpha=0.2, color=color)
    
    plt.xlabel('Sample Index', fontsize=12)
    plt.ylabel('Prediction Error', fontsize=12)
    plt.title(f'Rolling Prediction Error (window={window})', fontsize=14)
    plt.grid(True, alpha=0.3)
    if len(pred_cols) > 1:
        plt.legend()
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def plot_like_rate(df: pd.DataFrame, output_path: Path, window: int = 10):
    """Generate rolling like rate chart."""
    rolling = df['rating'].rolling(window=window, min_periods=1).mean()
    
    plt.figure(figsize=(10, 5))
    plt.plot(df['sampleIndex'], rolling, linewidth=2, color='#16a34a')
    plt.fill_between(df['sampleIndex'], rolling, alpha=0.3, color='#16a34a')
    plt.axhline(y=0.5, color='#9ca3af', linestyle='--', linewidth=1)
    plt.xlabel('Sample Index', fontsize=12)
    plt.ylabel('Like Rate', fontsize=12)
    plt.title(f'Rolling Like Rate (window={window})', fontsize=14)
    plt.ylim(0, 1)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def print_summary(df: pd.DataFrame, pred_cols: list[str]):
    """Print formatted metrics summary."""
    ttfl = time_to_first_like(df)
    likes, total, lr = like_rate(df)
    
    print("\nMetrics Summary")
    print("─" * 55)
    print(f"Total samples:         {total}")
    print(f"Time-to-first-like:    {ttfl if ttfl else 'N/A'} auditions")
    print()
    print(f"Like Rate:             {lr*100:.1f}% ({likes}/{total})")
    
    # Print metrics for each prediction column
    for col in pred_cols:
        label = {'mlpGenomePrediction': 'Genome MLP', 'mlpAudioPrediction': 'Audio MLP', 'mlpPrediction': 'MLP'}.get(col, col)
        correct, pairs, pa = pairwise_agreement(df, col)
        mean_err = (df[col] - df['rating']).abs().mean()
        
        print()
        print(f"[{label}]")
        print(f"  Pairwise Agreement:  {pa*100:.1f}% ({correct}/{pairs} pairs)")
        print(f"  Mean Pred Error:     {mean_err:.3f}")
    
    # Baseline comparison for primary prediction column
    primary_col = pred_cols[0] if pred_cols else 'mlpPrediction'
    baseline = baseline_comparison(df, primary_col)
    n = baseline['n']
    
    print(f"\nBaseline Comparison (First {n} vs Last {n}) [{primary_col}]")
    print("─" * 55)
    print(f"{'':20} {'First '+str(n):>12} {'Last '+str(n):>12}")
    
    f_lr = baseline['first']['like_rate'][2]
    l_lr = baseline['last']['like_rate'][2]
    print(f"{'Like Rate':20} {f_lr*100:>11.1f}% {l_lr*100:>11.1f}%")
    
    f_pa = baseline['first']['pairwise'][2]
    l_pa = baseline['last']['pairwise'][2]
    print(f"{'Pairwise Agreement':20} {f_pa*100:>11.1f}% {l_pa*100:>11.1f}%")
    
    f_err = baseline['first']['mean_error']
    l_err = baseline['last']['mean_error']
    print(f"{'Mean Pred Error':20} {f_err:>12.3f} {l_err:>12.3f}")


def print_comparison(df: pd.DataFrame, config1: str, config2: str):
    """Print side-by-side comparison of two configs."""
    if 'configFlags' not in df.columns:
        print("Error: configFlags column not found. Cannot compare configs.")
        return
    
    df1 = df[df['configFlags'] == config1]
    df2 = df[df['configFlags'] == config2]
    pred_cols = get_prediction_columns(df)
    
    print(f"\n=== Comparison: {config1} vs {config2} ===")
    print(f"{'':20} {config1:>15} {config2:>15}")
    print("-" * 52)
    
    print(f"{'Samples':20} {len(df1):>15} {len(df2):>15}")
    
    lr1 = like_rate(df1)[2] if len(df1) > 0 else 0
    lr2 = like_rate(df2)[2] if len(df2) > 0 else 0
    print(f"{'Like Rate':20} {lr1*100:>14.1f}% {lr2*100:>14.1f}%")
    
    for col in pred_cols:
        label = {'mlpGenomePrediction': 'Genome', 'mlpAudioPrediction': 'Audio', 'mlpPrediction': 'MLP'}.get(col, col)
        
        pa1 = pairwise_agreement(df1, col)[2] if len(df1) > 0 else 0
        pa2 = pairwise_agreement(df2, col)[2] if len(df2) > 0 else 0
        print(f"{f'Pairwise ({label})':20} {pa1*100:>14.1f}% {pa2*100:>14.1f}%")
        
        err1 = (df1[col] - df1['rating']).abs().mean() if len(df1) > 0 and col in df1.columns else 0
        err2 = (df2[col] - df2['rating']).abs().mean() if len(df2) > 0 and col in df2.columns else 0
        print(f"{f'Error ({label})':20} {err1:>15.3f} {err2:>15.3f}")


def rolling_pairwise_agreement(df: pd.DataFrame, pred_col: str, window: int = 50) -> pd.Series:
    """
    Calculate pairwise agreement over a rolling window.
    Returns a Series where each value corresponds to the window ending at that index.
    """
    results = []
    
    for i in range(len(df)):
        start = max(0, i - window + 1)
        # We want the window to end at i
        subset = df.iloc[start : i+1]
        
        # Only compute if we have a reasonable amount of data in the window
        # mostly to avoid noise at the very start
        if len(subset) < 5: 
            results.append(float('nan'))
            continue
            
        _, total, rate = pairwise_agreement(subset, pred_col)
        
        if total == 0:
            results.append(float('nan'))
        else:
            results.append(rate)
            
    return pd.Series(results, index=df.index)


def plot_rolling_pairwise(df: pd.DataFrame, output_path: Path, pred_cols: list[str], window: int = 50):
    """Generate rolling pairwise agreement chart."""
    plt.figure(figsize=(10, 5))
    
    colors = {'mlpGenomePrediction': '#2563eb', 'mlpAudioPrediction': '#dc2626', 'mlpPrediction': '#2563eb'}
    labels = {'mlpGenomePrediction': 'Genome MLP', 'mlpAudioPrediction': 'Audio MLP', 'mlpPrediction': 'MLP'}
    
    has_data = False
    for col in pred_cols:
        rolling = rolling_pairwise_agreement(df, col, window)
        # check if not all nan
        if not rolling.dropna().empty:
            has_data = True
            color = colors.get(col, '#2563eb')
            label = labels.get(col, col)
            plt.plot(df['sampleIndex'], rolling, linewidth=2, color=color, label=label)
    
    if not has_data:
        plt.close()
        return

    plt.axhline(y=0.5, color='#9ca3af', linestyle='--', linewidth=1, label='Random Guessing')
    plt.xlabel('Sample Index', fontsize=12)
    plt.ylabel('Pairwise Agreement', fontsize=12)
    plt.title(f'Rolling Pairwise Agreement (window={window})', fontsize=14)
    plt.ylim(0, 1)
    plt.grid(True, alpha=0.3)
    plt.legend(loc='lower right')
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def main():
    parser = argparse.ArgumentParser(description='Compute thesis metrics from feedback CSV')
    parser.add_argument('--csv', type=Path, default=DEFAULT_CSV,
                        help='Path to feedback_dataset.csv')
    parser.add_argument('--output', type=Path, default=Path(__file__).parent,
                        help='Output directory for charts')
    parser.add_argument('--no-charts', action='store_true',
                        help='Skip chart generation')
    parser.add_argument('--config', type=str, default=None,
                        help='Filter by configFlags column value (e.g., "baseline", "audio")')
    parser.add_argument('--compare', nargs=2, metavar=('CONFIG1', 'CONFIG2'),
                        help='Compare two configs side by side')
    parser.add_argument('--input-mode', choices=['genome', 'audio', 'both'], default='both',
                        help='Which MLP predictions to analyze (default: both)')
    args = parser.parse_args()
    
    print(f"Loading data from: {args.csv}")
    df = load_data(args.csv)
    df = df.sort_values('sampleIndex').reset_index(drop=True)
    
    pred_cols = get_prediction_columns(df)
    
    # Filter prediction columns based on --input-mode
    if args.input_mode == 'genome':
        pred_cols = [c for c in pred_cols if 'Genome' in c or c == 'mlpPrediction']
    elif args.input_mode == 'audio':
        pred_cols = [c for c in pred_cols if 'Audio' in c]
    
    if args.compare:
        print_comparison(df, args.compare[0], args.compare[1])
        return
    
    if args.config:
        if 'configFlags' not in df.columns:
            print("Warning: configFlags column not found. Showing all data.")
        else:
            df = df[df['configFlags'] == args.config]
            print(f"Filtered to config: {args.config} ({len(df)} samples)")
    
    print_summary(df, pred_cols)
    
    if not args.no_charts:
        suffix = f"_{args.config}" if args.config else ""
        error_path = args.output / f'rolling_prediction_error{suffix}.png'
        cum_error_path = args.output / f'cumulative_prediction_error{suffix}.png'
        rate_path = args.output / f'like_rate_over_time{suffix}.png'
        
        pairwise_path = args.output / f'rolling_pairwise_agreement{suffix}.png'
        
        plot_rolling_error(df, error_path, pred_cols)
        plot_cumulative_error(df, cum_error_path, pred_cols)
        plot_like_rate(df, rate_path)
        plot_rolling_pairwise(df, pairwise_path, pred_cols)
        
        print(f"\nCharts saved to:")
        print(f"  {error_path}")
        print(f"  {cum_error_path}")
        print(f"  {rate_path}")
        print(f"  {pairwise_path}")


if __name__ == '__main__':
    main()
