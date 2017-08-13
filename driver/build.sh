rm extra -r
mkdir extra
vita-libs-gen -c extra.yml extra
cd extra
cmake .
make
cd ..

rm build -r
mkdir build
cd build
cmake ../
make
make install
cd ..
