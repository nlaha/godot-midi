#!/usr/bin/env python
import os
import platform
import shutil

def normalize_path(val, env):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)

def validate_parent_dir(key, val, env):
    if not os.path.isdir(normalize_path(os.path.dirname(val), env)):
        raise UserError("'%s' is not a directory: %s" % (key, os.path.dirname(val)))

libname = "godotmidi"
projectdir = "game"

localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add(
    BoolVariable(
        key="compiledb",
        help="Generate compilation DB (`compile_commands.json`) for external tools",
        default=localEnv.get("compiledb", False),
    )
)
opts.Add(
    PathVariable(
        key="compiledb_file",
        help="Path to a custom `compile_commands.json` file",
        default=localEnv.get("compiledb_file", "compile_commands.json"),
        validator=validate_parent_dir,
    )
)
opts.Update(localEnv)

linux_cross_compilers = {
    "arm64": ("aarch64", ("arm64", "aarch64")),
    "rv64": ("riscv64", ("rv64", "riscv64")),
}
# Set compilers on the parent environment before godot-cpp applies its platform flags.
target_arch = ARGUMENTS.get("arch", "")
target_arch_aliases = {
    "aarch64": "arm64",
    "armv8": "arm64",
    "rv": "rv64",
    "riscv": "rv64",
    "riscv64": "rv64",
}
target_arch = target_arch_aliases.get(target_arch, target_arch)
cross_compiler = linux_cross_compilers.get(target_arch)
host_arch = platform.machine().lower()
target_platform = ARGUMENTS.get("platform", platform.system().lower())
if target_platform == "linux" and cross_compiler and host_arch not in cross_compiler[1]:
    compiler_prefix = cross_compiler[0] + "-linux-gnu"
    if not shutil.which(compiler_prefix + "-g++"):
        raise UserError(
            "Linux {} cross-compilation requires {}-g++ in PATH".format(target_arch, compiler_prefix)
        )
    localEnv.Replace(
        CC=compiler_prefix + "-gcc",
        CXX=compiler_prefix + "-g++",
        SHCC=compiler_prefix + "-gcc",
        SHCXX=compiler_prefix + "-g++",
        LINK=compiler_prefix + "-g++",
        SHLINK=compiler_prefix + "-g++",
        AR=compiler_prefix + "-ar",
        RANLIB=compiler_prefix + "-ranlib",
    )

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()
env["compiledb"] = False

env.Tool("compilation_db")
compilation_db = env.CompilationDatabase(
    normalize_path(localEnv["compiledb_file"], localEnv)
)
env.Alias("compiledb", compilation_db)

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

env.Append(CPPPATH=["extension/src/", ".cmake/doctest/src/doctest/doctest/"])
sources = Glob("extension/src/*.cpp")

file = "{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"])

if env["platform"] == "macos":
    platlibname = "{}.{}.{}".format(libname, env["platform"], env["target"])
    file = "{}.framework/{}".format(env["platform"], platlibname, platlibname)

libraryfile = "bin/{}/{}".format(env["platform"], file)
print("CCFLAGS: ", env['CCFLAGS'])

library = env.SharedLibrary(
    libraryfile,
    source=sources,
)

copy = env.InstallAs("{}/addons/godot_midi/bin/{}/lib{}".format(projectdir, env["platform"], file), library)

default_args = [library, copy]
if localEnv.get("compiledb", False):
    default_args += [compilation_db]
Default(*default_args)
