# -I "include"

# -L "libraries"

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -static-libgcc TODO(chris) not sure what this one does specifically
# -static-libstdc++ bring namespace std into the build

#-o specifies the name of our exectuable

windows:
	g++ main.cpp -IC:/mingw_dev_libs/include/SDL2 -LC:/mingw_dev_libs/lib -w -static-libgcc -static-libstdc++ -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o breakout

mac:
	g++ main.cpp -Iinclude/SDL2 -Llib -w -static-libstdc++ -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o breakout