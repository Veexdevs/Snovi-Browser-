"""Typed ctypes wrapper around the Snovi C++ engine."""
import ctypes
import json
from pathlib import Path
import os
import sys


class Engine:
    def __init__(self, library=None):
        if getattr(sys, "frozen", False):
            default = Path(sys._MEIPASS) / "snovi_bridge.dll"
        elif sys.platform == "win32":
            root = Path(__file__).resolve().parents[3]
            default = root / "build" / "Release" / "snovi_bridge.dll"
        else:
            root = Path(__file__).resolve().parents[3]
            default = root / "build" / "libsnovi_bridge.so"
        path = Path(library or os.environ.get("SNOVI_ENGINE_LIBRARY", default))
        self.lib = ctypes.CDLL(str(path))
        self.lib.snovi_create.restype = ctypes.c_void_p
        self.lib.snovi_execute.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
        self.lib.snovi_execute.restype = ctypes.c_void_p
        self.lib.snovi_free.argtypes = [ctypes.c_void_p]
        self.lib.snovi_destroy.argtypes = [ctypes.c_void_p]
        self.handle = self.lib.snovi_create()
        if not self.handle:
            raise RuntimeError("Não foi possível iniciar o motor Snovi")

    def execute(self, action, value=""):
        result = self.lib.snovi_execute(self.handle, action.encode(), value.encode())
        if not result:
            raise RuntimeError("Falha de memória no motor Snovi")
        try:
            return json.loads(ctypes.string_at(result).decode("utf-8", errors="replace"))
        finally:
            self.lib.snovi_free(result)

    def close(self):
        if self.handle:
            self.lib.snovi_destroy(self.handle)
            self.handle = None
