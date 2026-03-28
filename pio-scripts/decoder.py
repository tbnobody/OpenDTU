#!/usr/bin/env python

import argparse
import re
import os
import subprocess
import sys


class ESP32CrashParser(object):
    def __init__(self, toolchain_path, elf_path):
        self.toolchain_path = toolchain_path
        self.gdb_path = os.path.join(toolchain_path, "bin", "xtensa-esp32-elf-gdb.exe")
        self.addr2line_path = os.path.join(toolchain_path, "bin", "xtensa-esp32-elf-addr2line.exe")

        if not os.path.exists(self.gdb_path):
            raise Exception("GDB for ESP not found in {} - {} does not exist.\nUse --toolchain to point to "
                            "your toolchain folder.".format(self.toolchain_path, self.gdb_path))

        if not os.path.exists(self.addr2line_path):
            raise Exception("addr2line for ESP not found in {} - {} does not exist.\nUse --toolchain to point to "
                            "your toolchain folder.".format(self.toolchain_path, self.addr2line_path))

        self.elf_path = elf_path
        if not os.path.exists(self.elf_path):
            raise Exception("ELF file not found: '{}'".format(self.elf_path))

    def parse_text(self, text):
        m = re.search('Backtrace: (.*)', text)
        if m:
            print ("Stack trace:")
            for l in self.parse_stack(m.group(1)):
                print ("  {}".format(l).encode())
        else:
            print ("No stack trace found.")

    '''
    Decode one stack or backtrace.

    See: https://github.com/me-no-dev/EspExceptionDecoder/blob/master/src/EspExceptionDecoder.java#L402
    '''
    def parse_stack(self, text):
        r = re.compile('40[0-2][0-9a-fA-F]{5}')
        m = r.findall(text)
        return self.decode_function_addresses(m)

    def decode_function_address(self, address):
        args = [self.addr2line_path, "-e", self.elf_path, "-aipfC", address]
        return subprocess.check_output(args).strip()

    def decode_function_addresses(self, addresses):
        out = []
        for a in addresses:
            out.append(self.decode_function_address(a))
        return out

    '''
    GDB Should produce line number: https://github.com/me-no-dev/EspExceptionDecoder/commit/a78672da204151cc93979a96ed9f89139a73893f
    However it does not produce anything for me. So not using it for now.
    '''
    def decode_function_addresses_with_gdb(self, addresses):
        args = [self.gdb_path, "--batch"]

        # Disable user config file which might interfere here
        args.extend(["-iex", "set auto-load local-gdbinit off"])

        args.append(self.elf_path)

        args.extend(["-ex", "set listsize 1"])
        for address in addresses:
            args.append("-ex")
            args.append("l *0x{}".format(address))
        args.extend(["-ex", "q"])

        print ("Running: {}".format(args))
        out = subprocess.check_output(args)
        print (out)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--toolchain", help="Path to the Xtensa toolchain",
                        default=os.path.join("c:\\users\\thomas\\.platformio\\packages\\toolchain-xtensa32"))
    parser.add_argument("--elf", help="Path to the ELF file of the firmware",
                        default=".pio/build/lolin_s2_mini/firmware.elf")
    parser.add_argument("input")

    args = parser.parse_args()

    crash_parser = ESP32CrashParser(args.toolchain, args.elf)
    crash_parser.parse_text(args.input)


if __name__ == '__main__':
    main()
