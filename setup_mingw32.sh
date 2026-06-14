#!/bin/bash

# WARNING
# Strictly for targeting i686 CPUs (Pentium Pro, Pentium II, and later with CMOV support)
# Please note that this cross-compilation environment using Wine wrappers is experimental

CONFIRM=false
for arg in "$@"; do
    if [ "$arg" = "--confirm" ]; then
        CONFIRM=true
    fi
done

if [ "$CONFIRM" = "false" ]; then
    echo "WARNING"
    echo "Strictly for targeting i686 CPUs (Pentium Pro, Pentium II, and later with CMOV support)"
    echo "Please note that this cross-compilation environment using Wine wrappers is experimental"
    echo ""
    read -n 1 -s -r -p "Press any key to continue..."
    echo ""
fi

for cmd in curl 7z wine winepath sed; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "Error: $cmd is not installed."
        exit 1
    fi
done

URL="https://github.com/redpanda-cpp/mingw-lite/releases/download/15.2.0-r8/mingw32_686-msvcrt_win98-15.2.0-r8.7z"
ARCHIVE="mingw_compiler.7z"

rm -rf mingw temp_extracted "$ARCHIVE"

echo "Downloading..."
echo ""
curl -L -o "$ARCHIVE" "$URL"

echo "Extracting..."
mkdir -p temp_extracted
7z x "$ARCHIVE" -otemp_extracted -bsp1 -bso0

echo ""
echo "Moving files to target directory..."
SRC_DIR=$(find temp_extracted -type d -name "bin" -exec dirname {} \; -quit)
mkdir -p mingw
mv "$SRC_DIR"/* ./mingw/

rm -rf temp_extracted "$ARCHIVE"

echo "Writing some workarounds.."
cat << 'EOF' > mingw/bin/i686-w64-mingw32-gcc-wrapper
#!/bin/bash
REAL_COMPILER="$(dirname "$0")/i686-w64-mingw32-gcc.exe"
ARGS=()
DEP_FILE=""
NEXT_IS_DEP=false

for arg in "$@"; do
    if [ "$NEXT_IS_DEP" = true ]; then
        DEP_FILE="$arg"
        NEXT_IS_DEP=false
    fi
    if [ "$arg" = "-MF" ]; then
        NEXT_IS_DEP=true
    fi

    if [[ "$arg" =~ ^(-I|-L|-o)(/.*) ]]; then
        flag="${BASH_REMATCH[1]}"
        unix_path="${BASH_REMATCH[2]}"
        win_path=$(winepath -w "$unix_path")
        ARGS+=("$flag$win_path")
    elif [[ "$arg" =~ ^(/.*) ]]; then
        win_path=$(winepath -w "$arg")
        ARGS+=("$win_path")
    else
        ARGS+=("$arg")
    fi
done

WINEDEBUG=-all wine "$REAL_COMPILER" "${ARGS[@]}"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && [ ! -z "$DEP_FILE" ] && [ -f "$DEP_FILE" ]; then
    sed -i -e 's|[a-zA-Z]:/|/|g' -e 's|[a-zA-Z]:\\|/|g' -e 's|\\|/|g' "$DEP_FILE"
fi

exit $EXIT_CODE
EOF

cat << 'EOF' > mingw/bin/i686-w64-mingw32-g++-wrapper
#!/bin/bash
REAL_COMPILER="$(dirname "$0")/i686-w64-mingw32-g++.exe"
ARGS=()
DEP_FILE=""
NEXT_IS_DEP=false

for arg in "$@"; do
    if [ "$NEXT_IS_DEP" = true ]; then
        DEP_FILE="$arg"
        NEXT_IS_DEP=false
    fi
    if [ "$arg" = "-MF" ]; then
        NEXT_IS_DEP=true
    fi

    if [[ "$arg" =~ ^(-I|-L|-o)(/.*) ]]; then
        flag="${BASH_REMATCH[1]}"
        unix_path="${BASH_REMATCH[2]}"
        win_path=$(winepath -w "$unix_path")
        ARGS+=("$flag$win_path")
    elif [[ "$arg" =~ ^(/.*) ]]; then
        win_path=$(winepath -w "$arg")
        ARGS+=("$win_path")
    else
        ARGS+=("$arg")
    fi
done

WINEDEBUG=-all wine "$REAL_COMPILER" "${ARGS[@]}"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && [ ! -z "$DEP_FILE" ] && [ -f "$DEP_FILE" ]; then
    sed -i -e 's|[a-zA-Z]:/|/|g' -e 's|[a-zA-Z]:\\|/|g' -e 's|\\|/|g' "$DEP_FILE"
fi

exit $EXIT_CODE
EOF

cat << 'EOF' > mingw/bin/i686-w64-mingw32-windres-wrapper
#!/bin/bash
REAL_COMPILER="$(dirname "$0")/windres.exe"
ARGS=()
for arg in "$@"; do
    if [[ "$arg" =~ ^(-I|-O|-o)(/.*) ]]; then
        flag="${BASH_REMATCH[1]}"
        unix_path="${BASH_REMATCH[2]}"
        win_path=$(winepath -w "$unix_path")
        ARGS+=("$flag$win_path")
    elif [[ "$arg" =~ ^(/.*) ]]; then
        win_path=$(winepath -w "$arg")
        ARGS+=("$win_path")
    else
        ARGS+=("$arg")
    fi
done
WINEDEBUG=-all wine "$REAL_COMPILER" "${ARGS[@]}"
EOF

chmod +x mingw/bin/i686-w64-mingw32-gcc-wrapper
chmod +x mingw/bin/i686-w64-mingw32-g++-wrapper
chmod +x mingw/bin/i686-w64-mingw32-windres-wrapper

echo ""
echo "Done!"
echo "Now you can configure your project using"
echo "  cmake -DCMAKE_TOOLCHAIN_FILE=cmake/windows-win9x-mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=BUILD_TYPE .."