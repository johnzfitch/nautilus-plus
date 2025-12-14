# Nautilus Performance Monitoring

## Active Monitoring

A background watchdog is currently monitoring Nautilus for:
- **CPU spikes** (microstutters)
- **Memory leaks** (sustained growth)
- **High resource usage**
- **Performance degradation**

### Current Status

Check the watchdog status:
```bash
tail -f ~/dev/nautilus-plus/performance-logs/watchdog.out
```

View recent alerts:
```bash
tail -f ~/dev/nautilus-plus/performance-logs/alerts.log
```

## Monitoring Tools

### 1. Watchdog (Lightweight Background Monitor)
**Already running!** Monitors CPU, memory, and detects issues in real-time.

```bash
# View live metrics
tail -f ~/dev/nautilus-plus/performance-logs/metrics_$(date +%Y%m%d).csv

# Stop watchdog
pkill -f nautilus-watchdog
```

**Alerts triggered when:**
- CPU > 80% for 10+ seconds
- Memory grows > 50MB in 30 seconds
- CPU spikes > 10% suddenly (microstutters)
- RSS memory > 1GB

### 2. Full Monitoring Suite
Interactive script with multiple profiling options:

```bash
cd ~/dev/nautilus-plus
./monitor-nautilus.sh
```

**Options:**
1. **Normal run with debug output** - Detailed logs of all operations
2. **Memory leak detection (valgrind)** - Comprehensive leak checking (SLOW)
3. **Memory profiling (heaptrack)** - Detailed memory allocation analysis
4. **CPU profiling (perf)** - CPU hotspot identification
5. **Continuous resource monitoring** - Real-time stats display
6. **Full profiling suite** - All-in-one 5-minute monitoring session

### 3. Performance Analysis
Analyze collected metrics:

```bash
cd ~/dev/nautilus-plus
./analyze-performance.sh
```

**Reports:**
- Memory usage patterns and leak detection
- CPU usage statistics and spike analysis
- Microstutter detection
- Thread count analysis
- Recommendations for further investigation

## Log Files

All logs stored in: `~/dev/nautilus-plus/performance-logs/`

- `metrics_YYYYMMDD.csv` - Time-series performance data
- `alerts.log` - Performance issues and warnings
- `debug_*.log` - Detailed debug output
- `valgrind_*.log` - Memory leak reports
- `perf_*.data` - CPU profiling data
- `heaptrack_*` - Memory profiling data

## Interpreting Results

### Memory Leaks
**Symptoms:**
- Steadily increasing RSS memory over time
- More positive memory deltas than negative
- Alert: "POTENTIAL LEAK DETECTED"

**Action:**
```bash
./monitor-nautilus.sh  # Choose option 2 (valgrind)
# Use nautilus for 2-3 minutes, then close
# Check valgrind log for "definitely lost" blocks
```

### Microstutters
**Symptoms:**
- Frequent CPU spikes in metrics
- UI feels choppy during file operations
- Alert: "CPU spike detected"

**Action:**
```bash
./monitor-nautilus.sh  # Choose option 4 (perf)
# Reproduce the stuttering operation
# Check perf report for hot functions
```

### High CPU Usage
**Symptoms:**
- Average CPU > 30%
- Fans spinning up
- Alert: "High CPU usage"

**Common causes:**
- Thumbnail generation for large directories
- Search operations on slow FUSE mounts
- Excessive file monitoring

**Action:**
Check debug logs for slow operations:
```bash
grep -E "search|thumbnail|FUSE" performance-logs/debug_*.log
```

### Memory Growth
**Symptoms:**
- Alert: "Significant memory growth"
- RSS increasing > 50MB quickly

**Common causes:**
- Thumbnail cache not being freed
- File info cache growing unbounded
- Search results not cleaned up

## Quick Health Check

Run this anytime to see current status:

```bash
# Is watchdog running?
pgrep -f nautilus-watchdog && echo "Watchdog: RUNNING" || echo "Watchdog: STOPPED"

# Latest metrics summary
tail -5 ~/dev/nautilus-plus/performance-logs/metrics_$(date +%Y%m%d).csv

# Recent alerts
tail -10 ~/dev/nautilus-plus/performance-logs/alerts.log
```

## Custom Features Performance Notes

### Search-cache Integration
- Very fast but can spike CPU on large directories
- Monitor: CPU spikes during search operations

### Animated Thumbnails
- Higher memory usage for storing animation frames
- Monitor: Memory growth when viewing image directories

### FUSE Mount Detection
- Prevents hangs but adds latency checking
- Monitor: CPU usage during directory enumeration

### Search Result Limiting
- Default limit: 1000 results
- Prevents crash but may hide results
- Adjust: `gsettings set org.gnome.nautilus.preferences search-results-limit 2000`

## Stopping All Monitoring

```bash
# Stop watchdog
pkill -f nautilus-watchdog

# Stop any profiling tools
killall valgrind heaptrack
sudo pkill perf
```

## Troubleshooting

### Watchdog not collecting data
```bash
# Check if nautilus is running
pgrep nautilus

# Restart watchdog
pkill -f nautilus-watchdog
~/dev/nautilus-plus/nautilus-watchdog.sh &
```

### Can't analyze - no CSV files
```bash
# Check log directory
ls -lh ~/dev/nautilus-plus/performance-logs/

# Ensure watchdog has write permissions
chmod +w ~/dev/nautilus-plus/performance-logs/
```

## Best Practices

1. **Let it run** - Monitor for at least 30 minutes of normal usage
2. **Reproduce issues** - If you notice stuttering, do the same operation again while monitoring
3. **Compare baselines** - Check metrics after a fresh start vs. after hours of use
4. **Check alerts daily** - Review `alerts.log` for patterns

## Example Workflow

**Investigating a suspected memory leak:**

1. Start fresh monitoring:
   ```bash
   pkill -f nautilus-watchdog
   rm ~/dev/nautilus-plus/performance-logs/metrics_*.csv
   ~/dev/nautilus-plus/nautilus-watchdog.sh &
   ```

2. Use Nautilus normally for 1 hour

3. Analyze:
   ```bash
   ./analyze-performance.sh
   ```

4. If leak suspected, run valgrind:
   ```bash
   ./monitor-nautilus.sh  # Option 2
   ```

5. Review results and file bug report with logs
