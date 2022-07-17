# ElimSSAT

## Compile & Run

- Compile `unique`(dependencies of `manthan`)
    - `cd unique`
    - `mkdir build`
    - `cd build`
    - `cmake ..`
    - `make`
    - `cd ../..`
    - `cp unique/build/interpolatingsolver/src/itp.cpython-36m-x86_64-linux-gnu.so manthan/itp.so`
- Compile the code by typing `make` in the console.
- Unzip the benchmarks by `unzip benchmarks.zip`
- Run the solver by `./abc -q "ssat" <sdimacs-file>`
