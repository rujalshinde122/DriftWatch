# 🛡️ DriftWatch

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=flat-square&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Tests](https://img.shields.io/badge/tests-56%20passing-brightgreen.svg?style=flat-square)]()
[![Build](https://img.shields.io/badge/build-CMake%20%7C%20Make-orange.svg?style=flat-square)]()
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey.svg?style=flat-square)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square)](LICENSE)

> **A lightweight, high-performance C++17 observability daemon and real-time log anomaly detector.**  
> Built from scratch with zero runtime bloat — featuring non-blocking stream ingestion, sliding-window statistical analysis ($Z$-Score anomaly detection), multi-sink alert dispatching, and an interactive terminal dashboard.

---

## 🌟 Overview

DriftWatch continuously tails log streams in real-time, builds a moving statistical baseline of event frequencies, and automatically flags incidents using **rolling $Z$-score anomaly detection** ($Z = \frac{x - \mu}{\sigma}$).

When an anomalous error spike occurs (e.g., database connection pool exhaustion or network timeouts), DriftWatch immediately calculates the statistical deviation, renders a live frequency sparkline, and dispatches structured alerts to console banners, persistent alert log files, or webhooks.

---

## 🏗️ Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                              DriftWatch                                │
│                                                                        │
│   ┌──────────────┐     ┌──────────────┐     ┌──────────────────────┐  │
│   │  LogWatcher  │────▶│ EventParser  │────▶│     MetricEngine     │  │
│   │  (tail -f)   │     │ (regex rules)│     │ (sliding time window)│  │
│   └──────────────┘     └──────────────┘     └──────────┬───────────┘  │
│                                                        │               │
│                                                ┌───────▼───────────┐  │
│                                                │  AnomalyDetector  │  │
│                                                │  (rolling z-score)│  │
│                                                └───────┬───────────┘  │
│                                                        │               │
│                        ┌───────────────────────────────┴──────────┐   │
│                        ▼                                          ▼   │
│               ┌─────────────────┐                       ┌───────────┐  │
│               │   AlertEngine   │                       │TUIRenderer│  │
│               │ (file / console)│                       │(live dash)│  │
│               └─────────────────┘                       └───────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Key Features

- ⚡ **Zero-Lag Stream Ingestion**: Non-blocking, incremental file-pointer seeking with automatic detection of log rotation and file truncation.
- 🧠 **Statistical Anomaly Detection**:
  - Dynamically calculates rolling mean ($\mu$) and standard deviation ($\sigma$).
  - Evaluates $Z$-scores to distinguish expected traffic variations from true outages.
  - Zero-variance safeguards and minimum event thresholds prevent false positives during quiet baselines.
- 🖥️ **Live Interactive TUI Dashboard**:
  - **Live Log Feed**: Real-time scrolling event stream color-coded by log level (`INFO`, `WARN`, `ERROR`).
  - **Error Rate Visualizer**: Unicode sparkline bar chart (`_ ▂▃▄▅▆▇█`) tracking recent anomaly windows.
  - **Incident Log**: Live view of recent alerts with $Z$-scores, observed counts, and timestamps.
- ⚙️ **Configurable Multi-Sink Alerting**: JSON configuration with support for console banners, persistent alert file logging (`alerts.log`), and webhook integration.
- 🧪 **Built-in Mock Traffic Simulator**: Generates realistic web server logs and automatically injects incident bursts for zero-dependency demos.
- 📦 **Daemon Mode**: Run headless in the background via `--daemon` for production-like environments.

---

## 📊 How the Anomaly Math Works

1. **Sliding Time Window**: `MetricEngine` maintains recent events in an in-memory window (default: 30 seconds). Older entries are pruned automatically.
2. **Historical Sampling**: Every evaluation cycle (default: 2s), `AnomalyDetector` samples the current error count and maintains the last $K$ window counts in history.
3. **Z-Score Calculation**:
   $$\mu = \frac{1}{N} \sum_{i=1}^N x_i, \quad \sigma = \sqrt{\frac{1}{N} \sum_{i=1}^N (x_i - \mu)^2}$$
   $$Z = \frac{x_{\text{observed}} - \mu}{\sigma}$$
4. **Trigger Condition**: An alert is fired only when:
   - $Z \ge \text{threshold}$ (default: `2.5`, indicating a 99%+ statistical deviation)
   - $x_{\text{observed}} \ge \text{minEventCount}$ (default: `5`, guards against alerting on isolated single errors)
   - $\sigma > 0$ (guards against division by zero during uniform baseline activity)

---

## 🛠️ Build & Installation

### Requirements
- **C++17** compatible compiler (`clang++` or `g++`)
- **CMake 3.15+** or **GNU Make**
- **nlohmann-json** (`brew install nlohmann-json` on macOS or `apt-get install nlohmann-json3-dev` on Ubuntu/Debian)

### Quick Build with CMake
```bash
cmake -B build
cmake --build build
```

*(Alternatively, build using the included Makefile: `make`)*

---

## 🏃 Usage

### 1. Interactive Demo Mode (Built-in Simulator + Live TUI)
```bash
./build/driftwatch --mock
```
Runs the built-in traffic simulator and displays the real-time TUI dashboard. Watch the error sparklines spike and alerts fire as incident bursts are injected!

### 2. Headless Daemon Mode
```bash
./build/driftwatch --mock --daemon
```
Runs headless without the TUI, streaming structured events to `stdout` and appending alerts to `alerts.log`.

### 3. Monitoring a Real Log File
```bash
./build/driftwatch /var/log/system.log
# Or an Nginx access log:
./build/driftwatch /var/log/nginx/access.log
```

### 4. Custom Configuration File
```bash
./build/driftwatch --config custom_config.json
```

### CLI Reference
```
Usage:
  driftwatch [options] [log_file]

Options:
  --mock             Run built-in log generator and watch simulated traffic
  --daemon           Run headless in daemon mode (disable interactive TUI)
  --config <path>    Path to JSON configuration file (default: ./config.json)
  -v, --version      Display version information
  -h, --help         Display this help message
```

---

## ⚙️ Configuration (`config.json`)

```json
{
  "watch_files": [
    "/tmp/driftwatch_mock.log"
  ],
  "anomaly_threshold_z": 2.5,
  "min_event_count": 5,
  "window_seconds": 30,
  "history_depth": 10,
  "alert_sinks": {
    "console": true,
    "file": "alerts.log",
    "webhook": ""
  }
}
```

| Parameter | Type | Default | Description |
|:---|:---:|:---:|:---|
| `watch_files` | Array | `["/tmp/..."]` | Paths of target log files to continuously tail |
| `anomaly_threshold_z` | Float | `2.5` | Minimum $Z$-score deviation required to trigger an alert |
| `min_event_count` | Integer | `5` | Minimum observed error count required before evaluating alerts |
| `window_seconds` | Integer | `30` | Duration of the sliding time window for active metrics |
| `history_depth` | Integer | `10` | Number of historical evaluation cycles used for $\mu$ and $\sigma$ baselines |
| `alert_sinks.console` | Boolean | `true` | Display highlighted incident banners in console/daemon mode |
| `alert_sinks.file` | String | `"alerts.log"` | Persistent append-only file destination for structured alerts |
| `alert_sinks.webhook` | String | `""` | Webhook HTTP endpoint for remote notification dispatch |

---

## 🧪 Testing

DriftWatch includes 56 unit and integration tests covering parser regex, time-window sliding metrics, statistical calculations, alert guards, and configuration serialization:

```bash
# Run tests with CTest
ctest --test-dir build --output-on-failure

# Or run tests using Make
make test
```

### Test Coverage Highlights:
- ✅ **Log Ingestion & Stream Parser** (`tests/test_module2.cpp`): Parsing valid and invalid log patterns, whitespace flexibility, ISO timestamp extraction, thread-safe mock generator.
- ✅ **Metrics Engine & Anomaly Detector** (`tests/test_module3.cpp`): In-memory time-window retention, sliding eviction, statistical mean/variance calculation, sudden spike detection, zero-variance baseline handling, minimum count suppression.
- ✅ **Configuration, Alert Dispatcher & TUI** (`tests/test_module4_5.cpp`): JSON config serialization/deserialization, fallback defaults, multi-sink alert file persistence, TUI thread-safe ring buffers.

---

## 📁 Project Structure

```
DriftWatch/
├── CMakeLists.txt              # Standard C++17 build & CTest configuration
├── Makefile                    # Developer shortcut for build and testing
├── config.json                 # Default configuration file
├── README.md                   # Project documentation
├── src/
│   ├── main.cpp                # CLI entry point, signal handling & pipeline wiring
│   ├── LogEvent.h              # Structured log event definition
│   ├── Alert.h                 # Anomaly alert definition
│   ├── Config.h                # Configuration data structures
│   ├── LogWatcher.h / .cpp     # Real-time file tailing engine
│   ├── EventParser.h / .cpp    # Regex-based log parser
│   ├── MockLogGenerator.h / .cpp # Background incident traffic simulator
│   ├── MetricEngine.h / .cpp   # Thread-safe sliding time-window metrics
│   ├── AnomalyDetector.h / .cpp # Statistical baseline & Z-score detector
│   ├── ConfigLoader.h / .cpp   # JSON configuration loader
│   ├── AlertEngine.h / .cpp    # Multi-sink alert dispatcher
│   └── TUIRenderer.h / .cpp    # Terminal UI dashboard with live charts
└── tests/
    ├── test_module2.cpp        # Tests for EventParser & MockGenerator
    ├── test_module3.cpp        # Tests for MetricEngine & AnomalyDetector
    └── test_module4_5.cpp      # Tests for ConfigLoader, AlertEngine & TUI
```

---

## 💡 Key Engineering Challenges & Technical Deep Dive

- **Non-blocking Systems File I/O & Inode/Truncation Tracking**:
  - `LogWatcher` implements continuous, non-blocking file streaming by seeking to EOF on startup, incrementally reading newly appended lines, and resetting stream flags (`clear()`).
  - Automatically detects log truncation (e.g. `> logfile`) and rotation by tracking file size shrinks, seeking back to offset 0 without dropping subsequent stream events.
- **Asynchronous Concurrency & Thread Isolation**:
  - DriftWatch decouples log ingestion, mock traffic injection, statistical evaluation, and UI rendering across dedicated threads.
  - State synchronization is managed using fine-grained `std::mutex` locks, thread-safe ring buffers, and clean cooperative shutdown through `std::atomic<bool>`.
- **Dynamic Statistical Baselines vs. Static Thresholds**:
  - Traditional static alerting rules (e.g. `errors > 10`) break down during normal traffic peaks or lull periods. DriftWatch tracks the rolling mean ($\mu$) and standard deviation ($\sigma$) over a configurable historical window.
  - Triggers alerts using the Standard Score ($Z \ge 2.5$), with safeguards for zero-variance periods ($\sigma = 0$) and minimum sample volume gates to prevent alert fatigue.
- **Zero Runtime Overhead & Modern C++17 Design**:
  - Zero external runtime libraries — only header-only JSON (`nlohmann-json`) for configuration.
  - Strict adherence to RAII, move semantics, and the Single Responsibility Principle across isolated components (`LogWatcher`, `EventParser`, `MetricEngine`, `AnomalyDetector`, `AlertEngine`, `TUIRenderer`).

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
