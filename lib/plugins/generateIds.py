import re
import os
import sys

def writeFile(outf,pluginlist,taglist):
    outfile = open(outf, "w")
    #outfile.write("#ifndef __PLUGINIDS_H__\n")
    #outfile.write("#define __PLUGINIDS_H__\n\n")
    outfile.write("#pragma once\n\n");
    outfile.write("#ifndef NDEBUG\n")
    generateIds(outfile, pluginlist)
    #generateValueIds(outfile, pluginlist)
    generateValueIdsString(outfile,pluginlist)
    outfile.flush();
    #generateTagIds(outfile, taglist)
    #generateTagIdsString(outfile, taglist)
    outfile.write("#endif // NDEBUG\n")
    generateDebugFunction(outfile, pluginlist, taglist)
    #outfile.write("\n#endif /* __PLUGINIDS_H__ */\n")
    outfile.close
    
def generateIds(f,plist):
    f.write("enum PluginIds {\n")
    for p in plist:
        f.write("    Plugin"+p['pname'].capitalize()+" = "+p['pid']+",\n")
    f.write("};\n")
    
def generateValueIds(f,plist):
    for p in plist:
        f.write("enum Plugin"+p['pname'].capitalize()+"Ids {\n")
        for v in p['enumvalue']:
            f.write("    "+v.replace(" ","")+",\n")
        f.write("};\n")

def generateValueIdsString(f,plist):
    for p in plist:
        f.write("const char Plugin"+p['pname'].capitalize()+"IdString[] = \""+p['pname'].capitalize()+"\";\n")
        # for v in p['enumvalue']:
        #     enumval = v.replace(" ","")
        #     f.write("const char Plugin"+p['pname'].capitalize()+enumval+"IdString[] = \""+enumval+"\";\n")
    f.write("const char PluginUnknown[] = \"unknown\";\n")

def generateTagIds(f, taglist):
    for p in taglist:
        p = p.replace(" ","")
        f.write("const char TAG_"+p+"_String[] = \""+p.capitalize()+"\";\n")
    f.write("\n")
    
def generateTagIdsString(f, taglist):
    f.write("static const char* getTagNameDebug(int id) {\n\tswitch(id){\n")
    i = 0
    for p in taglist:
        p = p.replace(" ","")
        f.write("\t\tcase "+str(i)+": return TAG_"+p+"_String;\n")
        i+=1
    f.write("\t\tdefault: return PluginUnknown;\n")
    f.write("}\n}\n")
    
def generateDebugFunction(f,plist,tlist):
    f.write("#ifdef NDEBUG\n")
    f.write("#define DBGPRINTMESSAGETAGSLN(level,message)\n")
    f.write("#define DBGPRINTMESSAGELN(level,message)\n")
    f.write("#define DBGPRINTMESSAGELNCB(level,prefix,message)\n")
    f.write("#define DBGPRINTMESSAGEFROMTO(level,prefix,message)\n")
    f.write("#else\n")
    f.write("class PluginDebug {\npublic:\n")
    #generateTagIdsString(f, tlist)
    f.write("static const char* getPluginNameDebug(int pid) {\n\tswitch(pid){\n")
    for p in plist:
        f.write("\t\tcase "+p['pid']+": return Plugin"+p['pname'].capitalize()+"IdString;\n")
    f.write("\t\tdefault: return PluginUnknown;\n")
    f.write("}\n}\n")
    # f.write("static const char* getPluginValueNameDebug(int pid, int vid) {\n\tswitch(pid){\n")
    # for p in plist:
    #     f.write("\t\tcase "+p['pid']+": switch(vid){\t\t\t\n")
    #     i = 0
    #     for v in p['enumvalue']:
    #         f.write("\t\t\t\tcase "+str(i)+": return Plugin"+p['pname'].capitalize()+v.replace(" ","")+"IdString;\n")
    #         i+=1
    #     f.write("\t\t\t\tdefault: return PluginUnknown;\n")
    #     f.write("}\n")
    # f.write("\t\t\tdefault: return PluginUnknown;\n")
    # f.write("}\n}\n")
    f.write("}; /* PluginDebug class end */\n")
    # f.write("#define DBGPRINTMESSAGELN(level,prefix,message) ({\\\n")
    # f.write("       char msgbuffer[256];\\\n")
    # f.write("       message->toString(msgbuffer);\\\n")
    # f.write("       MessageOutput.println(msgbuffer);\\\n")
    # f.write("  })\n")
    # f.write("#define DBGPRINTMESSAGELNCB(level,prefix,message) ({\\\n")
    # f.write("       char msgbuffer[256];\\\n")
    # f.write("       message->toString(msgbuffer);\\\n")
    # f.write("       MessageOutput.printf(\"%s: %s\\n\",prefix,msgbuffer);\\\n")
    # f.write("  })\n")
    # f.write("#define DBGPRINTMESSAGEFROMTO(level,prefix,message) ({\\\n")
    # f.write("       char msgbuffer[256];\\\n")
    # f.write("       message->toString(msgbuffer);\\\n")
    # f.write("       MessageOutput.printf(\"%s: From %s to %s -> %s\\n\",prefix,PluginDebug::getPluginNameDebug(message->getSenderId()),PluginDebug::getPluginNameDebug(message->getReceiverId()),msgbuffer);\\\n")
    # f.write("  })\n")
    # f.write("#define DBGPRINTMESSAGEDURATION(level,prefix,message) ({\\\n")
    # f.write("       unsigned long duration = millis();\\\n")
    # f.write("       duration = duration - message->getTS();\\\n")
    # f.write("       MessageOutput.printf(\"%s: message processed in %lu [ms]\\n\",prefix,duration);\\\n")
    # f.write("  })\n")
    f.write("#endif\n")
        
def parseFile(fin):
    f = open(fin, "r")
    data = f.read().replace('\n', '')
    #print(data)
    pid = None;
    name = None;
    enumvals = None;
    x = re.search(r'(enum)\s+(\w+)\s+\{\s*([^\}]+)\s*\};(.*Plugin\s*\((\d*)\s*,\s*"([^"]*)")', data)
    if x:
        pid = x.group(5)
        name = x.group(6)
        enumvals = x.group(3)
    else:
        x = re.search(r'(.*Plugin\s*\((\d*)\s*,\s*"([^"]*)")', data)
        if x:
            pid = x.group(2)
            name = x.group(3)
    return pid,name,enumvals

def parseFileEnum(fin):
    f = open(fin, "r")
    data = f.read().replace('\n', '')
    #print(data)
    x = re.search(r'(enum class METADATA_TAGS)\s+\{\s*([^\}]+)\s*\}(.*)', data)
    if x:
        return x.group(2)
    else:
        return None

def getRecFilesDir(dir):
    subfolders = [ dir+'/'+f.name for f in os.scandir(dir) if f.is_dir() ]
    return subfolders

def getFilesDir(dir):
    files = os.listdir(dir)
    files = [dir+'/'+f for f in files if os.path.isfile(dir+'/'+f) & f.endswith('h')] 
    return files;

def getAllFiles(dir):
    files = getFilesDir(dir)
    subdirs = getRecFilesDir(dir)
    for d in subdirs:
        files.extend(getFilesDir(d))
    return files

try:
    scriptdir = os.path.dirname(os.path.abspath(__file__))
except NameError:
    approot = os.path.dirname(os.path.abspath(sys.argv[1]))
    scriptdir = os.path.join(approot, 'lib/plugins/')

genfilename = 'pluginids.h'  
gendir = scriptdir + 'src/base/'             
sourcedir = scriptdir + 'src/'    
genfile = gendir + genfilename         
print('scan for sourcefiles in '+sourcedir)
print('generating file: '+genfile)

#files = os.listdir(scriptdir)
#files = [scriptdir+'/'+f for f in files if os.path.isfile(scriptdir+'/'+f) & f.endswith('h')] 
files = getAllFiles(sourcedir)
print(*files, sep="\n")
pluginlist = list()
taglist = None
for f in files:
    pid, pname,enumvalue = parseFile(f)
    tags = parseFileEnum(f)
    if pid != None:
        print(pid+":"+pname+":"+str(enumvalue))
        if enumvalue != None:
            vids = enumvalue.split(",")
        else:
            vids = []
        pluginlist.append({'pid':pid,'pname':pname,'enumvalue':list(vids)})
    if tags != None:
        print(tags)
        taglist = tags.split(",")
writeFile(genfile,pluginlist,taglist)
