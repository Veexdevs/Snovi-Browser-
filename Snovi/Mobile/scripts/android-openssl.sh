#!/usr/bin/env bash
set -euo pipefail
: "${ANDROID_NDK_ROOT:?Set ANDROID_NDK_ROOT}"
root=$(cd "$(dirname "$0")/../../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
curl --fail --location --retry 3 'https://codeload.github.com/openssl/openssl/tar.gz/refs/tags/openssl-3.5.9' -o "$work/openssl.tar.gz"
echo "796831ec533bf41998eaaee9d632d921b8a69f57394fa4086ff01a941113d26d  $work/openssl.tar.gz" | shasum -a 256 -c -
export PATH="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH"
for entry in 'arm64-v8a android-arm64' 'x86_64 android-x86_64'; do
  read -r abi target <<< "$entry"
  mkdir -p "$work/$abi"
  tar xzf "$work/openssl.tar.gz" -C "$work/$abi" --strip-components=1
  pushd "$work/$abi" >/dev/null
  ./Configure "$target" -D__ANDROID_API__=26 no-shared no-tests -fPIC --libdir=lib --prefix="$root/Snovi/Mobile/Android/openssl/$abi"
  make -j"$(nproc)"
  make install_sw
  popd >/dev/null
done
