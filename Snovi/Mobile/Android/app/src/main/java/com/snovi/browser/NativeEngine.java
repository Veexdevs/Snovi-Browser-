package com.snovi.browser;

import java.nio.charset.StandardCharsets;
import org.json.JSONObject;

final class NativeEngine {
    static { System.loadLibrary("snovi_bridge"); }
    private static native long create(String caPath);
    private static native void destroy(long handle);
    private static native byte[] execute(long handle, byte[] action, byte[] value);
    private long handle;
    NativeEngine(String caPath) {
        handle = create(caPath);
        if (handle == 0) throw new IllegalStateException("Falha ao iniciar motor Snovi");
    }
    JSONObject run(String action, String value) throws Exception {
        byte[] data = execute(handle, action.getBytes(StandardCharsets.UTF_8), value.getBytes(StandardCharsets.UTF_8));
        if (data == null) throw new IllegalStateException("Falha no motor Snovi");
        return new JSONObject(new String(data, StandardCharsets.UTF_8));
    }
    void close() { if (handle != 0) { destroy(handle); handle=0; } }
}
