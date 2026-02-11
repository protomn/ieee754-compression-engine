import argparse
import numpy as np
import pandas as pd
from datetime import datetime, timedelta
import sys
import yfinance as yf
import os

def fetch_data(symbol: str, days: int, interval: str = '1m') -> pd.DataFrame:

    """
    Fetch data for required symbol from yfinance

    Args:
        - symbol: Ticker
        - days: Number of days of historical data
        - interval: '1m' (default)

    Retuns: 
        - pd.DataFrame with columns timestamp, price
    """

    print(f"\nFetching data for {symbol} from yfinance.\n")

    if interval == '1m':

        days = min(days, 14) #Fetch data at 1 minute intervals over the past 14 days.

    ticker = yf.Ticker(symbol)
    df = ticker.history(period = f"{days}d", interval = interval)

    if df.empty:
        print(f"No data returned for ticker {symbol}.")
        sys.exit(1)

    result = pd.DataFrame({
        'timestamp': df.index,
        'price': df['Close'].values
    })

    print(f"Fetched {len(result)} data points.")
    return result

def simulate_data(symbol: str, ticks: int,
                  start_price: float = 100.0, 
                  volatility: float = 0.02) -> pd.DataFrame:
    
    print(f"\nSimulating {ticks:,} realistic ticks for ticker {symbol}.\n")
    print(f"Starting price: ${start_price:.2f}")
    print(f"\nPrice Volatility: {volatility}\n")

    np.random.seed(42) #Reproducibility

    #Generating one tick per 100ms with variance
    baseTime = np.datetime64('2024-01-02T09:30:00')
    timeDelta = np.random.exponential(100, ticks).astype('timedelta64[ms]')
    cum_deltas = np.cumsum(timeDelta)
    timestamps = baseTime + cum_deltas
    
    prices = [start_price]
    discrete_noise = np.random.choice([-0.01, 0, 0.01], size = ticks, p = [0.1, 0.8, 0.1])
    change = np.random.normal(0, volatility * 0.01, ticks)

    prices = start_price + np.cumsum(change + discrete_noise)
    prices = prices.round(2)

    results = pd.DataFrame({
        'timestamp': timestamps,
        'price': prices
    })

    print(f"\nGenerated {len(results)} ticks.")
    print(f"\nPrice range: ${min(prices):.2f} - ${max(prices):.2f}")
    print(f"\nConsecutive Identical Values: {sum(1 for i in range(1, len(prices)) if prices[i] == prices[i - 1])} ({100 * sum(1 for i in range(len(prices)) if prices[i] == prices[i - 1])/len(prices):.1f}%)")

    return results

def add_noise(df: pd.DataFrame, bid_ask_spread: float = 0.01) -> pd.DataFrame:

    """
    Add bid-ask bounce to simulate market microstructure

    Args:
        - df: DataFrame with price column
        - bid_ask_spread: spread 

    Returns:
        Modified DataFrame with bid-ask bounce
    """

    side = np.random.choice([-0.5, 0.5], size = len(df))
    df['price'] = df['price'] + (bid_ask_spread) * side
    df['price'] = df['price'].round(2)

    return df

def export_csv(df: pd.DataFrame, filename: str, include_timestamp: bool = True):

    """
    Export to CSV in format compatible with C++ parser

    Format:
        - with timestamp: timestamp,price
        - without timestamp: price

    Args:
        - df: pd.DataFrame with time and price columns
        - filename: output csv filename
        - include_timestamp: whether to include timesstamp column
    """

    script_dir = os.path.dirname(os.path.abspath(__file__))

    data_dir = os.path.normpath(os.path.join(script_dir, '..', 'data'))

    if not os.path.exists(data_dir):
        os.makedirs(data_dir)

    full_path = os.path.join(data_dir, filename)

    if include_timestamp:
        
        df.to_csv(full_path, columns = ['timestamp', 'price'],
                  index = False, float_format = '%.6f')
        

        print("\nFormat: timestamp,price")

    else:
        
        df[['price']].to_csv(full_path, index = False,
                             header = True, float_format = '%.6f')
        
        print("\nFormat: price")

    print(f"\nExported {len(df)} rows.")

    print("\n First 5 rows: ")
    
    if include_timestamp:
        print(df[['timestamp', 'price']].head().to_string(index = False))
    else:
        print(df[['price']].head().to_string(index = False))

    
def stats(df: pd.DataFrame):

    prices = df['price'].values

    print("\nDATA STATISTICS\n")
    print(f"\nTotal ticks: {len(prices):,}")
    print(f"\nPrice range: ${prices.min():.2f} - ${prices.max():.2f}")
    print(f"\nMean price: ${prices.mean():.2f}")
    print(f"\nStd deviation: ${prices.std():.4f}")

    changes = np.diff(prices)
    identical = np.sum(changes == 0)

    print(f"\nConsecutive changes:")
    print(f"\nIdentical values: {identical:,} ({100 * identical / len(changes):.1f}%)")
    print(f"\nChanged values: {len(changes)-identical:,} ({100 * (len(changes) - identical) / len(changes):.1f}%)")
    print(f"\nMean absolute change: ${abs(changes[changes != 0]).mean():.4f}")

    print(f"\nChange distribution:")
    print(f"\n+/-$0.01: {sum(abs(changes) <= 0.01):,} ({100*sum(abs(changes) <= 0.01) / len(changes):.1f}%)")
    print(f"\n+/-$0.05: {sum(abs(changes) <= 0.05):,} ({100*sum(abs(changes) <= 0.05) / len(changes):.1f}%)")
    print(f"\n+/-$0.10: {sum(abs(changes) <= 0.10):,} ({100*sum(abs(changes) <= 0.10) / len(changes):.1f}%)")

    if 'timestamp' in df.columns:

        timeDeltas = df['timestamp'].diff().dropna()
        avgDelta = timeDeltas.mean()
        print(f"\nTiming:")
        print(f"\nTime span: {df['timestamp'].iloc[-1] - df['timestamp'].iloc[0]}")
        print(f"\nAvg tick interval: {avgDelta}")

def main():

    parser = argparse.ArgumentParser(
        description = 'Fetch financial data for compression testing',
        formatter_class = argparse.RawDescriptionHelpFormatter,
        epilog = """
                Examples:
                # Yahoo Finance data (free, no API key)
                python fetch_data.py --source yahoo --symbol SPY --days 5
                python fetch_data.py --source yahoo --symbol AAPL --days 1 --interval 1m
  
                # Simulated realistic ticks (best for large datasets)
                python fetch_data.py --source simulate --symbol SPY --ticks 1000000
                python fetch_data.py --source simulate --ticks 10000000 --volatility 0.01
  
                # Alpha Vantage (requires free API key from alphavantage.co)
                python fetch_data.py --source alphavantage --symbol TSLA --api-key YOUR_KEY
                """
    )

    parser.add_argument('--source', choices = ['yahoo', 'simulate'],
                       default = 'simulate', help = 'Data source')
    
    parser.add_argument('--symbol', default = 'SPY', 
                       help = 'Stock symbol (default: SPY)')
    
    parser.add_argument('--days', type = int, default = 5,
                       help = 'Days of historical data (for yahoo/alphavantage)')
    
    parser.add_argument('--interval', default = '1m',
                       choices = ['1m', '5m', '15m', '1h'],
                       help = 'Data interval for yahoo (default: 1m)')
    
    parser.add_argument('--ticks', type = int, default = 1000000,
                       help = 'Number of ticks to simulate (default: 1M)')
    
    parser.add_argument('--volatility', type = float, default = 0.02,
                       help = 'Volatility for simulation (default: 0.02)')
    
    parser.add_argument('--start-price', type = float, default = 100.0,
                       help = 'Starting price for simulation (default: 100.0)')
    
    parser.add_argument('--api-key', help = 'API key for Alpha Vantage')

    parser.add_argument('--output', default = 'market_data.csv',
                       help = 'Output CSV filename')
    
    parser.add_argument('--no-timestamp', action = 'store_true',
                       help = 'Export price only (no timestamp column)')
    
    parser.add_argument('--add-noise', action = 'store_true',
                       help = 'Add bid-ask bounce microstructure noise')
    
    args = parser.parse_args()

    if args.source == 'yahoo':
        df = fetch_data(args.symbol, args.days, args.interval)

    else:  # simulate
        df = simulate_data(args.symbol, args.ticks, 
                                     args.start_price, args.volatility)
        
    if args.add_noise:
        df = add_noise(df)
    
    # Print statistics
    stats(df)
    
    # Export
    export_csv(df, args.output, not args.no_timestamp)
    
if __name__ == '__main__':
    main()