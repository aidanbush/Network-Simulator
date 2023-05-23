# simulator
Requirements:
https://github.com/nlohmann/json
https://www.boost.org/
libtorch

## compiling
create build directory with cmake
in build call make in build
`cmake -DCMAKE_PREFIX_PATH=pathto-libtorch -S src/ -B build/`
