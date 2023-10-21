#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["extension/src/"])
sources = Glob("extension/src/*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "game/addons/godot_midi/bin/libgdgodotmidi.{}.{}.framework/libgdgodotmidi.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "game/addons/godot_midi/bin/libgdgodotmidi{}{}".format(
            env["suffix"], env["SHLIBSUFFIX"]
        ),
        source=sources,
    )

env.Append(CPPPATH=["thirdparty/"])

sources += Glob("tests/*.cpp")
env.Append(CXXFLAGS=["/DEBUG"])

tests = []
# build executable for each cpp file
# don't use glob as it returns a list of files not paths
for source in os.listdir("tests/"):
    if source.endswith(".cpp"):
        name = source[:-4]
        # set output directory to ./bin
        print("Building test: " + source)
        test = env.Program("tests/bin/" + name, sources)
        tests.append(test)

Default(library, tests)
