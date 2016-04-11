./find-makefiles.sh
mkdir -p extensions
echo "Running autoconf/configure tools"
autoreconf -i
mkdir -p build
cd build
# TODO: the "prefix" is needed for finding the p4include folder.
# It should be an absolute path.  This may need to change
# when we have a proper installation procedure.
../configure CXXFLAGS="-g -O0" --prefix=`pwd`/..
echo "### Configured for building in 'build' folder"
