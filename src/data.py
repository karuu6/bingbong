import os
import sys
import pytz
import argparse
from pprint import pprint
from datetime import datetime, timedelta

def str2datetime(inp):
    tz = pytz.timezone('America/Chicago')
    date = datetime.strptime(inp, '%Y-%m-%d')
    start = tz.localize(date.replace(hour=8, minute=30))
    end = tz.localize(date.replace(hour=14, minute=59))
    return start, end

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='MD',
        description='Fetch historical data'
    )
    parser.add_argument('username')
    parser.add_argument('password')
    parser.add_argument('ticker')
    parser.add_argument('date')
    args = parser.parse_args()

    start, end = str2datetime(args.date)

    ex = f'./bin/md {args.username} {args.password} CME {args.ticker} '
    ex += f'{start.timestamp():.0f} {end.timestamp():.0f} > data/{args.ticker}_{end.date()}.csv'
    
    rp = os.system(ex)
    sys.exit(rp)
