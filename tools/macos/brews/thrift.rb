class Thrift < Formula
  desc "Framework for scalable cross-language services development"
  homepage "https://thrift.apache.org/"

  stable do
    url "https://www.apache.org/dyn/closer.cgi?path=/thrift/0.9.3/thrift-0.9.3.tar.gz"
    sha256 "b0740a070ac09adde04d43e852ce4c320564a292f26521c46b78e0641564969e"

    # Apply any necessary patches (none currently required)
    [
      # Example patch:
      #
      # Apply THRIFT-2201 fix from master to 0.9.1 branch (required for clang to compile with C++11 support)
      # %w{836d95f9f00be73c6936d407977796181d1a506c f8e14cbae1810ade4eab49d91e351459b055c81dba144c1ca3c5d6f4fe440925},
    ].each do |name, sha|
      patch do
        url "https://git-wip-us.apache.org/repos/asf?p=thrift.git;a=commitdiff_plain;h=#{name}"
        sha256 sha
      end
    end
  end

  head do
    url "https://git-wip-us.apache.org/repos/asf/thrift.git", :branch => "0.9.3"

    # patch to install python library
    patch :DATA

    depends_on "autoconf" => :build
    depends_on "automake" => :build
    depends_on "libtool" => :build
    depends_on "pkg-config" => :build
  end

  option "with-perl", "Install Perl binding"
  option "with-python", "Install Python binding"
  option "with-libevent", "Install nonblocking server libraries"

  depends_on "bison" => :build
  depends_on "boost"
  depends_on "openssl"
  depends_on "libevent" => :optional
  depends_on python: :optional

  def install
    system "./bootstrap.sh" unless build.stable?

    exclusions = ["--without-c_glib", "--without-java", "--without-ruby",
                  "--without-erlang", "--without-go", "--without-haskel",
                  "--without-nodejs", "--without-php", "--without-php_extension",
                  "--disable-tests"]
    exclusions << "--without-python" if build.without? "python"
    exclusions << "--without-perl" if build.without? "perl"

    ENV.cxx11 if MacOS.version >= :mavericks && ENV.compiler == :clang

    # Don't install extensions to /usr:
    ENV["PY_PREFIX"] = prefix
    ENV["PHP_PREFIX"] = prefix

    system "./configure", "--disable-debug",
                          "--prefix=#{prefix}",
                          "--libdir=#{lib}",
                          "--with-cpp=yes",
                          "--with-openssl=#{HOMEBREW_PREFIX}/opt/openssl",
                          *exclusions
    ENV.j1
    system "make"
    system "make", "install"
  end

end

__END__
diff --git a/configure.ac b/configure.ac
index 18e3233..7d760a5 100755
--- a/configure.ac
+++ b/configure.ac
@@ -144,7 +144,10 @@ if test "$with_cpp" = "yes";  then
     have_cpp="yes"
   fi

-  AX_CHECK_OPENSSL()
+  AX_CHECK_OPENSSL([CPPFLAGS="$OPENSSL_INCLUDES $CPPFLAGS"
+                    LIBS="$OPENSSL_LIBRARIES $LIBS"
+                    LDFLAGS="$LDFLAGS $OPENSSL_LDFLAGS"],
+                   [ AC_MSG_ERROR("need openssl for cpp library")])

   AX_LIB_EVENT([1.0])
   have_libevent=$success
@@ -276,9 +279,10 @@ AM_CONDITIONAL(WITH_LUA, [test "$have_lua" = "yes"])

 AX_THRIFT_LIB(python, [Python], yes)
 if test "$with_python" = "yes";  then
-  AC_PATH_PROG([TRIAL], [trial])
+  dnl AC_PATH_PROG([TRIAL], [trial])
   AM_PATH_PYTHON(2.4,, :)
-  if test -n "$TRIAL" && test "x$PYTHON" != "x" && test "x$PYTHON" != "x:" ; then
+  dnl if test -n "$TRIAL" && test "x$PYTHON" != "x" && test "x$PYTHON" != "x:" ; then
+  if test "x$PYTHON" != "x" ; then
     have_python="yes"
   fi
 fi
