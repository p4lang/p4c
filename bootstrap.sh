set -e
./create-makefile.am.sh
echo "Running autoconf/configure tools"
libtoolize
aclocal 
autoconf
autoheader
automake --add-missing
mkdir -p build
cd build
# TODO: the "prefix" is needed for finding p4include.
# It should be an absolute path
../configure CXXFLAGS="-g -O0" --prefix=`pwd`/.. CXX="clang++"
ln -sf ../.gdbinit .

echo "### Configured for building in 'build' folder"
