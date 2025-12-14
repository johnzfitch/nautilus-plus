#!/bin/bash
# Nautilus Performance Monitoring Script
# Monitors CPU, memory, and provides detailed profiling

set -e

NAUTILUS_BIN="/usr/local/bin/nautilus"
LOG_DIR="$HOME/dev/nautilus-plus/performance-logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "$LOG_DIR"

echo "=== Nautilus Performance Monitor ==="
echo "Logs will be saved to: $LOG_DIR"
echo ""

# Function to show menu
show_menu() {
    echo "Select monitoring mode:"
    echo "1) Normal run with debug output"
    echo "2) Memory leak detection (valgrind - slow)"
    echo "3) Memory profiling (heaptrack)"
    echo "4) CPU profiling (perf)"
    echo "5) Continuous resource monitoring (lightweight)"
    echo "6) Full profiling suite (background monitoring)"
    echo "q) Quit"
    echo -n "Choice: "
}

# Function to kill any running nautilus instances
kill_nautilus() {
    echo "Stopping any running Nautilus instances..."
    killall nautilus 2>/dev/null || true
    sleep 1
}

# 1. Normal run with debug output
run_debug() {
    local log_file="$LOG_DIR/debug_${TIMESTAMP}.log"
    echo "Starting Nautilus with debug output..."
    echo "Log file: $log_file"
    echo ""

    kill_nautilus

    G_MESSAGES_DEBUG=all \
    G_DEBUG=gc-friendly \
    NAUTILUS_DEBUG=all \
    "$NAUTILUS_BIN" --new-window 2>&1 | tee "$log_file"
}

# 2. Memory leak detection with valgrind
run_valgrind() {
    local log_file="$LOG_DIR/valgrind_${TIMESTAMP}.log"
    echo "Starting Nautilus with Valgrind (this will be SLOW)..."
    echo "Log file: $log_file"
    echo ""

    kill_nautilus

    G_SLICE=always-malloc \
    G_DEBUG=gc-friendly \
    valgrind \
        --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --log-file="$log_file" \
        --num-callers=40 \
        --suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
        --suppressions=/usr/share/gtk-4.0/valgrind/gtk.supp \
        "$NAUTILUS_BIN" --new-window

    echo ""
    echo "Valgrind log saved to: $log_file"
    echo "Checking for leaks..."
    grep -A 5 "LEAK SUMMARY" "$log_file" || echo "No leak summary found yet"
}

# 3. Memory profiling with heaptrack
run_heaptrack() {
    local output_file="$LOG_DIR/heaptrack_${TIMESTAMP}"
    echo "Starting Nautilus with heaptrack..."
    echo "Output: $output_file"
    echo ""

    kill_nautilus

    heaptrack -o "$output_file" "$NAUTILUS_BIN" --new-window

    echo ""
    echo "Heaptrack data saved. Analyzing..."
    heaptrack --analyze "$output_file".gz | tee "$output_file"_analysis.txt
}

# 4. CPU profiling with perf
run_perf() {
    local output_file="$LOG_DIR/perf_${TIMESTAMP}.data"
    echo "Starting Nautilus with perf..."
    echo "Output: $output_file"
    echo "Press Ctrl+C when done profiling"
    echo ""

    kill_nautilus

    # Start nautilus in background
    "$NAUTILUS_BIN" --new-window &
    local nautilus_pid=$!
    sleep 2

    echo "Recording performance data (PID: $nautilus_pid)..."
    sudo -A perf record -F 99 -p $nautilus_pid -g -o "$output_file" -- sleep 60

    echo ""
    echo "Generating report..."
    sudo -A perf report -i "$output_file" --stdio > "$LOG_DIR/perf_${TIMESTAMP}_report.txt"

    echo "Perf report saved to: $LOG_DIR/perf_${TIMESTAMP}_report.txt"
    echo "View with: sudo perf report -i $output_file"
}

# 5. Continuous resource monitoring
run_resource_monitor() {
    local log_file="$LOG_DIR/resources_${TIMESTAMP}.csv"
    echo "Starting continuous resource monitoring..."
    echo "Log file: $log_file"
    echo "Press Ctrl+C to stop monitoring"
    echo ""

    # Write CSV header
    echo "timestamp,cpu_percent,mem_rss_mb,mem_vsz_mb,threads,io_read_mb,io_write_mb" > "$log_file"

    while true; do
        # Find nautilus process
        local pid=$(pgrep -x nautilus | head -1)

        if [ -z "$pid" ]; then
            echo "$(date +%Y-%m-%d\ %H:%M:%S) - Nautilus not running, waiting..." | tee -a "$log_file"
            sleep 5
            continue
        fi

        # Get process stats
        local timestamp=$(date +%Y-%m-%d\ %H:%M:%S)
        local stats=$(ps -p $pid -o %cpu,rss,vsz,nlwp --no-headers)
        local cpu=$(echo $stats | awk '{print $1}')
        local rss=$(echo $stats | awk '{print $2/1024}')  # Convert to MB
        local vsz=$(echo $stats | awk '{print $3/1024}')  # Convert to MB
        local threads=$(echo $stats | awk '{print $4}')

        # Get I/O stats
        local io_stats=$(cat /proc/$pid/io 2>/dev/null || echo "rchar: 0 wchar: 0")
        local io_read=$(echo "$io_stats" | grep rchar | awk '{print $2/1024/1024}')  # Convert to MB
        local io_write=$(echo "$io_stats" | grep wchar | awk '{print $2/1024/1024}')  # Convert to MB

        # Log to CSV
        echo "$timestamp,$cpu,$rss,$vsz,$threads,$io_read,$io_write" >> "$log_file"

        # Display current stats
        printf "\r[%s] CPU: %5.1f%% | RSS: %7.1f MB | VSZ: %7.1f MB | Threads: %3d | I/O R/W: %.1f/%.1f MB" \
               "$timestamp" "$cpu" "$rss" "$vsz" "$threads" "$io_read" "$io_write"

        sleep 2
    done
}

# 6. Full profiling suite (background)
run_full_suite() {
    echo "Starting full profiling suite..."
    echo "This will:"
    echo "  1. Start nautilus with debug logging"
    echo "  2. Monitor resources in background"
    echo "  3. Run for 5 minutes"
    echo ""

    kill_nautilus

    # Start resource monitoring in background
    local resource_log="$LOG_DIR/resources_${TIMESTAMP}.csv"
    echo "timestamp,cpu_percent,mem_rss_mb,mem_vsz_mb,threads" > "$resource_log"

    (
        while true; do
            local pid=$(pgrep -x nautilus | head -1)
            if [ -n "$pid" ]; then
                local timestamp=$(date +%Y-%m-%d\ %H:%M:%S)
                local stats=$(ps -p $pid -o %cpu,rss,vsz,nlwp --no-headers)
                echo "$timestamp,$(echo $stats | sed 's/ /,/g' | awk -F, '{print $1","$2/1024","$3/1024","$4}')" >> "$resource_log"
            fi
            sleep 1
        done
    ) &
    local monitor_pid=$!

    # Start nautilus with debug output
    G_MESSAGES_DEBUG=all \
    G_DEBUG=gc-friendly \
    NAUTILUS_DEBUG=all \
    "$NAUTILUS_BIN" --new-window > "$LOG_DIR/debug_${TIMESTAMP}.log" 2>&1 &
    local nautilus_pid=$!

    echo "Nautilus PID: $nautilus_pid"
    echo "Monitor PID: $monitor_pid"
    echo ""
    echo "Monitoring for 5 minutes. Use nautilus normally to generate activity."
    echo "Logs are being written to $LOG_DIR/"
    echo ""
    echo "Press Enter to stop monitoring early, or wait 5 minutes..."

    read -t 300 || true

    echo ""
    echo "Stopping monitors..."
    kill $monitor_pid 2>/dev/null || true

    echo ""
    echo "=== Monitoring Summary ==="
    echo "Debug log: $LOG_DIR/debug_${TIMESTAMP}.log"
    echo "Resource log: $resource_log"
    echo ""

    if [ -f "$resource_log" ]; then
        echo "Peak memory usage:"
        awk -F, 'NR>1 {print $3}' "$resource_log" | sort -n | tail -1 | xargs printf "  RSS: %.1f MB\n"
        echo "Average CPU usage:"
        awk -F, 'NR>1 {sum+=$2; count++} END {if(count>0) printf "  %.1f%%\n", sum/count}' "$resource_log"
    fi
}

# Main menu loop
while true; do
    show_menu
    read -r choice

    case $choice in
        1) run_debug ;;
        2) run_valgrind ;;
        3) run_heaptrack ;;
        4) run_perf ;;
        5) run_resource_monitor ;;
        6) run_full_suite ;;
        q|Q) echo "Exiting..."; exit 0 ;;
        *) echo "Invalid choice" ;;
    esac

    echo ""
    echo "---"
    echo ""
done
