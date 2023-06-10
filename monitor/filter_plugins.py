#!/usr/bin/env python3

from platformio.commands.device import DeviceMonitorFilter

#
# no clue what i'm doing :)
#
class Pluginfilter(DeviceMonitorFilter):
    NAME = "pluginfilter"
    pluginnames = []
    buffer = ""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        pluginstring = self.config.get("env:" + self.environment, "custom_plugin_filter_names") #env.GetProjectOptions("custom_plugin_filter_names")
        self.pluginnames = pluginstring.split(',')
        print("Pluginfilter is loaded: "+pluginstring)

    def line(self, text):
        for plg in self.pluginnames:
            index = text.startswith(plg)
            if index != False:
                return text
        return None

    def rx(self, text):
        last = 0
        l = ""
        while True:
            idx = text.find("\n", last)
            if idx == -1:
                if len(self.buffer) < 4096:
                    self.buffer += text[last:]
                break

            line = text[last:idx]
            if self.buffer:
                line = self.buffer + line
                self.buffer = ""
            last = idx + 1

            if line and line[-1] == "\r":
                line = line[:-1]

            extra = self.line(line)
            if extra is not None:
                l += extra+"\n"
                last += len(extra)
        return l
        
    def tx(self, text):
        print(f"Sent: {text}\n")
        return text

