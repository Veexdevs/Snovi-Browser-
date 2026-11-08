"""Check SDK discovery and propagation without downloading the Android SDK."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

script = Path(__file__).resolve().parents[1] / 'Mobile/scripts/prepare-android-sdk.sh'
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp) / 'android sdk'  # Exercise paths containing spaces too.
    manager = root / 'cmdline-tools/latest/bin/sdkmanager'
    manager.parent.mkdir(parents=True)
    manager.write_text('#!/usr/bin/env bash\nprintf "%s\\n" "$@" > "$ARGS_FILE"\n')
    manager.chmod(0o755)
    env = dict(os.environ, ANDROID_HOME=str(root), GITHUB_PATH=str(Path(tmp) / 'github_path'), ARGS_FILE=str(Path(tmp) / 'args'))
    subprocess.run([str(script)], env=env, check=True)
    assert (Path(tmp) / 'args').read_text().splitlines() == [
        '--sdk_root=' + str(root), '--install', 'ndk;27.2.12479018',
        'cmake;3.22.1', 'platforms;android-35', 'build-tools;35.0.0']
    assert (Path(tmp) / 'github_path').read_text().splitlines() == [
        str(root / 'cmdline-tools/latest/bin'), str(root / 'platform-tools'), str(root / 'emulator')]
    manager.unlink()
    result = subprocess.run([str(script)], env=env, text=True, capture_output=True)
    assert result.returncode != 0 and 'command-line tools not found' in result.stderr
print('Android SDK setup checks passed')
