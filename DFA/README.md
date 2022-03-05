# DFA - Deterministic Finite Automata Simulator

## Build
```bash
mkdir build
cd build
cmake ../
make
```


## Usage after installation
Type `./dfa -h` or `./dfa --help` to find the cli usage.
### Basic usage
```bash
./dfa ../test_data/repeating_A_s.dfa -s -r
a,b,a,b,a,a,b,b
```
### Basic usage with only files
```bash
echo a,b,a,b,a,a,b,b > input.dat
./dfa ../test_data/repeating_A_s.dfa -Iinput.dat -Ooutput.dat -s -r
cat output.dat
```
