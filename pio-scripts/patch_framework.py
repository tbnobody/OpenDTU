# SPDX-License-Identifier: GPL-2.0-or-later
#
# Patch framework files that are outside the git repository.
# Uses string replacement instead of git apply.
#
import os
import hashlib

Import("env")

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoespressif32")

MARKER = "// OPENDTU_PATCHED"

def file_hash(path):
    with open(path, "rb") as f:
        return hashlib.md5(f.read()).hexdigest()

def patch_file(rel_path, replacements):
    """Apply a list of (old, new) string replacements to a framework file.
    Uses a marker comment to avoid double-patching."""
    abs_path = os.path.join(FRAMEWORK_DIR, rel_path)
    if not os.path.exists(abs_path):
        print(f"patch_framework: file not found: {abs_path}")
        return False

    with open(abs_path, "r", encoding="utf-8") as f:
        content = f.read()

    if MARKER in content:
        print(f"patch_framework: {rel_path} already patched")
        return True

    for old, new in replacements:
        if old not in content:
            print(f"patch_framework: WARNING: pattern not found in {rel_path}:")
            print(f"  {old[:80]}...")
            return False
        content = content.replace(old, new, 1)

    content = MARKER + "\n" + content

    with open(abs_path, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"patch_framework: patched {rel_path}")
    return True

def main():
    # Fix: %ld format specifier for uint32_t baudrate -> %u
    # Fix: null pointer dereference (move log_e before uart = NULL)
    # References:
    #   https://github.com/espressif/arduino-esp32/issues/9634
    #   Fixed upstream in Arduino ESP32 core >= 3.0.0-RC3
    #   Check if this patch can be removed when platform bundles that version.
    patch_file("cores/esp32/esp32-hal-uart.c", [
        (
            'log_v("UART%d baud(%ld) Mode(%x) rxPin(%d) txPin(%d)", uart_nr, baudrate, config, rxPin, txPin);',
            'log_v("UART%d baud(%u) Mode(%x) rxPin(%d) txPin(%d)", uart_nr, baudrate, config, rxPin, txPin);',
        ),
        (
            '        uartEnd(uart_nr);\n        uart = NULL;\n        log_e("UART%d initialization error.", uart->num);',
            '        uartEnd(uart_nr);\n        log_e("UART%d initialization error.", uart->num);\n        uart = NULL;',
        ),
    ])

main()
