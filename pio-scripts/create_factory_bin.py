# Part of ESPEasy build toolchain.
#
# Combines separate bin files with their respective offsets into a single file
# This single file must then be flashed to an ESP32 node with 0 offset.
#
# Original implementation: Bartłomiej Zimoń (@uzi18)
# Maintainer: Gijs Noorlander (@TD-er)
#
# Special thanks to @Jason2866 (Tasmota) for helping debug flashing to >4MB flash
# Thanks @jesserockz (esphome) for adapting to use esptool.py with merge_bin
#
# Typical layout of the generated file:
#    Offset | File
# -  0x1000 | ~\.platformio\packages\framework-arduinoespressif32\tools\sdk\esp32\bin\bootloader_dout_40m.bin
# -  0x8000 | ~\ESPEasy\.pio\build\<env name>\partitions.bin
# -  0xe000 | ~\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin
# - 0x10000 | ~\ESPEasy\.pio\build\<env name>/<built binary>.bin

Import("env")

env = DefaultEnvironment()
platform = env.PioPlatform()

import sys
import csv
import subprocess
import shutil
from os.path import join, getsize, exists, isdir
from os import listdir

sys.path.append(join(platform.get_package_dir("tool-esptoolpy")))
import esptool

def esp32_build_filesystem(fs_name, fs_size):
    filesystem_dir = env.subst("$PROJECT_DATA_DIR")
    print("Creating %dKiB filesystem with content:" % (int(fs_size, 0)/1024) )
    if not isdir(filesystem_dir) or not listdir(filesystem_dir):
        print("No files added -> will NOT create littlefs.bin and NOT overwrite fs partition!")
        return False
    # this does not work on GitHub, results in 'mklittlefs: No such file or directory'
    tool =  shutil.which(env.subst(env["MKFSTOOL"]))
    if tool is None or not exists(tool):
        print("Using fallback mklittlefs")
        tool = "~/.platformio/packages/tool-mklittlefs/mklittlefs"

    cmd = (tool, "-c", filesystem_dir, "-s", fs_size, fs_name)
    returncode = subprocess.call(cmd, shell=False)
    print("Return Code:", returncode)
    return True

def esp32_create_combined_bin(source, target, env):
    print("Generating combined binary for serial flashing")

    # The offset from begin of the file where the app0 partition starts
    # This is defined in the partition .csv file
    app_offset = 0x10000
    fs_offset = -1
    fs_name = env.subst("$BUILD_DIR/littlefs.bin")

    with open(env.BoardConfig().get("build.partitions")) as csv_file:
        print("Read partitions from ", env.BoardConfig().get("build.partitions"))
        csv_reader = csv.reader(csv_file, delimiter=',')
        line_count = 0
        for row in csv_reader:
            if line_count == 0:
                print(f'{",  ".join(row)}')
                line_count += 1
            else:
                if (len(row) < 4):
                    continue
                print(f'{row[0]}   {row[1]}   {row[2]}   {row[3]}   {row[4]}')
                line_count += 1
                if(row[0] == 'app0'):
                    app_offset = int(row[3], base=16)
                elif(row[0] == 'spiffs'):
                    partition_size = row[4]
                    if esp32_build_filesystem(fs_name, partition_size):
                        fs_offset = int(row[3], base=16)

    new_file_name = env.subst("$BUILD_DIR/${PROGNAME}.factory.bin")
    sections = env.subst(env.get("FLASH_EXTRA_IMAGES"))
    firmware_name = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    chip = env.get("BOARD_MCU")
    flash_size = env.BoardConfig().get("upload.flash_size")
    flash_freq = env.BoardConfig().get("build.f_flash", '40m')
    flash_freq = flash_freq.replace('000000L', 'm')
    flash_mode = env.BoardConfig().get("build.flash_mode", "dio")
    memory_type = env.BoardConfig().get("build.arduino.memory_type", "qio_qspi")
    if flash_mode == "qio" or flash_mode == "qout":
        flash_mode = "dio"
    if memory_type == "opi_opi" or memory_type == "opi_qspi":
        flash_mode = "dout"
    cmd = [
        "--chip",
        chip,
        "merge_bin",
        "-o",
        new_file_name,
        "--flash_mode",
        flash_mode,
        "--flash_freq",
        flash_freq,
        "--flash_size",
        flash_size,
    ]

    # platformio estimates the amount of flash used to store the firmware. this
    # estimate is not accurate. we perform a final check on the firmware bin
    # size by comparing it against the respective partition size.
    max_size = env.BoardConfig().get("upload.maximum_size", 1)
    fw_size = getsize(firmware_name)
    if (fw_size > max_size):
        raise Exception("firmware binary too large: %d > %d" % (fw_size, max_size))

    print("    Offset | File")
    for section in sections:
        sect_adr, sect_file = section.split(" ", 1)
        print(f" -  {sect_adr} | {sect_file}")
        cmd += [sect_adr, sect_file]

    print(f" - {hex(app_offset)} | {firmware_name}")
    cmd += [hex(app_offset), firmware_name]

    if fs_offset != -1:
        print(f" - {hex(fs_offset)} | {fs_name}")
        cmd += [hex(fs_offset), fs_name]

    print('Using esptool.py arguments: %s' % ' '.join(cmd))

    esptool.main(cmd)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", esp32_create_combined_bin)
