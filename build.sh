#!/bin/bash
# $File: build.sh
# $Last-Modified: "2025-06-10 16:23:09"
# $Author: Matyas Constans.
# $Notice: (C) Matyas Constans 2024 - All Rights Reserved.
# $License: You may use, distribute and modify this code under the terms of the MIT license.
# $Note: Project build file.

# NOTE(cmat): Bake shaders.
pushd src/core/core_shader > /dev/null
./core_shader_bake_metal.sh
popd > /dev/null

# NOTE(cmat): Make build folder, push it as active directory.
mkdir -p build
pushd build > /dev/null
code_path=$(realpath "../src/")

# NOTE(cmat): Compiler arguments.
compiler="
-O0 -g -x objective-c -fsanitize=address -fno-omit-frame-pointer"

# NOTE(cmat): Linker arguments.
libraries="
    -framework Foundation
    -framework AppKit
    -framework QuartzCore
    -framework Metal
    -o test"

# NOTE(cmat): Build flags.
defines="
-DBuild_Flag_Debug=1
-DBuild_Target_Graphical=1
"

# NOTE(cmat): Translation units.
source="
${code_path}/test.c
"
# NOTE(cmat): Compile & Link
clang $compiler $defines $source $libraries

popd > /dev/null


