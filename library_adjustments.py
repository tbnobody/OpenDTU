import os
import re

Import("env")

def getLibraryPath(env, libraryPath = None):
    """Function for retrieving the requested library path for the choosen board in a plain stupid way."""
    if not libraryPath:
        raise ValueError('`libraryPath` should not be empty!')

    return os.path.join(env["PROJECT_DIR"], ".pio", "libdeps", env["BOARD"], libraryPath)

def replaceInFile(filePath, text, subs, flags=0):
    """
        Function for replacing content for the given file
        Taken from https://www.studytonight.com/python-howtos/search-and-replace-a-text-in-a-file-in-python
    """
    if os.path.exists(filePath):
        with open(filePath, "r+") as file:
            #read the file contents
            file_contents = file.read()
            text_pattern = re.compile(re.escape(text), flags)
            file_contents = text_pattern.sub(subs, file_contents)
            file.seek(0)
            file.truncate()
            file.write(file_contents)

def adjustEspAsyncWebServer(env):
    """Reusable function for replacements within `ESP Async WebServer` library"""
    libPath = getLibraryPath(env, "ESP Async WebServer")
    if os.path.exists(libPath):
        replaceInFile(os.path.join(libPath, "src", "AsyncWebSocket.cpp"), "IPAddress(0U)", "IPAddress((uint32_t)0)")

def adjustSeeedXiaoEsp32c3(env):
    """Function for replacements required for `seeed_xiao_esp32c3` board"""
    adjustEspAsyncWebServer(env)

def adjustNothing(env):
    """Dummy function for doing absolutely nothing :-)"""
    print ("Nothing to adjust!")

# Replacement lookup table
# key: boardname as stated in platform.io
# value: replacement function to be used
boards = {
    'seeed_xiao_esp32c3': adjustSeeedXiaoEsp32c3
}

# Lookup and replace, no entry -> dummy call
boards.get(env["BOARD"], adjustNothing)(env)
