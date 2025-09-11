# OpenDTU-OnBattery GitHub Copilot Instructions

**ALWAYS follow these instructions first and only fallback to additional search and context gathering if the information here is incomplete or found to be in error.**

OpenDTU-OnBattery is an ESP32-based firmware project that provides solar inverter monitoring and battery management with a web interface. The project combines C++ ESP32 firmware with a Vue.js web application that gets embedded into the firmware.

## Essential Setup and Build Process

### Required Dependencies
Install dependencies in this exact order:

```bash
# Install Node.js 22 (REQUIRED - do not use older versions)
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
sudo apt-get install nodejs -y

# Verify Node.js 22 installation
export PATH=/usr/bin:$PATH
node --version  # Should show v22.x.x

# Enable corepack for yarn
sudo corepack enable

# Install Python dependencies
python3 -m pip install --upgrade pip
pip install --upgrade platformio setuptools

# Install C++ linting
pip install cpplint
```

### Build Process (CRITICAL ORDER)
**The webapp MUST be built before firmware compilation. Never skip this step.**

```bash
# Navigate to repository root
cd /path/to/OpenDTU-OnBattery

# Build webapp first (ALWAYS required before firmware)
cd webapp
yarn install --frozen-lockfile  # Takes ~25 seconds
yarn build                       # Takes ~15 seconds - NEVER CANCEL

# Return to repository root for firmware build
cd ..

# Build firmware (requires internet access)
pio run -e generic_esp32s3_usb   # NEVER CANCEL - requires 60+ minute timeout
```

### Timing Expectations and Warnings
- **Webapp dependencies**: ~25 seconds
- **Webapp build**: ~15 seconds - NEVER CANCEL, set timeout to 60+ seconds  
- **Firmware build**: 10-45 minutes on first run - NEVER CANCEL, set timeout to 60+ minutes
- **PlatformIO platform downloads**: 5-15 minutes - NEVER CANCEL, requires internet access

### Network Requirements and Limitations
**CRITICAL**: Firmware builds require internet access for:
- ESP32 platform downloads (espressif32@6.10.0)
- Library dependencies
- Build tools

If you encounter `HTTPClientError` during `pio run`, this indicates network restrictions that prevent PlatformIO from downloading required components. In restricted environments:
- Webapp builds work without internet
- Linting works without internet  
- Firmware builds will fail without internet access

## Development and Validation

### Linting and Code Quality
Run these commands to validate code quality:

```bash
# Webapp linting and formatting
cd webapp
yarn lint                   # Takes ~3 seconds
yarn prettier --check src/  # Takes ~3 seconds

# C++ linting
cd ..
cpplint --repository=. --recursive \
  --filter=-build/c++11,-runtime/references,-readability/braces,-whitespace,-legal,-build/include \
  ./src ./include ./lib/Hoymiles ./lib/MqttSubscribeParser ./lib/TimeoutHelper ./lib/ResetReason
# Takes ~10 seconds
```

### Available Build Environments
Default CI environments (use any of these with `pio run -e <environment>`):
- `generic_esp32_4mb_no_ota` - 4MB ESP32, no OTA support
- `generic_esp32_8mb` - 8MB ESP32 with OTA
- `generic_esp32s3` - ESP32-S3 
- `generic_esp32s3_usb` - ESP32-S3 with USB (default)

### Webapp Development
The webapp can be developed independently:

```bash
cd webapp
yarn dev      # Start development server (if network available)
yarn preview  # Preview production build on port 4173
```

## Project Structure and Key Files

### Repository Layout
```
├── .github/workflows/     # CI/CD pipelines
├── webapp/               # Vue.js web application  
│   ├── src/             # Vue.js source code
│   ├── package.json     # Node.js dependencies
│   └── yarn.lock        # Lockfile for dependencies
├── src/                 # C++ ESP32 firmware source
├── include/             # C++ header files
├── lib/                 # External C++ libraries
├── pio-scripts/         # PlatformIO build scripts
├── platformio.ini       # PlatformIO configuration
└── webapp_dist/         # Built webapp (created by yarn build)
```

### Critical Files
- `platformio.ini`: ESP32 environments and build configuration
- `webapp/package.json`: Node.js scripts and dependencies
- `pio-scripts/compile_webapp.py`: Automatically rebuilds webapp if needed
- `src/main.cpp`: ESP32 firmware entry point

## Validation and Testing

### Manual Validation Steps
After making changes, ALWAYS run this complete validation sequence:

```bash
# 1. Validate webapp
cd webapp
yarn install --frozen-lockfile
yarn lint
yarn prettier --check src/
yarn build

# 2. Validate C++ code
cd ..
cpplint --repository=. --recursive \
  --filter=-build/c++11,-runtime/references,-readability/braces,-whitespace,-legal,-build/include \
  ./src ./include ./lib/Hoymiles ./lib/MqttSubscribeParser ./lib/TimeoutHelper ./lib/ResetReason

# 3. Attempt firmware build (may fail in restricted networks)
pio run -e generic_esp32s3_usb
```

### CI/CD Validation
The project uses these automated workflows:
- **build.yml**: Full firmware builds for all environments
- **cpplint.yml**: C++ code linting
- **yarnlint.yml**: JavaScript/TypeScript linting
- **yarnprettier.yml**: Code formatting validation

### No Unit Tests
**Important**: This project currently has no unit tests. The `test/` directory exists but is empty except for documentation.

## Common Issues and Troubleshooting

### Build Failures
1. **"HTTPClientError" during pio run**: Ensure unrestricted internet access is available for ESP32 platform downloads. Network firewalls or restrictions will cause firmware builds to fail. Contact repository administrators to configure network access if builds fail with network errors.
2. **"yarn: command not found"**: Run `sudo corepack enable`
3. **"Node version too old"**: Ensure Node.js 22 is installed and in PATH
4. **"webapp_dist not found"**: Run `yarn build` in webapp directory first

### Development Workflow
1. Make code changes
2. Run validation commands
3. Build webapp (`yarn build`)
4. Test firmware compilation (`pio run -e <environment>`)
5. Commit only after all validations pass

### Hardware Notes
- Project targets ESP32, ESP32-S3, and ESP32-C3 microcontrollers
- Supports various development boards (see platformio.ini environments)
- Embedded web interface is served from ESP32 flash memory
- Different memory configurations supported (4MB, 8MB, 16MB)

## Key Integrations and Features
- **Solar Inverter Communication**: Hoymiles protocol support
- **Battery Management**: Multiple BMS types (JK BMS, Pylontech, etc.)
- **Power Limiting**: Dynamic power control based on consumption
- **Web Interface**: Real-time monitoring and configuration
- **MQTT Integration**: Home automation compatibility
- **Multiple Hardware Variants**: Various ESP32 board configurations

Always reference these instructions to avoid common pitfalls and ensure consistent development practices.