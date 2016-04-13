# NativeBukkit

**Do not use this unless you have a very good reason for it!**

NativeBukkit is two things:

- a bukkit plugin (containing its own plugin loader) that allows the loading of .so libraries as plugins for a minecraft server.

- a version of the Bukkit API extended in pure C, allowing developers to write plugins in native code using NativeBukkit's header files.

Because NativeBukkit only works with ELF dynamic libraries, Linux and BSD will be the only supported platform. I don't care about your proprietary platform.

### Performance

NativeBukkit uses the JNI and its own wrapper structures and functions to call into Java code. When first initialized, NativeBukkit will resolve method IDs/handles for all of Bukkit's types, methods, and classes - leaving invocation with a low overhead.

However, some operations (ie. wrapping Bukkit types) will require heavier interaction with the JNI. This means handling operations such as events will be **slower** in native code than in pure Java. **NativeBukkit is only a viable option for performance and memory usage if your plugin does most of its operations independant of the Bukkit API.**

### Supported languages

You can write plugins in native code as long as you can interact with C functions and types and compile to a shared object (.so). This includes C, C++, D, Rust, and many, many others.

Some languages will be very difficult to use with NativeBukkit. You need to pass function pointers to the NativeBukkit API for various reasons, call functions from a pointer, and work with C-style strings. I recommend writing your plugins in C/C++ or Rust for these compatibility reasons.