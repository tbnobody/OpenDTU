# Makefile for OpenDTU-OnBattery tests

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# Test executable
TEST_EXEC = test_overscaling

.PHONY: all clean test help

all: $(TEST_EXEC)

# Only build if source file is newer than executable
$(TEST_EXEC): test_overscaling.cpp ../src/OverscalingCalculator.cpp
	$(CXX) $(CXXFLAGS) -I../include -o $@ $< ../src/OverscalingCalculator.cpp

test: $(TEST_EXEC)
	@echo "Running overscaling bug fix tests..."
	./$(TEST_EXEC)

clean:
	rm -f $(TEST_EXEC)

help:
	@echo "Available targets:"
	@echo "  all    - Build test executable"
	@echo "  test   - Build and run tests"
	@echo "  clean  - Remove test executable"
	@echo "  help   - Show this help"
	@echo ""
	@echo "This test verifies the fix for PowerLimiterOverscalingInverter.cpp"
	@echo "Bug: Was using current shading state instead of new shading state"
	@echo "Fix: Now uses new shading state for overscaling calculation"
