
package jni;

import org.bukkit.plugin.Plugin;

import ca.jarcode.nativebukkit.NativeBukkit;
import ca.jarcode.nativebukkit.NativeBukkit.*;

public final class JNIPlugin {

	public static native long open(String path);
	
	private final long __handle;
	private final String __name;
	private long __internal = 0;
	private final Plugin __plugin;
	
	public JNIPlugin(long handle, NativePlugin plugin) {
		__handle = handle;
		__name = plugin.getName();
		__plugin = plugin;
	}
	public native void onEnable();
	public native void onDisable();
	public native void onLoad();
	public native void close();
}
