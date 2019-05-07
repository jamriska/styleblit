em++ main.cpp styleblit/styleblit.cpp -I"." -I"styleblit" -s WASM=0 -s TOTAL_MEMORY=33554432 -s USE_GLFW=3 -std=c++0x -DNDEBUG -O3 --preload-file styleblit --preload-file data -o styleblit.html
