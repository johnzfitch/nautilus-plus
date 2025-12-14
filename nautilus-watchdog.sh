#!/bin/bash
# Lightweight background monitor for Nautilus
# Detects performance issues, memory leaks, and microstutters

LOG_DIR="$HOME/dev/nautilus-plus/performance-logs"
ALERT_LOG="$LOG_DIR/alerts.log"
METRICS_LOG="$LOG_DIR/metrics_$(date +%Y%m%d).csv"

mkdir -p "$LOG_DIR"

# Thresholds
CPU_THRESHOLD=80        # Alert if CPU > 80% for extended period
MEM_GROWTH_THRESHOLD=50 # Alert if memory grows by 50MB in 30 seconds
STUTTER_THRESHOLD=10    # Alert if CPU spikes > 10% suddenly

# Initialize metrics log
if [ ! -f "$METRICS_LOG" ]; then
    echo "timestamp,cpu,rss_mb,vsz_mb,threads,cpu_delta,mem_delta" > "$METRICS_LOG"
fi

echo "[$(date)] Nautilus Watchdog started" | tee -a "$ALERT_LOG"
echo "Monitoring for performance issues..."
echo "Alerts will be logged to: $ALERT_LOG"
echo "Metrics will be logged to: $METRICS_LOG"
echo ""

# Previous values for delta calculation
prev_cpu=0
prev_mem=0
high_cpu_count=0

while true; do
    # Find nautilus process
    pid=$(pgrep -x nautilus | head -1)

    if [ -z "$pid" ]; then
        sleep 5
        continue
    fi

    # Get current stats
    timestamp=$(date +%Y-%m-%d\ %H:%M:%S)
    stats=$(ps -p $pid -o %cpu,rss,vsz,nlwp --no-headers 2>/dev/null)

    if [ -z "$stats" ]; then
        sleep 2
        continue
    fi

    cpu=$(echo $stats | awk '{print $1}')
    rss=$(echo $stats | awk '{print $2/1024}')  # MB
    vsz=$(echo $stats | awk '{print $3/1024}')  # MB
    threads=$(echo $stats | awk '{print $4}')

    # Calculate deltas
    cpu_delta=$(awk "BEGIN {print $cpu - $prev_cpu}")
    mem_delta=$(awk "BEGIN {print $rss - $prev_mem}")

    # Log metrics
    echo "$timestamp,$cpu,$rss,$vsz,$threads,$cpu_delta,$mem_delta" >> "$METRICS_LOG"

    # Check for high CPU usage
    if (( $(echo "$cpu > $CPU_THRESHOLD" | bc -l) )); then
        ((high_cpu_count++))
        if [ $high_cpu_count -ge 5 ]; then  # High for 10 seconds
            echo "[ALERT] [$timestamp] High CPU usage: ${cpu}% (threshold: ${CPU_THRESHOLD}%)" | tee -a "$ALERT_LOG"
            high_cpu_count=0
        fi
    else
        high_cpu_count=0
    fi

    # Check for CPU spikes (microstutters)
    if (( $(echo "$cpu_delta > $STUTTER_THRESHOLD" | bc -l) )) && [ $prev_cpu != 0 ]; then
        echo "[ALERT] [$timestamp] CPU spike detected: ${prev_cpu}% -> ${cpu}% (delta: ${cpu_delta}%)" | tee -a "$ALERT_LOG"
    fi

    # Check for memory growth
    if (( $(echo "$mem_delta > $MEM_GROWTH_THRESHOLD" | bc -l) )) && [ $prev_mem != 0 ]; then
        echo "[ALERT] [$timestamp] Significant memory growth: ${prev_mem}MB -> ${rss}MB (delta: ${mem_delta}MB)" | tee -a "$ALERT_LOG"
    fi

    # Check for excessive memory usage (>1GB RSS)
    if (( $(echo "$rss > 1024" | bc -l) )); then
        echo "[ALERT] [$timestamp] High memory usage: ${rss}MB" | tee -a "$ALERT_LOG"
    fi

    # Display current status
    printf "\r[%s] CPU: %5.1f%% (%+.1f) | RSS: %7.1f MB (%+.1f) | Threads: %3d" \
           "$(date +%H:%M:%S)" "$cpu" "$cpu_delta" "$rss" "$mem_delta" "$threads"

    prev_cpu=$cpu
    prev_mem=$rss

    sleep 2
done
