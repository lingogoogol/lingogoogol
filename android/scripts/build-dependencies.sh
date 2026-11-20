#!/usr/bin/env bash
# Build the third-party libraries of the Android app into <prefix>/<ABI>/{include,lib}.
#
# Usage: build-dependencies.sh [prefix] [ABI...]
#   prefix  defaults to ../lib/android beside the repository (where CMake looks by default)
#   ABI     defaults to "arm64-v8a x86_64"
# Environment: ANDROID_NDK (default: newest NDK under $ANDROID_HOME/ndk), ANDROID_API (29), JOBS (8).
#
# Versions match the Windows build: OpenSSL 3.4.1, curl 8.23.0 (no zlib, like the
# Windows libcurl.dll), zstd 1.5.7, FreeType 2.14.1, Vulkan-Headers 1.4.341.
# Header-only: GLM 1.0.1, nlohmann/json 3.12.0, and GLFW 3.5.1's glfw3.h, whose API
# the Android GLFW layer implements. zlib is the NDK's system libz.
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
prefix=${1:-$script_dir/../../../lib/android}
shift || true
abis=("$@")
[ ${#abis[@]} -gt 0 ] || abis=(arm64-v8a x86_64)
api=${ANDROID_API:-29}
jobs=${JOBS:-8}
if [ -z "${ANDROID_NDK:-}" ]; then
    ANDROID_NDK=$(ls -d "${ANDROID_HOME:?set ANDROID_HOME or ANDROID_NDK}"/ndk/* | sort -V | tail -1)
fi
host_bin=$(echo "$ANDROID_NDK"/toolchains/llvm/prebuilt/*/bin)
mkdir -p "$prefix"
prefix=$(cd "$prefix" && pwd)
work=${WORK_DIR:-$prefix/.build}
sources=$work/sources
mkdir -p "$sources"

fetch() { # name url sha256
    local archive=$sources/$(basename "$2")
    if [ ! -f "$archive" ] || ! echo "$3  $archive" | sha256sum -c --quiet - 2>/dev/null; then
        curl -fsSL -o "$archive" "$2"
        echo "$3  $archive" | sha256sum -c --quiet -
    fi
    if [ ! -d "$sources/$1" ]; then
        mkdir -p "$sources/$1.tmp"
        case $archive in
            *.zip) unzip -q "$archive" -d "$sources/$1.tmp" ;;
            *) tar -xf "$archive" -C "$sources/$1.tmp" ;;
        esac
        mv "$sources/$1.tmp"/* "$sources/$1"
        rmdir "$sources/$1.tmp"
    fi
}

fetch openssl https://github.com/openssl/openssl/releases/download/openssl-3.4.1/openssl-3.4.1.tar.gz \
    002a2d6b30b58bf4bea46c43bdd96365aaf8daa6c428782aa4feee06da197df3
fetch curl https://github.com/curl/curl/releases/download/curl-8_23_0/curl-8.23.0.tar.xz \
    8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5
fetch zstd https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-1.5.7.tar.gz \
    eb33e51f49a15e023950cd7825ca74a4a2b43db8354825ac24fc1b7ee09e6fa3
fetch freetype https://github.com/freetype/freetype/archive/refs/tags/VER-2-14-1.tar.gz \
    44bd69d1f0750603410cfd4f26f1c5523c5a3a087a2dfa8639b757dc7c6677be
fetch vulkan-headers https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.341.tar.gz \
    8876aad926b2e72bdefddc34885472d5332e2d20aa3ed57313466f0bc982cf3f
fetch glm https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz \
    9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c
fetch json https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz \
    42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
fetch glfw https://github.com/glfw/glfw/releases/download/3.5.1/glfw-3.5.1.zip \
    ea79bc5feffc254c87291980c2d0bce9acebb68c4983b79f961dcd2cb8a611a0

for abi in "${abis[@]}"; do
    case $abi in
        arm64-v8a) openssl_target=android-arm64 ;;
        x86_64) openssl_target=android-x86_64 ;;
        *) echo "Unsupported ABI: $abi" >&2; exit 1 ;;
    esac
    out=$prefix/$abi
    build=$work/$abi
    mkdir -p "$out/include" "$build"

    cm() { # source-subdir cmake-args...
        local name=$1; shift
        local dir=$build/${name//\//_}
        echo "### $abi $name"
        cmake -S "$sources/$name" -B "$dir" -G Ninja -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
            -DANDROID_ABI="$abi" -DANDROID_PLATFORM="$api" -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DCMAKE_INSTALL_PREFIX="$out" -DCMAKE_FIND_ROOT_PATH="$out" "$@" > "$dir.configure.log" 2>&1
        cmake --build "$dir" -j"$jobs" > "$dir.build.log" 2>&1
        cmake --install "$dir" > "$dir.install.log" 2>&1
    }

    cm zstd/build/cmake -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_STATIC=ON -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_TESTS=OFF
    cm freetype -DBUILD_SHARED_LIBS=OFF -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON \
        -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
    cm vulkan-headers

    echo "### $abi openssl"
    mkdir -p "$build/openssl"
    (
        cd "$build/openssl"
        export ANDROID_NDK_ROOT=$ANDROID_NDK PATH="$host_bin:$PATH"
        "$sources/openssl/Configure" "$openssl_target" -D__ANDROID_API__="$api" \
            --prefix="$out" --libdir=lib no-shared no-tests no-docs no-apps > configure.log 2>&1
        make -j"$jobs" > build.log 2>&1
        make install_sw > install.log 2>&1
    )

    cm curl -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DCURL_USE_OPENSSL=ON -DOPENSSL_ROOT_DIR="$out" \
        -DOPENSSL_USE_STATIC_LIBS=ON -DCURL_ZLIB=OFF -DCURL_BROTLI=OFF -DCURL_ZSTD=OFF -DUSE_NGHTTP2=OFF \
        -DCURL_USE_LIBPSL=OFF -DCURL_USE_LIBSSH2=OFF -DUSE_LIBIDN2=OFF -DCURL_DISABLE_LDAP=ON \
        -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF

    echo "### $abi headers"
    rm -rf "$out/include/glm" "$out/include/nlohmann" "$out/include/GLFW"
    cp -r "$sources/glm/glm" "$out/include/"
    cp -r "$sources/json/include/nlohmann" "$out/include/"
    mkdir -p "$out/include/GLFW"
    cp "$sources/glfw/include/GLFW/glfw3.h" "$out/include/GLFW/"
done
echo "Android dependencies are in $prefix"
