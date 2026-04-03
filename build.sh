#!/bin/bash
echo "build started."

# NOTE(cmat): Unpack arguments.
for arg in "$@"; do
  declare $arg='1';
done

# NOTE(cmat): Generate tags file.
# ctags -R .

# NOTE(cmat): Exit on error.
set -eu

# NOTE(cmat): Set working directory to the build.sh folder.
cd "$(dirname "$0")"

# NOTE(cmat): Remove old build directory.
rm -rf build/

# NOTE(cmat): Absolute path for source files (compiler errors are cleaner).
source_folder=$(realpath "./src")

# NOTE(cmat): Compilation parameters.
build_folder="build"

include_folders="-I${source_folder}"

define_flags=""

if [[ -n ${release-} ]]; then
define_flags+=" -DBUILD_DEBUG=0 -DBUILD_ASSERT=0"
else
define_flags+=" -DBUILD_DEBUG=1 -DBUILD_ASSERT=1"
fi

compiler_exec=""
source_files=""
compiler_flags=""
linker_flags=""

# NOTE(cmat): Entry point.
source_files+=" ${source_folder}/cfdr_client/cfdr_entry.c"

# ------------------------------------------------------------
# -- Clean build
mkdir -p $build_folder

# ------------------------------------------------------------
# -- WASM build path

# NOTE(cmat): 1 Megabyte
wasm_stack_size=$[1 * 1024 * 1024]

compiler_exec+="clang"

# compiler_flags+=" -fsanitize=address"
# compiler_flags+=" -fsanitize=undefined"
# compiler_flags+=" -fsanitize-address-use-after-scope"
# compiler_flags+=" -fno-omit-frame-pointer"

if [[ -n ${release-} ]]; then
compiler_flags+=" -O3"
else
compiler_flags+=" -O0"
compiler_flags+=" -g"
fi


# TODO(cmat): strlen optimizations are probably significant enough
# that we want to roll our own strlen
# NOTE(cmat): Clang will insert strlen calls, which calls into the CRT,
# which we have disabled.
compiler_flags+=" -fno-builtin-strlen"

compiler_flags+=" -std=c23"
compiler_flags+=" --target=wasm32"
compiler_flags+=" -ffreestanding"
compiler_flags+=" -nostdlib"
compiler_flags+=" -nodefaultlibs"
compiler_flags+=" -msimd128"

linker_flags+=" -flto"
linker_flags+=" -Wl,--export-table"
linker_flags+=" -Wl,--lto-O3"
linker_flags+=" -Wl,-z,stack-size=${wasm_stack_size}"
linker_flags+=" -Wl,--no-entry"
linker_flags+=" -Wl,-allow-undefined"
linker_flags+=" -o alice_canvas.wasm"

# NOTE(cmat): Copy required resources to build folder.
if [[ -n ${sze_portal-} ]]; then
cp "${source_folder}/cfdr_sze_portal/index.html"              "${build_folder}/"
cp "${source_folder}/cfdr_sze_portal/auth_sze_portal.js"      "${build_folder}/"
cp "${source_folder}/cfdr_sze_portal/silent-check-sso.html"   "${build_folder}/"
cp "${source_folder}/cfdr_sze_portal/alice_canvas.js"         "${build_folder}/"
else
cp "${source_folder}/web/index.html"                          "${build_folder}/"
cp "${source_folder}/web/alice_canvas.js"                     "${build_folder}/"
fi

rm -rf "${build_folder}/examples"
ln -sf "$(realpath examples)" "${build_folder}/examples"

# NOTE(cmat): Compile with walloc.c
# TODO(cmat): We want to remove this dependency as soon as possible.
source_files+=" ${source_folder}/thirdparty/walloc.c"

# ------------------------------------------------------------
# NOTE(cmat): Invoke compiler
pushd build > /dev/null 2>&1

echo "compiler flags: ${compiler_flags}"
$compiler_exec $source_files $define_flags $include_folders $compiler_flags $linker_flags

popd > /dev/null 2>&1

echo "build successful!"
