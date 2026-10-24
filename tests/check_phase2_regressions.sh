#!/bin/bash
# Run in an isolated build environment with the edition's Qt development packages.
# Keep the application checkout unchanged while testing the pre-port source.
set -euo pipefail

if [[ $# != 2 ]]; then
    echo "Usage: $0 BASELINE_REVISION NEW_OUTPUT_DIRECTORY" >&2
    exit 2
fi
source_root=$(cd -- "$(dirname -- "$0")/.." && pwd)
baseline=$(git -C "$source_root" rev-parse --verify "$1^{commit}")
output=$(realpath -m -- "$2")
if [[ -e "$output" ]]; then
    echo "Output directory already exists: $output" >&2
    exit 2
fi
mkdir -p "$output"
exec > >(tee "$output/run.log") 2>&1
printf 'Source: %s\nBaseline: %s\n' "$(git -C "$source_root" rev-parse HEAD)" "$baseline"
export QT_QPA_PLATFORM=offscreen QML_DISABLE_DISK_CACHE=1 QSG_RHI_BACKEND=opengl
export XDG_CONFIG_HOME="$output/config" XDG_CACHE_HOME="$output/cache" XDG_RUNTIME_DIR="$output/runtime"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
configure=(-G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_UNIT_TESTS=ON
    -DBUILD_QT_UI=ON -DBUILD_QT3D_RENDERER=ON -DBUILD_SERENITY_RENDERER=OFF
    -DBUILD_KDGUI_UI=OFF -DBUILD_FLUTTER_UI=OFF -DFETCHCONTENT_FULLY_DISCONNECTED=ON)

cmake -S "$source_root" -B "$output/head" "${configure[@]}" -DBUILD_QUICK3D_RENDERER=ON > "$output/head-configure.log" 2>&1
cmake --build "$output/head" --parallel 4 > "$output/head-build.log" 2>&1
for name in stereo_frustum stereo_layout side_menu_layout quick3d_renderer; do
    test -x "$output/head/tests/auto/${name}_test"
done
ctest --test-dir "$output/head" --output-on-failure -V > "$output/head-tests.log" 2>&1
echo "Current source: full build and four test executables passed"

mkdir "$output/baseline-src"
git -C "$source_root" archive "$baseline" | tar -x -C "$output/baseline-src"
test ! -e "$output/baseline-src/shared/include/shared/stereo_layout.h"
test ! -e "$output/baseline-src/allegiance/renderer/quick3d/quick3d_renderer.h"
# Only the test changes: the baseline's side-menu implementation stays intact.
cp "$source_root/tests/auto/side_menu_layout.cpp" "$output/baseline-src/tests/auto/side_menu_layout.cpp"
cmake -S "$output/baseline-src" -B "$output/baseline" "${configure[@]}" > "$output/baseline-configure.log" 2>&1
cmake --build "$output/baseline" --parallel 4 --target side_menu_layout_test stereo_frustum_test > "$output/baseline-build.log" 2>&1
ctest --test-dir "$output/baseline" -R '^stereo_frustum$' --output-on-failure -V > "$output/baseline-frustum.log" 2>&1
if "$output/baseline/tests/auto/side_menu_layout_test" > "$output/baseline-side-menu.log" 2>&1; then
    echo "ERROR: the new side-menu tests unexpectedly passed on the baseline" >&2
    exit 1
fi
grep -F 'FAIL!  : SideMenuLayout::pathChoiceIsUnderAdvanced()' "$output/baseline-side-menu.log"
grep -F 'FAIL!  : SideMenuLayout::rendererChoiceIsUnderAdvanced()' "$output/baseline-side-menu.log"
grep -F 'Totals: 3 passed, 2 failed, 0 skipped' "$output/baseline-side-menu.log"

# These tests cannot compile before their production APIs exist. Check the
# precise missing header, so an unrelated compiler/setup error cannot pass.
if c++ -std=c++20 -fsyntax-only -I"$output/baseline-src/shared/include" \
    "$source_root/tests/auto/stereo_layout.cpp" > "$output/baseline-stereo-layout.log" 2>&1; then
    echo "ERROR: stereo_layout unexpectedly compiled on the baseline" >&2
    exit 1
fi
grep -F 'shared/stereo_layout.h: No such file or directory' "$output/baseline-stereo-layout.log"
read -r -a qt_cflags <<< "$(pkg-config --cflags Qt6Test)"
if c++ -std=c++20 -fsyntax-only "${qt_cflags[@]}" \
    -I"$output/baseline-src/allegiance" -I"$output/baseline-src/shared/include" \
    "$source_root/tests/auto/quick3d_renderer.cpp" > "$output/baseline-quick3d.log" 2>&1; then
    echo "ERROR: quick3d_renderer unexpectedly compiled on the baseline" >&2
    exit 1
fi
grep -F 'renderer/quick3d/quick3d_renderer.h: No such file or directory' "$output/baseline-quick3d.log"
echo "PASS: current source builds and passes; baseline retains the frustum pass and rejects all three phase 2 test groups"
