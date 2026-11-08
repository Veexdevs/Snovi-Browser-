#!/usr/bin/env bash
# GitHub's Ubuntu image installs sdkmanager under the SDK, but not on PATH.
set -euo pipefail
: "${ANDROID_HOME:?ANDROID_HOME must point to the Android SDK}"
sdkmanager="$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager"
if [[ ! -x "$sdkmanager" ]]; then
  echo "Android SDK command-line tools not found at: $sdkmanager" >&2
  exit 1
fi
export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$ANDROID_HOME/emulator:$PATH"
if [[ -n "${GITHUB_PATH:-}" ]]; then
  printf '%s\n' "$ANDROID_HOME/cmdline-tools/latest/bin" "$ANDROID_HOME/platform-tools" "$ANDROID_HOME/emulator" >> "$GITHUB_PATH"
fi
"$sdkmanager" --sdk_root="$ANDROID_HOME" --install \
  'ndk;27.2.12479018' 'cmake;3.22.1' 'platforms;android-35' 'build-tools;35.0.0'
