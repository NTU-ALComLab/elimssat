# ElimSSAT

The prototype Stochastic Boolean satisfiability solver based on quantifier elimination.

## Dependencies

- [Cadet](https://github.com/MarkusRabe/cadet.git)
- [Manthan](https://github.com/meelgroup/manthan.git)
- [Unique](https://github.com/fslivovsky/unique.git)

## Prerequisite

- [pybind11](https://github.com/pybind/pybind11)
    * For building the python binding of `Unique` for `Manthan`.
- Boost Library
    - Require for `Unique`.

## Compile

- We give a script for installing and compiling dependencies. Use the following command.
```
./build.sh
```

## Usage

- The solver is built in the [abc](https://github.com/berkeley-abc/abc.git) system as a command.
To execute the solver, use the following command.
```
./abc -q "ssat" <sdimacs>
```

## Benchmarks

- We provide three toy cases for testing purpose. You can found them in `benchmark` folder.

- The full benchmarks can be found in the repository of [ClauSSAT](git@github.com:NTU-ALComLab/ClauSSat.git).

## Contact

If you have any problems or suggestions, feel free to create an issue or contact us through r09943108@ntu.edu.tw.
