--[[
    NativeBukkit's build script. It assumes a POSIX-compliant environment with bash.
    
    if you are compiling for another platform, create a .lua script that sets these
    variables accordingly and then call 'dofile("build.lua")'. You can also add
    another rule in the makefile.
]]

-- buffering is annoying, turn it off
io.output():setvbuf("no")

--[[ Settings ]]

function default(gstr, val)
    if _G[gstr] == nil then _G[gstr] = val
    else assert(type(_G[gstr]) == type(val)) end
end
-- assume openjdk8 if JAVA_HOME is not set
JAVA_HOME = os.getenv("JAVA_HOME") or "/usr/lib/jvm/java-8-openjdk"

-- include folders
default("INCLUDES", {"api", JAVA_HOME .. "/include", JAVA_HOME .. "/include/linux"})
-- library names to pass to the compiler
default("LIBRARIES", {})
-- full library names (not paths) such as 'libfoo.so.0' for prequisite checks
default("LIB_DEPENDENCIES", {})

default("COMPILER", "gcc")
-- if you are compiling for your own system, use -march=native
default("COMPILER_CFLAGS", "-m64")
default("COMPILER_ARGS", "-Wall -Werror -fPIC -O2 " .. COMPILER_CFLAGS)
default("COMPILER_DEBUG_ARGS", "-Wall -Wextra -fPIC -O0 -ggdb3 -DJU_DEBUG " .. COMPILER_CFLAGS)
default("COMPILER_OUTFLAG", "-o")
default("COMPILER_INCLUDEFLAG", "-I")
default("COMPILER_SKIP_LINK", "-c")
default("COMPILER_LANG_FLAG", "-x")

default("LINKER", "gcc")
default("LINKER_ARGS", "-shared -fvisibility=hidden")
default("LINKER_OUTFLAG", "-o")
default("LINKER_LIBRARYFLAG", "-l")

-- symbols to preserve when stripping the final executable
default("PRESERVE_SYMBOLS", {
            "Java_jni_JNIEntry_entry",
            "Java_jni_JNIPlugin_onLoad",
            "Java_jni_JNIPlugin_onEnable",
            "Java_jni_JNIPlugin_onDisable",
            "Java_jni_JNIPlugin_close",
            "Java_jni_JNIPlugin_open",
            "Java_jni_JNIRunnable_run"
})
-- whether to strip all symbols, or only strip uneeded symbols
default("AGGRESSIVE_STRIP", true)

default("SOURCES", "src") -- sources, recursively searched
default("OUTPUTS", "out") -- objects
default("HEADERS", "gen") -- generated headers

-- final executable or library name
default("FINAL", "native-final/nativebukkit.so")

-- whether to strip source files for iheader syntax, and to generate headers
default("IHEADERS", true)

-- if the current system complies with the Filesystem Hierarchy Standard (most Linux distributions).
-- BSD users can leave this as true, since everything this script uses that relies on FHS is also
-- true for BSD systems.

-- Disabling this will remove some OS-dependent checks before building.
default("FHS_COMPLIANT_SYSTEM", true)

-- if the compiler in use is GCC. Disabling this will remove prerequisite checks for libraries.
default("GCC_BASED_COMPILER", true)

-- for build.c
NATIVE_LIB = "build.c"
LUA_CFLAGS = "-Wall -fPIC"
LUA_LFLAGS = "-shared"
LUA_LINK = "lua"

-- like table.concat, except it ignores nil and empty string values
function concat_s(tbl, pre, idx)
    local str = ""
    for i = 1, #tbl do
        local v = idx ~= nil and tbl[i][idx] or tbl[i]
        if (v ~= nil and v ~= "") then
            str = pre ~= nil and str .. pre .. v or str .. v
            if (i ~= #tbl) then str = str .. " " end
        end
    end
    return str
end

-- Ugly hack for lua 5.x's os.execute function to return the right value.
-- LuaJIT will return the correct value, though.
if string.sub(_VERSION, 1, 4) == "Lua " then
    __old_exec = os.execute
    os.execute = function(str)
        local ret = __old_exec(str)
        if type(ret) == "number" then
            return math.floor(ret / 256)
        elseif type(ret) == "boolean" then
            -- because for some undocumented reason, this might actually return a boolean.
            return ret and 0 or 1
        end
    end
end

-- goals
goals = {
    prep = function()
        prereq {"iheaders", "strip"}
        prereq_libs(LIB_DEPENDENCIES)
        SOURCES = trim_node(SOURCES)
        OUTPUTS = trim_node(OUTPUTS)
        INCLUDES[#INCLUDES + 1] = "gen"
        os.execute("mkdir -p " .. SOURCES .. " && mkdir -p "
                       .. OUTPUTS .. " && mkdir -p " .. HEADERS .. " && mkdir -p native-final")
    end,
    mvn = function()
        goals.load_native()
        goals.prep()
        if (C.isfile(FINAL) ~= true) then
            error("the 'mvn' goal cannot be ran before 'all'")
        end
        writeb("running maven goal: ", TERM_GREEN)
        print("install")
        local mvn_cmd = "mvn install"
        printcmd(mvn_cmd)
        if (os.execute(mvn_cmd) ~= 0) then
            error("maven build failed")
        end
    end,
    debug = function()
        goals.all(true)
    end,
    all = function(debug_mode)
        goals.load_native()
        goals.prep()
        if IHEADERS then
            writeb("generating headers: ", TERM_GREEN)
            print(SOURCES .. " -> " .. HEADERS)
            local cmd = "iheaders -G -d " .. HEADERS .. " -r " .. SOURCES
                .. " " .. concat_s(sort_files_t(SOURCES, "c", "cpp"), nil, "full")
            printcmd("iheaders -G -d " .. HEADERS .. " -r " .. SOURCES .. " {sources}")
            if (os.execute(cmd) ~= 0) then
                error("failed to generate headers")
            end
        end
        for k, entry in sort_files(SOURCES, "c", "cpp", "S") do
            if (string.sub(entry.filen, 1, 1) ~= ".") then
                writeb("compiling: ", TERM_GREEN)
                print(entry.full)
                local ext = entry.file:sub(entry.file:find(".", 1, true), entry.file:len())
                local genstr = ""
                if #INCLUDES > 0 then
                    for i = 1, #INCLUDES do
                        genstr = genstr .. COMPILER_INCLUDEFLAG .. " \"" .. INCLUDES[i] .. "\""
                        if (#INCLUDES ~= i) then genstr = genstr .. " " end
                    end
                end
                local parts = nil
                if debug_mode == true then COMPILER_ARGS = COMPILER_DEBUG_ARGS end
                if ext == ".c" and IHEADERS then
                    parts = {
                        "iheaders", "-p", "-O", entry.full, "|",
                        COMPILER, COMPILER_ARGS, genstr,
                        COMPILER_INCLUDEFLAG, "\"" .. SOURCES .. "\"", COMPILER_OUTFLAG,
                        OUTPUTS .. entry.path .. "/" .. entry.filen .. ".o",
                        COMPILER_SKIP_LINK, COMPILER_LANG_FLAG, "c", "-"
                    }
                else
                    parts = {
                        COMPILER, COMPILER_ARGS, genstr,
                        COMPILER_INCLUDEFLAG, "\"" .. SOURCES .. "\"",
                        entry.full, COMPILER_OUTFLAG,
                        OUTPUTS .. entry.path .. "/" .. entry.filen .. ".o",
                        COMPILER_SKIP_LINK
                    }
                end
                local cmd = concat_s(parts)
                printcmd(cmd)
                if (os.execute("mkdir -p " .. OUTPUTS .. entry.path .. "/") ~= 0) then
                    error("failed to make directory: " .. OUTPUTS .. entry.path .. "/");
                end
                if (os.execute(cmd) ~= 0) then
                    error("failed to compile: " .. entry.full)
                end
            end
        end
        writeb("linking: ", TERM_GREEN)
        print(FINAL)
        local genstr = ""
        if #LIBRARIES > 0 then
            for i = 1, #LIBRARIES do
                genstr = genstr .. LINKER_LIBRARYFLAG .. LIBRARIES[i]
                if (#LIBRARIES ~= i) then genstr = genstr .. " " end
            end
        end
        local obj_list = {}
        for k, entry in sort_files(OUTPUTS, "o") do
            obj_list[#obj_list + 1] = entry.full
        end
        local parts = {
            LINKER, LINKER_ARGS, genstr, table.concat(obj_list, " "), LINKER_OUTFLAG, FINAL
        }
        local cparts = {
            LINKER, LINKER_ARGS, genstr, "{objects}", LINKER_OUTFLAG, FINAL
        }
        printcmd(concat_s(cparts))
        
        if (os.execute(concat_s(parts)) ~= 0) then
            error("failed to link: " .. FINAL)
        end

        if debug_mode ~= true then
            writeb("stripping: ", TERM_GREEN);
            print(FINAL)
            local strip_ignore = concat_s(PRESERVE_SYMBOLS, "-K")
            local sparts = {
                "strip", AGGRESSIVE_STRIP and "--strip-all" or "--strip-unneeded",
                strip_ignore, FINAL
            }
            printcmd(concat_s(sparts))
            if (os.execute(concat_s(sparts)) ~= 0) then
                error("failed to strip: " .. FINAL)
            end
        end
        goals.mvn()
    end,
    install = function()
        goals.load_native()
        goals.prep()
        error("stub")
    end,
    clean = function()
        local cmd = "rm -rf " .. OUTPUTS
        printb("cleaning output directory...", TERM_GREEN)
        printcmd(cmd)
        os.execute(cmd)
    end,
    load_native = function()
        BUILD_CMD = concat_s {
            COMPILER, LUA_CFLAGS, NATIVE_LIB, COMPILER_OUTFLAG, "build.o", COMPILER_SKIP_LINK
        }

        LINK_CMD = concat_s {
            LINKER, LUA_LFLAGS, LINKER_LIBRARYFLAG, LUA_LINK,
            "build.o", LINKER_OUTFLAG, "build.so"
        }

        printb(string.format("compiling %s...", NATIVE_LIB), TERM_GREEN)
        printcmd(BUILD_CMD)

        if os.execute(BUILD_CMD) ~= 0 then
            error("failed to compile " .. NATIVE_LIB)
        end

        printb(string.format("linking %s...", NATIVE_LIB), TERM_GREEN)
        printcmd(LINK_CMD)

        if os.execute(LINK_CMD) ~= 0 then
            error("failed to link " .. NATIVE_LIB)
        end

        package.loadlib("./build.so", "entry")()

        -- replace our ugly hack with a less hacky one
        os.execute = C.system;
    end
}

-- prevent goals from being ran twice in single execution
for k, v in pairs(goals) do
    local temp = goals[k]
    goals[k] = function(...)
        if _G["once_" .. k] == nil then
            temp(...)
            _G["once_" .. k] = true
        end
    end
end

-- colors

TERM_CHAR = string.char(0x1b)
TERM_RED = TERM_CHAR .. "[31m"
TERM_GREEN = TERM_CHAR .. "[32m"
TERM_CMD = TERM_CHAR .. "[36m"
TERM_BOLD = TERM_CHAR .. "[1m"
TERM_RESET = TERM_CHAR .. "[0m"

__error = error
function error(message, level)
    printb(message, TERM_RED)
    __error("aborting...", 2)
end

function printc(message, c) print(c .. message .. TERM_RESET) end
function printb(message, c) print(TERM_BOLD .. c .. message .. TERM_RESET) end
function writec(message, c) io.write(c .. message .. TERM_RESET) end
function writeb(message, c) io.write(TERM_BOLD .. c .. message .. TERM_RESET) end
function printcmd(cmd) print(TERM_BOLD .. "-> " .. TERM_RESET .. TERM_CMD .. cmd .. TERM_RESET) end

-- assert programs exist
function prereq(tbl)
    if FHS_COMPLIANT_SYSTEM then
        printc("checking for prerequisite programs...", TERM_GREEN);
        for i = 1, #tbl do
            if C.findprog(tbl[i]) == false then
                error("program '" .. tbl[i] .. "' is not installed.");
            else print("'" .. tbl[i] .. "' is installed") end
        end
    end
end

-- assert libraries exist using GCC options
function prereq_libs(tbl)
    local paths = nil;
    if GCC_BASED_COMPILER then
        printc("checking for prerequisite libraries...", TERM_GREEN);
        local handle = io.popen(COMPILER .. " --print-search-dirs")
        for ln in handle:lines() do
            if (string.sub(ln, 1, 12) == "libraries: =") then
                paths = {}
                local path_list = string.sub(ln, 13)
                local seg = 0
                while true do
                    local idx = string.find(path_list, ":", seg + 1, true)
                    if idx ~= nil then
                        paths[#paths + 1] = trim_node(string.sub(path_list, seg + 1, idx - 1))
                        seg = idx;
                    else break end
                end
                break
            end
        end
        handle:close()
    else return end
    if paths == nil and GCC_BASED_COMPILER then
        printc("Couldn't retrieve library search directory from GCC!", TERM_RED)
        return
    end
    for i = 1, #tbl do
        local exists = false
        for t = 1, #paths do
            if C.isfile(paths[t] .. "/" .. tbl[i]) then exists = true break end
        end
        if ~exists then
            error("library '" .. tbl[i] .. "' is not installed.");
        end
    end
end

-- recursive iterate over files
-- returns table with 'file' set to the filename, 'path' to its folder relative to the 'folder' arg, and
-- 'full', which corresponds to the full path of the file
function r_iter(folder, path, base)
    if (path == nil) then path = "" end
    if (base == nil) then base = folder end
    local tbl = {}
    local list = C.list(folder)
    if (list == nil) then
        error(folder .. " does not exist")
    end
    for i = 1, #list do
        local node = path .. "/" .. list[i]
        if C.isfile(base .. "/" .. node) == false then
            local next_list = r_iter(base .. "/" .. node, node, base)
            for v = 1, #next_list do
                tbl[#tbl + 1] = next_list[v]
            end
        else
            tbl[#tbl + 1] = {file = list[i], path = path, full = base .. node}
        end
    end
    return tbl
end

-- find files with a given extension, using r_iter
function sort_files_t(root, ...)
    local list = r_iter(root)
    for i = 1, #list do
        local entry = list[i].file
        local ext_idx = string.find(entry, ".", 1, true)
        local rm = true
        for k, v in ipairs({...}) do
            if string.sub(entry, ext_idx + 1) == v then
                rm = false
                break
            end
        end
        if rm then
            list[i] = nil
        else
            list[i].filen = string.sub(entry, 1, ext_idx - 1)
        end
    end
    return list
end

function sort_files(root, ...)
    return pairs(sort_files_t(root, ...))
end

-- prevent a node from ending with a "/" (doesn't affect anything, just makes a weird path)
function trim_node(base)
    while string.sub(base, base:len()) == "/" do
        base = string.sub(base, 1, base:len() - 1)
    end
    return base
end

-- print lua version
print("Lua environment: " .. _VERSION);
-- assert we have a working shell
assert(__old_exec(nil) ~= 0)
-- assert there's an argument
assert(arg[1] ~= nil)
-- execute goal
goals[arg[1]]()
