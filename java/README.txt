This directory contains the Java Native Interface (JNI) code for FastBit
functions and a simple test program milky.java.


On a unix machine, you need autoconf, automake and java in order to use
the Java Native Interface (JNI) code in FastBit.c and FastBit.h.  If you
are using gcc, it is likely that the commands './configure && make'
would have build the correct library (libfastbitjni.so) for you already.
Before executing 'make install', the file libfastbitjni.so should be
located in .libs within this directory.  In order for the java run-time
environment to find it, you need to include the directory containing the
file in LD_LIBRARY_PATH.

If autoconf scripts didnot produce the right makefiles for you, you need
a variations of the following command line to build the .so file.

gcc -O -shared -o libfastbitjni.so -Wl,-soname,libfastbitjni.so \
     -I/export/home/jdk1.2/include \
     -I/export/home/jdk1.2/include/linux FastBit.c  \
     ..\src\libfastbit.lo -lm

On a windows machine, use ..\win\java.vcproj to build a DLL library for
the JNI interface.  The DLL file (named fastbitjni.dll) will end up in
either ..\win\dll\Debug or ..\win\dll\Release depending how you build
it.  The directory containg the DLL file needs to be in environment
variable PATH in order for the java run-time system to find it.

To test JNI interface, an example file (named milky.java) is provided.
To compile it, type

javac FastBit.java milky.java

Two class files are generated from FastBit.java, FastBit.class and
FastBit$QueryHandle.class.  These two class files need to be in a
directory named gov/lbl/fastbit in the current working directory or some
directory named in the environment variable named CLASSPATH.

To run

java milky data-dir "query conditions"

for example,

java milky ../tests/tmp/t1 "a>10 and c between 50 and 70"

The above mentioned data directory is created by running "make check" in
the top level directory of FastBit source code distribution.  The
expected output is

applying "a>10 and c between 50 and 70" on data in ../tests/tmp/t1 produced 42 hits


----

About the name: milky.java is named after Milky Stork (Mycteria
cinerea), an endangered white stork that was common on Java Island,
Indonesia.  More information about the bird can be found at
http://www.birdlife.org/datazone/sites/index.html?action=SpcHTMDetails.asp&sid=3825
A few other example programs in FastBit are also named after various
storks, such as ibis, ardea and thula.
