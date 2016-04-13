
package jni;

import org.bukkit.plugin.java.JavaPlugin;

public final class JNIEntry {

	private final JavaPlugin plugin;
	
	public JNIEntry(JavaPlugin plugin) {
		this.plugin = plugin;
	}
	
	public native void entry();
}
