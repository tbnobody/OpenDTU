# OpenDTU-OnBattery Overscaling Calculator Tests

This directory contains unit tests for the `OverscalingCalculator` class.

## Building and Running Tests

```bash
# Build and run all tests
make test

# Build only
make all

# Clean up
make clean
```

## Test Coverage

The tests cover various overscaling scenarios:
- No shading scenarios
- Single and multiple MPPT shading
- Increasing and decreasing power scenarios
- Realistic partial shading with non-zero values
- Edge cases and boundary conditions

## GitHub Workflow

Tests run automatically on GitHub when test files or the OverscalingCalculator are modified.
