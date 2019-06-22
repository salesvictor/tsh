# tsh - Test Shell

## Running this code
### Using CMake
If you have CMake version 3.5 or higher installed, than you can install tsh running the following commands in tsh root directory:
```
mkdir -p build && cd build
cmake ..
sudo make install
```
### Using raw Makefile
Just run following commands:
```
cd src
make
./tsh
```

## Built-in commands

| Command | Description |
| --- | --- |
| `jobs` | Show all jobs executed by the shell. |
| `fg %N` | Sends job `N` (as listed by `jobs`) to the foreground
| `bg %N` | Sends job `N` (as listed by `jobs`) to the background
| `cd <path>` | Changes current working directory to `<path>`
| `exit` or `^D` | Exits the shell.
