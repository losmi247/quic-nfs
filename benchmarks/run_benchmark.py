import subprocess
import re
import os
import argparse
import numpy as np
import matplotlib.pyplot as plt

PI_NET_INTERFACE = "eth0"

BENCHMARK_NAME = ""
FIO_FILE_PATH = ""

TRAFFIC_SETTING_UNIT = ""
TRAFFIC_SETTING = ""
TRAFFIC_SETTING_VALUES = []

def parse_command_line_args():
    global BENCHMARK_NAME, FIO_FILE_PATH, TRAFFIC_SETTING_UNIT, TRAFFIC_SETTING, TRAFFIC_SETTING_VALUES

    parser = argparse.ArgumentParser(description="Run fio bennchmarks with packet loss and protocol selection")

    parser.add_argument("--benchmark_name", required=True, help="Specify the benchmark name")

    parser.add_argument("--fio_file_path", required=True, help="Specify the fio job file to use")

    parser.add_argument("--traffic_setting_unit", choices = ["%", "ms"], required=True, help="Specify the fio unit for the traffic setting")
    parser.add_argument("--traffic_setting", choices = ["loss", "latency"], required=True, help="Select the traffic setting for the test")
    parser.add_argument("--traffic_setting_values", nargs="+", type=int, required=True, help="Specify the traffic setting values")

    args = parser.parse_args()

    BENCHMARK_NAME = args.benchmark_name
    FIO_FILE_PATH = args.fio_file_path

    TRAFFIC_SETTING_UNIT = args.traffic_setting_unit
    TRAFFIC_SETTING = args.traffic_setting
    TRAFFIC_SETTING_VALUES = args.traffic_setting_values

def create_logging_directories():
    os.makedirs(f"./benchmarks/logs/{BENCHMARK_NAME}/tcp", exist_ok=True)
    os.makedirs(f"./benchmarks/logs/{BENCHMARK_NAME}/quic", exist_ok=True)

def get_mean(values):
    return np.mean(values) if values else 0

def get_std_dev(values):
    return np.std(values, ddof=1) if len(values) > 1 else 0

def run_benchmark(benchmark_name, fio_file_path, traffic_setting, traffic_setting_values, proto, num_runs = 10):
    iops_means = []
    bandwidth_means = []
    iops_std_devs = []
    bandwidth_std_dev = []

    subprocess.run(["sudo", "tc", "qdisc", "del", "dev", PI_NET_INTERFACE, "root"], check=False)

    fio_filename = os.path.basename(fio_file_path)

    for traffic_setting_value in traffic_setting_values:
        print(f"Starting {traffic_setting}={traffic_setting_value}:")

        log_directory = f"./benchmarks/logs/{benchmark_name}/{proto}/{fio_filename}__{traffic_setting_value}_{traffic_setting}_{proto}"
        os.makedirs(log_directory, exist_ok=True)

        iops_values = []
        bandwidth_values = []

        if traffic_setting_value > 0:
            if traffic_setting == "loss":
                subprocess.run(["sudo", "tc", "qdisc", "add", "dev", PI_NET_INTERFACE, "root", "netem", "loss", "random", f"{traffic_setting_value}%"], check=True)
            else:
                subprocess.run(["sudo", "tc", "qdisc", "add", "dev", PI_NET_INTERFACE, "root", "netem", "delay", f"{traffic_setting_value}ms", "5ms"], check=True)

        for i in range(1, num_runs + 1):
            log_file = f"{log_directory}/{fio_filename}__{traffic_setting_value}_{traffic_setting}_{proto}_{i}.log"

            # run fio and capture output
            subprocess.run(["fio", fio_file_path, "--output=" + log_file], check=True)

            # read log file and extract IOPS
            with open(log_file, "r") as f:
                content = f.read()

                match_iops = re.search(r"IOPS=\s*(\d+)", content)
                if match_iops:
                    iops_values.append(int(match_iops.group(1)))
                else:
                    print("Failed to extract IOPS from fio logs")

                match_bandwidth = re.search(r"BW=\s*([\d.]+)KiB/s", content)
                if match_bandwidth:
                    bandwidth_values.append(float(match_bandwidth.group(1)))
                else:
                    match_bandwidth = re.search(r"BW=\s*([\d.]+)B/s", content)
                    if match_bandwidth:
                        bandwidth_values.append(float(match_bandwidth.group(1)) / 1024)
                    else:
                        print("Failed to extract bandwidth from fio logs")  

        # compute average and standard deviation
        iops_means.append(get_mean(iops_values))
        iops_std_devs.append(get_std_dev(iops_values))

        bandwidth_means.append(get_mean(bandwidth_values))
        bandwidth_std_dev.append(get_std_dev(bandwidth_values))

        # progress logs
        step_report = f"{traffic_setting}={traffic_setting_value}, Avg IOPS={iops_means[-1]}, Std Dev={iops_std_devs[-1]}, Avg BW={bandwidth_means[-1]}, Std Dev={bandwidth_std_dev[-1]}"
        print(step_report)
        with open(f"{log_directory}/results.txt", "a") as f:
            f.write(step_report)

        # reset traffic setting
        if traffic_setting_value > 0:
            subprocess.run(["sudo", "tc", "qdisc", "del", "dev", PI_NET_INTERFACE, "root"], check=True)

        print(f"Finished loss {traffic_setting}={traffic_setting_value}: Avg IOPS={iops_means[-1]}, Avg BW={bandwidth_means[-1]}\n")

    return iops_means, iops_std_devs, bandwidth_means, bandwidth_std_dev

def plot_results(benchmark_name, traffic_setting, traffic_setting_unit, traffic_setting_values, tcp_iops_means, tcp_iops_std_devs, tcp_bandwidth_means, tcp_bandwidth_std_dev, quic_iops_means, quic_iops_std_devs, quic_bandwidth_means, quic_bandwidth_std_dev, num_runs = 10):
    # plot IOPS
    plt.figure(figsize=(8,6))
    plt.errorbar(traffic_setting_values, tcp_bandwidth_means, yerr=tcp_bandwidth_std_dev, fmt='o-', capsize=5, label="TCP Bandwidth", color="blue")
    plt.errorbar(traffic_setting_values, quic_bandwidth_means, yerr=quic_bandwidth_std_dev, fmt='o-', capsize=5, label="QUIC Bandwidth", color="red")

    plt.xlabel(f"Simulated {traffic_setting} [{traffic_setting_unit}]")
    plt.ylabel(f"Average IOPS over {num_runs} runs")
    plt.title(f"IOPS for TCP/QUIC vs Simulated {traffic_setting} with Standard Deviation")
    plt.legend()
    plt.grid(True)
    # Save the plot
    plt.savefig(f"./benchmarks/graphs/{benchmark_name}_iops.png")
    plt.show()

    # plot Bandwidth
    plt.figure(figsize=(8,6))
    plt.errorbar(traffic_setting_values, tcp_iops_means, yerr=tcp_iops_std_devs, fmt='o-', capsize=5, label="TCP IOPS", color="blue")
    plt.errorbar(traffic_setting_values, quic_iops_means, yerr=quic_iops_std_devs, fmt='o-', capsize=5, label="QUIC IOPS", color="red")

    plt.xlabel(f"Simulated {traffic_setting} [{traffic_setting_unit}]")
    plt.ylabel(f"Average Bandwidth over {num_runs} runs")
    plt.title(f"Bandwidth for TCP/QUIC vs Simulated {traffic_setting} with Standard Deviation")
    plt.legend()
    plt.grid(True)
    # Save the plot
    plt.savefig(f"./benchmarks/graphs/{benchmark_name}_bw.png")
    plt.show()

def main():
    parse_command_line_args()
    create_logging_directories()

    print("Running benchmark for TCP:\n")
    tcp_iops_means, tcp_iops_std_devs, tcp_bandwidth_means, tcp_bandwidth_std_dev = run_benchmark(BENCHMARK_NAME, f"{FIO_FILE_PATH}_tcp.fio", TRAFFIC_SETTING, TRAFFIC_SETTING_VALUES, proto="tcp")
    print("\n\nRunning benchmark for QUIC:\n")
    quic_iops_means, quic_iops_std_devs, quic_bandwidth_means, quic_bandwidth_std_dev = run_benchmark(BENCHMARK_NAME, f"{FIO_FILE_PATH}_quic.fio", TRAFFIC_SETTING, TRAFFIC_SETTING_VALUES, proto="quic")

    plot_results(BENCHMARK_NAME, TRAFFIC_SETTING, TRAFFIC_SETTING_UNIT, TRAFFIC_SETTING_VALUES, tcp_iops_means, tcp_iops_std_devs, tcp_bandwidth_means, tcp_bandwidth_std_dev, quic_iops_means, quic_iops_std_devs, quic_bandwidth_means, quic_bandwidth_std_dev)

if __name__ == "__main__":
    main()
    
