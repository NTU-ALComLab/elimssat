# build Cadet
mkdir -p bin
echo "> Build Cadet..."
git clone https://github.com/MarkusRabe/cadet.git
cd cadet
./configure && make
cd ..
cp cadet/cadet bin/

# build Manthan
echo "> Build Manthan..."
git clone https://github.com/meelgroup/manthan.git
cd manthan
# install dependencies
pip3 install -r requirement.txt
# apply custom changes to Manthan
git apply ../manthan.patch
cd ..

# build Unique
echo "> Build Unique..."
git clone https://github.com/fslivovsky/unique.git
cd unique
git submodule init
git submodule update
mkdir build
cd build
cmake .. && make -j8
cd ../..
# move essential libraries for linking purposes
cp unique/build/avy/src/libClauseItpSeq.a lib
cp unique/build/avy/src/libAvyDebug.so lib
cp $(find unique/build/interpolatingsolver/src/itp.cpython*) manthan/itp.so

# build the ABC system and the solver
echo "> Build ABC..."
make -j8
