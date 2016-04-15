
package jni;

public final class JNIRunnable implements Runnable {
	private final long __handle;
	private final long __udata;
	/* (void* udata, void (*ptr) (void* udata)) */
	public JNIRunnable(long udata, long handle) {
		__handle = handle; /* function to be executed */
		__udata = udata;   /* extra instance data     */
	}
	@Override
	public native void run();
}
