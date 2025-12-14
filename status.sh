#!/bin/bash
# Quick status check for Nautilus monitoring

echo "=== Nautilus Performance Monitor Status ==="
echo ""

# Check if watchdog is running
if pgrep -f nautilus-watchdog > /dev/null; then
    watchdog_pid=$(pgrep -f nautilus-watchdog)
    echo "✓ Watchdog: RUNNING (PID: $watchdog_pid)"
else
    echo "✗ Watchdog: STOPPED"
fi

# Check if nautilus is running
if pgrep -x nautilus > /dev/null; then
    nautilus_pid=$(pgrep -x nautilus | head -1)
    echo "✓ Nautilus: RUNNING (PID: $nautilus_pid)"

    # Get current stats
    stats=$(ps -p $nautilus_pid -o %cpu,rss,nlwp --no-headers 2>/dev/null)
    if [ -n "$stats" ]; then
        cpu=$(echo $stats | awk '{print $1}')
        rss=$(echo $stats | awk '{print $2/1024}')
        threads=$(echo $stats | awk '{print $3}')
        printf "  CPU: %.1f%% | Memory: %.1f MB | Threads: %d\n" "$cpu" "$rss" "$threads"
    fi
else
    echo "✗ Nautilus: NOT RUNNING"
fi

echo ""

# Check for recent metrics
metrics_file="$HOME/dev/nautilus-plus/performance-logs/metrics_$(date +%Y%m%d).csv"
if [ -f "$metrics_file" ]; then
    line_count=$(wc -l < "$metrics_file")
    echo "📊 Metrics collected: $((line_count - 1)) data points"

    # Show last 3 entries
    echo ""
    echo "Recent metrics:"
    tail -3 "$metrics_file" | awk -F, '{
        printf "  [%s] CPU: %5.1f%% | RSS: %7.1f MB | Threads: %2d\n", $1, $2, $3, $5
    }'
else
    echo "📊 No metrics collected yet"
fi

echo ""

# Check for alerts
alerts_file="$HOME/dev/nautilus-plus/performance-logs/alerts.log"
if [ -f "$alerts_file" ]; then
    alert_count=$(grep -c "ALERT" "$alerts_file" 2>/dev/null)
    alert_count=${alert_count:-0}
    if [ "$alert_count" -gt 0 ]; then
        echo "⚠️  Alerts: $alert_count issue(s) detected"
        echo ""
        echo "Recent alerts:"
        grep "ALERT" "$alerts_file" | tail -3
    else
        echo "✓ No alerts"
    fi
else
    echo "✓ No alerts"
fi

echo ""
echo "Commands:"
echo "  View live metrics:  tail -f $metrics_file"
echo "  Run analysis:       ~/dev/nautilus-plus/analyze-performance.sh"
echo "  Full monitoring:    ~/dev/nautilus-plus/monitor-nautilus.sh"
