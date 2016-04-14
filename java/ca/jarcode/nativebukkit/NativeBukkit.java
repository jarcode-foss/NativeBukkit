
package ca.jarcode.nativebukkit;

import org.bukkit.*;
import org.bukkit.event.*;
import org.bukkit.plugin.*;
import org.bukkit.configuration.*;
import org.bukkit.configuration.file.*;
import org.bukkit.plugin.java.*;
import org.bukkit.command.*;
import org.bukkit.generator.*;

import java.util.*;
import java.util.logging.*;
import java.util.regex.*;

import java.io.*;
import java.nio.file.*;

import jni.*;

public class NativeBukkit extends JavaPlugin {
	
	private static final String NATIVE_LIBRARY = "nativebukkit.so";
	private static boolean LOADED_NATIVES = false;
	private static Loader LOADER;
	private static final boolean INIT_ON_STARTUP = false; /* disable for debugging init */
	private static NativeBukkit instance;

	private final Logger b = Bukkit.getLogger();
	
	{ instance = this; }
	
	public void onEnable() {
		if (!LOADED_NATIVES) {
			File result = null;
			InputStream stream = null;
			try {
				stream = this.getClass().getClassLoader().getResourceAsStream(NATIVE_LIBRARY);
				if (stream == null)
					throw new RuntimeException("Could not obtain stream to native library (null)");
				File folder = getDataFolder();
				if (!folder.exists())
					if (!folder.mkdir())
						throw new RuntimeException("Failed to create plugin folder: " + folder.getAbsolutePath());
				result = new File(folder, NATIVE_LIBRARY);
				if (result.exists()) result.delete();
				Files.copy(stream, Paths.get(result.getAbsolutePath()));
			} catch (Throwable e) {
				throw new RuntimeException(e);
			} finally {
				if (stream != null) {
					try {
						stream.close();
					} catch (Throwable ignored) {}
				}
			}
			System.load(result.getAbsolutePath());
			LOADED_NATIVES = true;
			new JNIEntry(this).entry();
			/* Provide a simple loader for native plugins */
			Bukkit.getPluginManager().registerInterface(Loader.class);
			/* Load native plugins through the plugin manager */
			for (File f : getFile().getParentFile().listFiles()) {
				if (!f.isDirectory() && f.getName().endsWith(".so")) {
					try {
						b.info(String.format("Loading native plugin '%s'...", f.getName()));
						Plugin plugin = Bukkit.getPluginManager().loadPlugin(f);
						Bukkit.getPluginManager().enablePlugin(plugin);
					} catch (InvalidPluginException | InvalidDescriptionException e) {
						e.printStackTrace();
					}
				}
			}
		}
	}

	/* Native plugin loader, ignores traditional listener registration and provides stub descriptions */
	public static class Loader implements PluginLoader {
		public Loader(Server server) {}
		public Map<Class<? extends Event>, Set<RegisteredListener>>
			createRegisteredListeners(Listener listener, Plugin plugin) {
			throw new RuntimeException("Cannot register listener classes for native plugins");
		}
		public void disablePlugin(Plugin plugin) { ((NativePlugin) plugin).onDisable(); }
		public void enablePlugin(Plugin plugin) { ((NativePlugin) plugin).onEnable(); }
		public PluginDescriptionFile getPluginDescription(File file) {
			return new PluginDescriptionFile(file.getName(), "unknown", "native");
		}
		public Pattern[] getPluginFileFilters() { return new Pattern[] { Pattern.compile("\\.so$") }; }
		public Plugin loadPlugin(File file) {
			return new NativePlugin(file, this, JNIPlugin.open(file.getAbsolutePath()));
		}
	}
	
	/* Plugin implementation for native libraries */
	/* Most of the implementations in this class are stubs */
	public static class NativePlugin implements Plugin {
		private boolean enabled = false;
		private final File lib;
		private final JNIPlugin plugin;
		private final Loader loader;
		private NativePlugin(File lib, Loader loader, long handle) {
			this.lib = lib;
			this.loader = loader;
			plugin = new JNIPlugin(handle, this);
			plugin.onLoad();
		}
		public FileConfiguration getConfig() { return null; } /* ignore */
		public com.avaje.ebean.EbeanServer getDatabase() { return null; } /* stub */
		public File getDataFolder() {
			return new File(instance.getFile().getParentFile(), getName().split(".")[0]);
		}
		public ChunkGenerator getDefaultWorldGenerator(String worldName, String id) { return null; }
		/* native plugins don't include any extra information */
		public PluginDescriptionFile getDescription() {
			return new PluginDescriptionFile(lib.getName(), "unknown", "native");
		}
		public Logger getLogger() { return Bukkit.getLogger(); } /* never used */
		public String getName() { return lib.getName(); }
		public PluginLoader getPluginLoader() { return loader; }
		public InputStream getResource(String filename) { return null; } /* never used */
		public Server getServer() { return Bukkit.getServer(); }
		public boolean isEnabled() { return enabled; }
		public boolean isNaggable() { return false; } /* never used */
		public void onDisable() {
			if (enabled) {
				plugin.onDisable();
				enabled = false;
			}
		}
		public void onEnable() {
			if (!enabled) {
				plugin.onEnable();
				enabled = true;
			}
		}
		public void onLoad() { plugin.onLoad(); }
		public void reloadConfig() {} /* ignore */
		public void saveConfig() {} /* ignore */
		public void saveDefaultConfig() {} /* ignore */
		public void saveResource(String path, boolean replace) {} /* ignore */
		public void setNaggable(boolean nag) {} /* never used */
		public List<String>
			onTabComplete(CommandSender sender, Command command, String alias, String[] args) {
			return new ArrayList<String>();
		}
		public boolean
			onCommand(CommandSender sender, Command command, String label, String[] args) { return true; }
	}
}
