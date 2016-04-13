
package jni;

public final class JNIPlugin {

	public static native long open(String path);
	
	private final long __handle;
	public JNIPlugin(long handle) {
		__handle = handle;
	}
	public native void onEnable();
	public native void onDisable();
	public native void onLoad();
	public native void close();
}
