# BEHAVIORAL MODEL REPOSITORY

## Building the code:
1. sudo apt-get install autotools-dev dh-autoreconf
2. ./autogen.sh
3. ./configure
4. make
5. make check

## If you update any of the source files (.cc, .thrift, .h, or .c)
1. make; make check

## If you add a new source file (source or header).
1. Find nearest enclosing Makefile.am and add source here.
2. make; make check

## If you add a new folder
1. Create a new Makefile.am for that folder.
2. Update the SUBDIRS variable in the parent folder's Makefile.am.
3. Update AC_CONFIG_FILES in configure.ac to generate the Makefile for this folder.
4. make; make check

## If you add a new external library dependency
1. Use either AC_CHECK_LIB or AC_CHECK_HEADER in configure.ac
2. make; make check

# Autotools and dependencies:
1. You don't need to clean your repository before building.
2. This is because automake generates rebuild rules
(http://www.gnu.org/software/automake/manual/html_node/Rebuilding.html) that
automatically track dependencies of Makefiles on Makefile.am and configure.ac
3. That said, if you are paranoid:
  *  Move all untracked files that you care about outside your repository.
  *  Run git clean -xfd (WARNING!!!: This deletes everything not in version control).
  *  ./autogen.sh; ./configure; make; make check
4. I have run several spot checks on the build system and it seems to do the
right thing. If you seem to be doing git clean -xfd too often, the build system
isn't working (because the rebuild rules should be taking care of this for
you). If so, I would like to know (anirudh@barefootnetworks.com)
