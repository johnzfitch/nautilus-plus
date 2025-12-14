#!/bin/bash
# Analyze Nautilus performance logs

LOG_DIR="$HOME/dev/nautilus-plus/performance-logs"

if [ ! -d "$LOG_DIR" ]; then
    echo "No logs found in $LOG_DIR"
    exit 1
fi

echo "=== Nautilus Performance Analysis ==="
echo ""

# Find latest metrics file
LATEST_METRICS=$(ls -t "$LOG_DIR"/metrics_*.csv 2>/dev/null | head -1)

if [ -z "$LATEST_METRICS" ]; then
    echo "No metrics files found"
    exit 1
fi

echo "Analyzing: $LATEST_METRICS"
echo ""

# Memory analysis
echo "=== Memory Analysis ==="
echo "Peak RSS usage:"
awk -F, 'NR>1 {if($3>max) max=$3} END {printf "  %.1f MB\n", max}' "$LATEST_METRICS"

echo "Average RSS usage:"
awk -F, 'NR>1 {sum+=$3; count++} END {if(count>0) printf "  %.1f MB\n", sum/count}' "$LATEST_METRICS"

echo "Memory growth rate:"
awk -F, 'NR>1 {print $3}' "$LATEST_METRICS" | awk '
    NR==1 {first=$1}
    {last=$1}
    END {
        if(NR>1) {
            growth = last - first
            printf "  %.1f MB total growth\n", growth
            if(growth > 0) {
                rate = growth / (NR * 2 / 60)  # Growth per minute (2 sec intervals)
                printf "  %.2f MB/minute\n", rate
            }
        }
    }
'

# Memory leak detection
echo ""
echo "Checking for memory leaks (sustained growth):"
awk -F, 'NR>1 {
    if(prev != "") {
        delta = $3 - prev
        if(delta > 5) positive++
        else if(delta < -5) negative++
    }
    prev = $3
}
END {
    if(positive > negative * 2) {
        print "  ⚠️  POTENTIAL LEAK DETECTED - Memory consistently growing"
        printf "  Positive deltas: %d, Negative deltas: %d\n", positive, negative
    } else {
        print "  ✓ No obvious memory leaks detected"
        printf "  Positive deltas: %d, Negative deltas: %d\n", positive, negative
    }
}' "$LATEST_METRICS"

# CPU analysis
echo ""
echo "=== CPU Analysis ==="
echo "Average CPU usage:"
awk -F, 'NR>1 {sum+=$2; count++} END {if(count>0) printf "  %.1f%%\n", sum/count}' "$LATEST_METRICS"

echo "Peak CPU usage:"
awk -F, 'NR>1 {if($2>max) max=$2} END {printf "  %.1f%%\n", max}' "$LATEST_METRICS"

echo "CPU spikes (>20% increase):"
awk -F, 'NR>1 && $6 > 20 {count++} END {
    if(count > 0) {
        printf "  ⚠️  %d spikes detected\n", count
    } else {
        print "  ✓ No significant spikes"
    }
}' "$LATEST_METRICS"

# Microstutter detection
echo ""
echo "=== Microstutter Detection ==="
awk -F, 'NR>1 {
    if($6 > 10 && $6 < 50) micro++
    else if($6 >= 50) major++
}
END {
    if(micro > 0 || major > 0) {
        printf "  ⚠️  Detected %d microstutters and %d major stutters\n", micro, major
    } else {
        print "  ✓ No stutters detected"
    }
}' "$LATEST_METRICS"

# Thread analysis
echo ""
echo "=== Thread Analysis ==="
echo "Thread count:"
awk -F, 'NR>1 {sum+=$5; count++; if($5>max) max=$5; if(min==0 || $5<min) min=$5}
END {
    if(count>0) {
        printf "  Min: %d, Max: %d, Avg: %.1f\n", min, max, sum/count
    }
}' "$LATEST_METRICS"

# Check alerts
echo ""
echo "=== Recent Alerts ==="
if [ -f "$LOG_DIR/alerts.log" ]; then
    tail -20 "$LOG_DIR/alerts.log"
else
    echo "  No alerts logged"
fi

# Generate recommendations
echo ""
echo "=== Recommendations ==="

# Check for memory leaks
avg_growth=$(awk -F, 'NR>1 {sum+=$7; count++} END {if(count>0) print sum/count; else print 0}' "$LATEST_METRICS")
if (( $(echo "$avg_growth > 1" | bc -l) )); then
    echo "  ⚠️  Consider running valgrind to check for memory leaks"
    echo "     Run: ./monitor-nautilus.sh (option 2)"
fi

# Check for high CPU
avg_cpu=$(awk -F, 'NR>1 {sum+=$2; count++} END {if(count>0) print sum/count; else print 0}' "$LATEST_METRICS")
if (( $(echo "$avg_cpu > 30" | bc -l) )); then
    echo "  ⚠️  High average CPU usage - consider CPU profiling"
    echo "     Run: ./monitor-nautilus.sh (option 4)"
fi

# Check for stutters
stutter_count=$(awk -F, 'NR>1 && $6 > 10 {count++} END {print count+0}' "$LATEST_METRICS")
if [ "$stutter_count" -gt 10 ]; then
    echo "  ⚠️  Frequent CPU spikes detected - may cause UI stuttering"
    echo "     Check debug logs for slow operations"
fi

echo ""
echo "Full metrics available at: $LATEST_METRICS"
