"""Run with Python 3 and a C++17 compiler (MSVC requires a developer shell)."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "lib/Hoymiles/src/HoymilesRadio_CMT.cpp").read_text()
start = source.index("        while (_radio->available()) {")
end = source.index("        _packetReceived = false;", start)
receive = source[start:end]

# Exercise the production receive block against a latched interrupt and packet bursts.
harness = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <queue>
#include <stdexcept>
#include "types.h"
#define FRAGMENT_BUFFER_SIZE 30
#define ESP_LOGE(...) ((void)0)
struct Radio {
    int pending = 0, reads = 0, checks = 0, restarts = 0;
    bool latched = false;
    bool available() {
        if (++checks > 100) throw std::runtime_error("receive loop does not terminate");
        return pending > 0 || latched;
    }
    void flush_rx() { pending = 0; } // FIFO clear does not acknowledge interrupt flags.
    bool startListening() { ++restarts; pending = 0; latched = false; return true; }
    uint8_t getDynamicPayloadSize() { return 12; }
    uint8_t getChannel() { return 20; }
    int8_t getRssiDBm() { return -42; }
    void read(void*, uint8_t) { ++reads; --pending; }
};
void receive(Radio* _radio, std::queue<fragment_t>& _rxBuffer) {
RECEIVE_BLOCK
}
int main() {
    for (int initial : {0, 29, 30, 31}) {
        std::queue<fragment_t> buffer;
        for (int i = 0; i < initial; ++i) buffer.push(fragment_t{});
        Radio radio;
        radio.pending = 40;
        radio.latched = true;
        receive(&radio, buffer);
        assert(buffer.size() == static_cast<size_t>(std::max(initial, 30)));
        assert(radio.reads == std::max(0, 30 - initial));
        assert(radio.restarts == 1);
        assert(!radio.latched);
        while (!buffer.empty()) buffer.pop();
        radio.pending = 2;
        receive(&radio, buffer);
        assert(buffer.size() == 2); // Reception resumes after saturation.
        assert(radio.restarts == 1);
    }
    std::queue<fragment_t> buffer;
    Radio radio;
    radio.pending = 3;
    receive(&radio, buffer);
    assert(buffer.size() == 3 && radio.reads == 3 && radio.restarts == 0);
}
'''.replace("RECEIVE_BLOCK", receive)

with tempfile.TemporaryDirectory(prefix="opendtu-cmt-test-") as folder:
    folder = Path(folder)
    test = folder / "receive.cpp"
    test.write_text(harness)
    executable = folder / "receive.exe"
    compiler = shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    if compiler:
        command = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                   "-I" + str(root / "lib/Hoymiles/src"), str(test), "-o", str(executable)]
    else:
        command = ["cl", "/nologo", "/std:c++17", "/EHsc", "/W4", "/WX",
                   "/I" + str(root / "lib/Hoymiles/src"), str(test),
                   "/Fe:" + str(executable), "/Fo:" + str(folder) + "\\"]
    subprocess.run(command, check=True, cwd=folder)
    subprocess.run([str(executable)], check=True, timeout=5)
print("CMT reception: normal packets, saturation, latched flags and recovery passed.")
