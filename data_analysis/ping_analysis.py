import re
import pandas as pd
import matplotlib.pyplot as plt

def parse_ping_data(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()
    
    data = []
    for line in lines:
        match = re.search(r'\[(\d+\.\d+)\] 64 bytes from (\S+): icmp_seq=(\d+) ttl=\d+ time=(\d+) ms', line)
        if match:
            timestamp, ip, seq, time_ms = match.groups()
            data.append({
                'timestamp': float(timestamp),
                'ip': ip,
                'seq': int(seq),
                'time_ms': float(time_ms)
            })
        else:
            # If no match, assume it's a failed ping
            timestamp_match = re.search(r'\[(\d+\.\d+)\]', line)
            if timestamp_match:
                timestamp = float(timestamp_match.group(1))
                data.append({
                    'timestamp': timestamp,
                    'ip': None,
                    'seq': None,
                    'time_ms': None
                })
    
    return pd.DataFrame(data)

def calculate_metrics(df):
    successful_pings = len(df)
    total_pings = df.iat[-1, 2]
    delivery_rate = successful_pings / total_pings if total_pings > 0 else 0
    average_time = df['time_ms'].mean()
    min_time = df['time_ms'].min()
    max_time = df['time_ms'].max()
    
    # Longest consecutive successful pings
    df['success'] = 1
    df['consecutive'] = (df['seq'].diff() != 1).cumsum()
    longest_success = df.groupby('consecutive')['success'].transform('sum').max()
    
    # Longest consecutive failures
    all_seqs = set(range(total_pings))
    success_seqs = set(df['seq'])
    failure_seqs = all_seqs - success_seqs
    failure_df = pd.DataFrame(list(failure_seqs), columns=['seq']).sort_values(by='seq')
    failure_df['consecutive'] = (failure_df['seq'].diff() != 1).cumsum()
    longest_failure = failure_df.groupby('consecutive')['seq'].transform('count').max()

    # Conditional probabilities
    df['prev_seq'] = df['seq'].shift()
    df['prev_time_ms'] = df['time_ms'].shift()
    
    # Calculate P(success | success)
    success_given_success = df[(df['prev_seq'] + 1 == df['seq']) & (df['prev_time_ms'].notna())]
    total_success_following_success = df[df['prev_time_ms'].notna()].shape[0]
    prob_success_given_success = success_given_success.shape[0] / total_success_following_success if total_success_following_success > 0 else 0
    
    # Calculate P(success | failure)
    success_given_failure = df[(df['prev_seq'] + 1 == df['seq']) & (df['prev_time_ms'].isna())]
    total_failures = total_pings - successful_pings
    prob_success_given_failure = success_given_failure.shape[0] / total_failures if total_failures > 0 else 0
    
    return {
        'total_pings': total_pings,
        'successful_pings': successful_pings,
        'delivery_rate': delivery_rate,
        'average_time': average_time,
        'min_time': min_time,
        'max_time': max_time,
        'longest_success': longest_success,
        'longest_failure': longest_failure,
        'prob_success_given_success': prob_success_given_success,
        'prob_success_given_failure': prob_success_given_failure
    }

def plot_rtt_over_time(df):
    plt.figure(figsize=(14, 7))
    plt.plot(df['timestamp'], df['time_ms'], marker='o', linestyle='-', markersize=2)
    plt.xlabel('time')
    plt.ylabel('RTT (ms)')
    plt.title('RTT over time')
    plt.grid(True)
    plt.savefig('rtt_over_time.png')
    plt.show()

def plot_rtt_distribution(df):
    plt.figure(figsize=(14, 7))
    plt.hist(df['time_ms'].dropna(), bins=50, alpha=0.7, density=True)
    plt.xlabel('RTT (ms)')
    plt.ylabel('density')
    plt.title('RTT distribution')
    #plt.legend()
    
    plt.twinx()
    df['time_ms'].dropna().plot(kind='kde', color='red', label='KDE')
    plt.ylabel('KDE density')
    plt.legend(loc='upper right')
    
    plt.savefig('rtt_distribution.png')
    plt.show()

def plot_rtt_correlation(df):
    plt.figure(figsize=(14, 7))
    plt.scatter(df['prev_time_ms'], df['time_ms'], alpha=0.5)
    plt.xlabel('last RTT (ms)')
    plt.ylabel('current RTT (ms)')
    plt.title('RTT correlation')
    plt.grid(True)
    plt.savefig('rtt_correlation.png')
    plt.show()

def summarize_results(metrics):
    print("结论:")
    print(f"总ping次数: {metrics['total_pings']}")
    print(f"成功ping次数: {metrics['successful_pings']}")
    print(f"网络路径的整体交付率为 {metrics['delivery_rate'] * 100:.2f}%。")
    print(f"最长连续成功ping为 {metrics['longest_success']} 次。")
    print(f"最长连续失败ping为 {metrics['longest_failure']} 次。")
    print(f"给定成功后下一次成功的概率为 {metrics['prob_success_given_success'] * 100:.2f}%。")
    print(f"给定失败后下一次成功的概率为 {metrics['prob_success_given_failure'] * 100:.2f}%。")
    print(f"平均延迟: {metrics['average_time']:.2f} ms")
    print(f"最小延迟为 {metrics['min_time']:.2f} 毫秒。")
    print(f"最大延迟为 {metrics['max_time']:.2f} 毫秒。")

def main():
    file_path = 'data.txt'
    df = parse_ping_data(file_path)
    metrics = calculate_metrics(df)
    
    plot_rtt_over_time(df)
    plot_rtt_distribution(df)
    plot_rtt_correlation(df)
    
    summarize_results(metrics)

if __name__ == "__main__":
    main()