# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2022 Thomas Basler and others
#
import pkg_resources

Import("env")

required_pkgs = {'dulwich'}
installed_pkgs = {pkg.key for pkg in pkg_resources.working_set}
missing_pkgs = required_pkgs - installed_pkgs

if missing_pkgs:
    env.Execute('"$PYTHONEXE" -m pip install dulwich')

from dulwich import porcelain

def get_firmware_specifier_build_flag():
    try:
        build_version = porcelain.describe('.')  # '.' refers to the repository root dir
    except Exception as err:
        print(f"Unexpected {err=}, {type(err)=}")
        build_version = "g0000000"
    try:
        branch_name = porcelain.active_branch('.').decode('utf-8')  # '.' refers to the repository root dir
    except Exception as err:
        print(f"Unexpected {err=}, {type(err)=}")
        branch_name = "master"
    build_flag = "-D AUTO_GIT_HASH=\\\"" + build_version + "\\\" "
    build_flag += "-D AUTO_GIT_BRANCH=\\\"" + branch_name + "\\\""
    print("Firmware Revision: " + build_version)
    print("Firmware build on branch: " + branch_name)
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_firmware_specifier_build_flag()]
)