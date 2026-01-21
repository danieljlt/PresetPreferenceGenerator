#!/usr/bin/env python3
"""
Compute metrics from feedback_dataset.csv

Metrics computed:
1. Time-to-first-like - How many auditions until first like
2. Like rate - Proportion of likes to total feedback
3. Pairwise agreement - % of (liked, disliked) pairs ranked correctly by MLP
4. Rolling prediction error - Mean |prediction - rating| over sliding window
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
    
    required = ['rating', 'sampleIndex', 'mlpPrediction']
    missing = [c for c in required if c not in df.columns]
    if missing:
        raise ValueError(f"Missing columns: {missing}")
    
    return df


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


def pairwise_agreement(df: pd.DataFrame) -> tuple[int, int, float]:
    """
    Return (correct_pairs, total_pairs, rate).
    For all (liked, disliked) pairs, check if MLP predicted liked > disliked.
    """
    likes = df[df['rating'] == 1.0]['mlpPrediction'].values
    dislikes = df[df['rating'] == 0.0]['mlpPrediction'].values
    
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


def rolling_prediction_error(df: pd.DataFrame, window: int = 10) -> pd.Series:
    """Return Series of rolling mean |prediction - rating|."""
    error = (df['mlpPrediction'] - df['rating']).abs()
    return error.rolling(window=window, min_periods=1).mean()


def baseline_comparison(df: pd.DataFrame, n: int = 20) -> dict:
    """Compare first n vs last n samples."""
    if len(df) < 2 * n:
        n = len(df) // 2
    
    first = df.head(n)
    last = df.tail(n)
    
    return {
        'n': n,
        'first': {
            'like_rate': like_rate(first),
            'pairwise': pairwise_agreement(first),
            'mean_error': (first['mlpPrediction'] - first['rating']).abs().mean()
        },
        'last': {
            'like_rate': like_rate(last),
            'pairwise': pairwise_agreement(last),
            'mean_error': (last['mlpPrediction'] - last['rating']).abs().mean()
        }
    }


def plot_rolling_error(df: pd.DataFrame, output_path: Path, window: int = 10):
    """Generate rolling prediction error chart."""
    rolling = rolling_prediction_error(df, window)
    
    plt.figure(figsize=(10, 5))
    plt.plot(df['sampleIndex'], rolling, linewidth=2, color='#2563eb')
    plt.fill_between(df['sampleIndex'], rolling, alpha=0.3, color='#2563eb')
    plt.xlabel('Sample Index', fontsize=12)
    plt.ylabel('Prediction Error', fontsize=12)
    plt.title(f'Rolling Prediction Error (window={window})', fontsize=14)
    plt.grid(True, alpha=0.3)
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


def print_summary(df: pd.DataFrame):
    """Print formatted metrics summary."""
    ttfl = time_to_first_like(df)
    likes, total, lr = like_rate(df)
    correct, pairs, pa = pairwise_agreement(df)
    mean_err = (df['mlpPrediction'] - df['rating']).abs().mean()
    
    print("\nMetrics Summary")
    print("─" * 45)
    print(f"Total samples:         {total}")
    print(f"Time-to-first-like:    {ttfl if ttfl else 'N/A'} auditions")
    print()
    print(f"Like Rate:             {lr*100:.1f}% ({likes}/{total})")
    print(f"Pairwise Agreement:    {pa*100:.1f}% ({correct}/{pairs} pairs)")
    print(f"Mean Prediction Error: {mean_err:.3f}")
    
    # Baseline comparison
    baseline = baseline_comparison(df)
    n = baseline['n']
    
    print(f"\nBaseline Comparison (First {n} vs Last {n})")
    print("─" * 45)
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


def main():
    parser = argparse.ArgumentParser(description='Compute thesis metrics from feedback CSV')
    parser.add_argument('--csv', type=Path, default=DEFAULT_CSV,
                        help='Path to feedback_dataset.csv')
    parser.add_argument('--output', type=Path, default=Path(__file__).parent,
                        help='Output directory for charts')
    parser.add_argument('--no-charts', action='store_true',
                        help='Skip chart generation')
    parser.add_argument('--config', type=str, default=None,
                        help='Filter by configFlags column value (e.g., "baseline", "adaptive+novelty")')
    parser.add_argument('--compare', nargs=2, metavar=('CONFIG1', 'CONFIG2'),
                        help='Compare two configs side by side')
    args = parser.parse_args()
    
    print(f"Loading data from: {args.csv}")
    df = load_data(args.csv)
    df = df.sort_values('sampleIndex').reset_index(drop=True)
    
    if args.compare:
        if 'configFlags' not in df.columns:
            print("Error: configFlags column not found. Cannot compare configs.")
            return
        
        config1, config2 = args.compare
        df1 = df[df['configFlags'] == config1]
        df2 = df[df['configFlags'] == config2]
        
        print(f"\n=== Comparison: {config1} vs {config2} ===")
        print(f"{'':20} {config1:>15} {config2:>15}")
        print("-" * 52)
        
        lr1 = like_rate(df1)[2] if len(df1) > 0 else 0
        lr2 = like_rate(df2)[2] if len(df2) > 0 else 0
        print(f"{'Samples':20} {len(df1):>15} {len(df2):>15}")
        print(f"{'Like Rate':20} {lr1*100:>14.1f}% {lr2*100:>14.1f}%")
        
        pa1 = pairwise_agreement(df1)[2] if len(df1) > 0 else 0
        pa2 = pairwise_agreement(df2)[2] if len(df2) > 0 else 0
        print(f"{'Pairwise Agreement':20} {pa1*100:>14.1f}% {pa2*100:>14.1f}%")
        
        err1 = (df1['mlpPrediction'] - df1['rating']).abs().mean() if len(df1) > 0 else 0
        err2 = (df2['mlpPrediction'] - df2['rating']).abs().mean() if len(df2) > 0 else 0
        print(f"{'Mean Pred Error':20} {err1:>15.3f} {err2:>15.3f}")
        return
    
    if args.config:
        if 'configFlags' not in df.columns:
            print("Warning: configFlags column not found. Showing all data.")
        else:
            df = df[df['configFlags'] == args.config]
            print(f"Filtered to config: {args.config} ({len(df)} samples)")
    
    print_summary(df)
    
    if not args.no_charts:
        suffix = f"_{args.config}" if args.config else ""
        error_path = args.output / f'rolling_prediction_error{suffix}.png'
        rate_path = args.output / f'like_rate_over_time{suffix}.png'
        
        plot_rolling_error(df, error_path)
        plot_like_rate(df, rate_path)
        
        print(f"\nCharts saved to:")
        print(f"  {error_path}")
        print(f"  {rate_path}")


if __name__ == '__main__':
    main()

