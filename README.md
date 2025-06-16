# JSON-like FS
A simple FUSE file system that allows you to mount a JSON-like structure as a file system. The structure can be modified through the file system interface.
## Features
- Mount a JSON structure made up solely of objects (directories) and strings (files) as a file system
- Modify the structure as you please using the file system interface
- Print it back in JSON format using `getfattr`
## Requirements
- CMake
- FUSE 3
- cJSON
- GLib
## Build
This project has only been tested so far in Fedora 42, its default package manager `dnf` provides all the necessary dependencies. It should also work on other Linux distributions and using virtual machines or containers with the necessary dependencies installed.
 
After cloning the repository and installing the dependencies, you can build the project using CMake. The following commands will create a build directory, configure the project, and compile it:
```bash
(mkdir build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && cmake --build .)
```
## Usage
 
```bash
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ mkdir testdir
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ build/main --structure='{"hella":"Hello World!\n","foo": {"bar": "baz\n"}}' testdir/
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ getfattr -e text -n user.structure testdir/ | grep -o '".*"' | jq -r | jq .
{
  "hella": "Hello World!\n",
  "foo": {
    "bar": "baz\n"
  }
}
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ mkdir testdir/new
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ cp -r testdir/foo/ testdir/new/
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ getfattr -e text -n user.structure testdir/ | grep -o '".*"' | jq -r | jq .
{
  "hella": "Hello World!\n",
  "foo": {
    "bar": "baz\n"
  },
  "new": {
    "foo": {
      "bar": "baz\n"
    }
  }
}
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ fusermount -u testdir
carlos@fedora:~/Documents/cs_primer/cs_primer/operating_systems/file_systems/custom_file_system$ 
```


