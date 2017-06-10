rm extra -r
vita-libs-gen -c extra.yml extra
cd extra
cmake .
make
cd ..

cd build
make clean
rm CMakeCache.txt
cmake ../
make
make install
cd ..
