"""
ESP32 Tomato — PlatformIO pre-build script
Automatically builds the SPIFFS filesystem image before upload.
"""
Import("env")

def before_build(source, target, env):
    print("ESP32 Tomato: Checking SPIFFS data folder...")
    import os
    data_dir = os.path.join(env["PROJECT_DIR"], "data")
    if not os.path.exists(data_dir):
        print("[WARNING] data/ folder not found — web UI will not be available")
        return
    files = os.listdir(data_dir)
    print(f"[SPIFFS] Found {len(files)} files in data/: {', '.join(files)}")

env.AddPreAction("upload", before_build)
