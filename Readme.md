sudo pacman -S vulkan-headers

cmake -DCMAKE_BUILD_TYPE=Debug ..

gdb -q ./model_viewer -ex run -ex bt -ex quit

gdb -q ./model_viewer -ex run -ex 'bt full' -ex quit