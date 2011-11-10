path_decomposed_tries
=====================

This library implements the data structures described in the paper
"Fast Compressed Tries using Path Decomposition".

Both the unit tests and the perftest contain good examples of how to
use the structures. More examples will follow.

How to build the code
---------------------

### Dependencies ###

The following dependencies have to be installed to compile the library.

* CMake >= 2.6, for the build system
* Boost >= 1.42

Also, the library `succinct` has to be downloaded as a git submodule,
so the following two commands have to be executed *before running
cmake*:

    $ git submodule init
    $ git submodule update

### Supported systems ###

The code has been developed and tested mainly on Windows 7 and Windows
Server 2008 R2, but it has been tested also on Mac OS X and Linux.

The code has been tested only on x86-64. Compiling it on 32bit
architectures would probably require some work.

### Building on Unix ###

The project uses CMake. To build it on Unix systems it should be
sufficient to do the following:

    $ cmake . -DCMAKE_BUILD_TYPE=Release
    $ make

It is also advised to perform a `make test`, which runs the unit tests.

### Building on Windows ###

On Windows, Boost is not installed in default locations, so it is
necessary to set some environment variables to allow the build system
to find them.

* `BOOST_ROOT` must be set to the directory which contains the `boost`
  include directory.
* The directory that contains Boost DLLs must be
  added to `PATH` so that the executables find them

Once the env variables are set, the quickest way to build the code is
by using NMake (instead of the default Visual Studio). Run the
following commands in a Visual Studio x64 Command Prompt:

    $ cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release .
    $ nmake
    $ nmake test


