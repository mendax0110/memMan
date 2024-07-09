
## memMan
memMan is a simple tool to read and write memory of a process. It is written in C++ and uses the Windows API to access the memory of a process. The tool is useful for debugging and reverse engineering purposes.

### Usage
To use memMan, follow these steps:

1. Clone the repository
```bash	
git clone https://github.com/mendax0110/memMan.git
```
2. Navigate to the loggy directory
```bash
cd memMan
```
3. Create a build directory
```bash
mkdir build
```
4. Navigate to the build directory
```bash
cd build
```
5. Generat the build files
```bash
cmake ..
```
6. Build the project
```bash
cmake --build .
```
7. List the memory regions of a process
```bash
memMan.exe <process_name> -l
```
8. Read memory of a process
```bash
memMan.exe <process_name> <address> -r
```
9. Write memory of a process
```bash
memMan.exe <process_name> <address> -w <value>
```
10. Auto inject integer into memory region
```bash
memMan.exe <process_name> <address> -a <value>
```

## Supported Platforms
- Windows