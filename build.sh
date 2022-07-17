# build Cadet
mkdir -p bin
echo "> Build Cadet..."
git clone https://github.com/MarkusRabe/cadet.git
cd cadet
./configure && make
cd ..
cp cadet/cadet bin/
echo "> Build Manthan..."
git clone https://github.com/meelgroup/manthan.git
cd manthan
git apply ../manthan.patch
cd ..
echo "> Build Unique..."
git clone https://github.com/fslivovsky/unique.git
cd unique
git submodule init
git submodule update
mkdir build
cd build
cmake .. && make -j8
cd ../..
# move essential libraries
cp unique/build/avy/src/libClauseItpSeq.a lib
cp unique/build/avy/src/libAvyDebug.so lib
cp $(find unique/build/interpolatingsolver/src/itp.cpython*) manthan/itp.so
echo "> Build ABC..."
make -j8
