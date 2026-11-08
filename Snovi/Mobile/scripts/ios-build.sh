#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
curl --fail --location --retry 3 'https://codeload.github.com/openssl/openssl/tar.gz/refs/tags/openssl-3.5.9' -o "$work/openssl.tar.gz"
echo "796831ec533bf41998eaaee9d632d921b8a69f57394fa4086ff01a941113d26d  $work/openssl.tar.gz" | shasum -a 256 -c -
mkdir -p "$work/openssl"
tar xzf "$work/openssl.tar.gz" -C "$work/openssl" --strip-components=1
pushd "$work/openssl" >/dev/null
./Configure ios64-xcrun no-shared no-tests no-asm --libdir=lib --prefix="$work/install" -miphoneos-version-min=15.0
make -j"$(sysctl -n hw.ncpu)"
make install_sw
popd >/dev/null
cmake -S "$root/Snovi" -B "$work/build" \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_BUILD_TYPE=Release -DSNOVI_MOBILE=ON \
  -DOPENSSL_USE_STATIC_LIBS=TRUE -DOPENSSL_INCLUDE_DIR="$work/install/include" \
  -DOPENSSL_SSL_LIBRARY="$work/install/lib/libssl.a" -DOPENSSL_CRYPTO_LIBRARY="$work/install/lib/libcrypto.a"
cmake --build "$work/build" --target snovi_bridge --parallel
out="$root/mobile-artifacts"; mkdir -p "$out" "$work/Payload/Snovi.app"
app="$work/Payload/Snovi.app"
cp "$root/Snovi/Mobile/iOS/Info.plist" "$app/Info.plist"
cp "$root/Snovi/Mobile/iOS/ca-bundle.pem" "$app/ca-bundle.pem"
sdk=$(xcrun --sdk iphoneos --show-sdk-path)
xcrun --sdk iphoneos swiftc -swift-version 5 -parse-as-library -O -sdk "$sdk" -target arm64-apple-ios15.0 \
  "$root/Snovi/Mobile/iOS/Snovi.swift" "$work/build/libsnovi_bridge.a" "$work/build/libsnovi_core.a" \
  "$work/install/lib/libssl.a" "$work/install/lib/libcrypto.a" -Xlinker -lc++ \
  -framework UIKit -framework Foundation -framework Security -framework CoreFoundation -o "$app/Snovi"
plutil -lint "$app/Info.plist"
lipo -verify_arch arm64 "$app/Snovi"
(cd "$work" && zip -qry "$out/Snovi-iOS-unsigned.ipa" Payload)
unzip -t "$out/Snovi-iOS-unsigned.ipa"
