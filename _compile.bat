windres resource.rc -O coff -o resource.res

g++ Oil.cpp resource.res -o Oil.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows
g++ A.cpp -o A.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows
g++ B.cpp -o B.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows
g++ C.cpp -o C.exe -O3 -s -DNDEBUG -static -static-libgcc -static-libstdc++ -mwindows
