# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2022 Thomas Basler and others
#
import os

Import("env")

try:
    from dulwich import porcelain
except ModuleNotFoundError:
    env.Execute('"$PYTHONEXE" -m pip install dulwich')
    from dulwich import porcelain


def updateFileIfChanged(filename, content):
    mustUpdate = True
    try:
        with open(filename, "rb") as fp:
            if fp.read() == content:
                mustUpdate = False
    except:
        pass
    if mustUpdate:
        with open(filename, "wb") as fp:
            fp.write(content)
    return mustUpdate


def get_build_version():
    try:
        build_version = porcelain.describe('.')  # '.' refers to the repository root dir
    except:
        build_version = "g0000000"
    print ("Firmware Revision: " + build_version)
    return build_version


def get_build_branch():
    try:
        branch_name = porcelain.active_branch('.').decode('utf-8')  # '.' refers to the repository root dir
    except Exception as err:
        branch_name = "master"
    print("Firmware Branch: " + branch_name)
    return branch_name


def get_firmware_specifier_build_flag():
    build_version = get_build_version()
    build_flag = "-D AUTO_GIT_HASH=\\\"" + build_version + "\\\""
    build_branch = get_build_branch()
    build_flag += " -D AUTO_GIT_BRANCH=\\\"" + build_branch + "\\\""
    return (build_flag)


def do_main():
    if 0:
        # this results in a full recompilation of the whole project after each commit
        env.Append(
            BUILD_FLAGS=[get_firmware_specifier_build_flag()]
        )
    else:
        # we just create a .c file containing the needed datas
        targetfile = os.path.join(env.subst("$BUILD_DIR"), "__compiled_constants.c")
        lines = ""
        lines += "/* Generated file within build process - Do NOT edit */\n"

        if 0:
            # Add the current date and time as string in UTC timezone
            from datetime import datetime, timezone
            now = datetime.now(tz=timezone.utc)
            COMPILED_DATE_TIME_UTC_STR = now.strftime("%Y/%m/%d %H:%M:%S")
            lines += 'const char *__COMPILED_DATE_TIME_UTC_STR__ = "%s";\n' % (COMPILED_DATE_TIME_UTC_STR)

        if 1:
            # Add the description of the current git revision
            lines += 'const char *__COMPILED_GIT_HASH__ = "%s";\n' % (get_build_version())
            # ... and git branch
            lines += 'const char *__COMPILED_GIT_BRANCH__ = "%s";\n' % (get_build_branch())

        updateFileIfChanged(targetfile, bytes(lines, "utf-8"))

        # Add the created file to the buildfiles - platformio knows how to handle *.c files
        env.AppendUnique(PIOBUILDFILES=[targetfile])

do_main()
