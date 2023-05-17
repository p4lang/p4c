How to create Ubuntu (or Debian) packages, to be buildable locally or
in a Ubuntu Launchpad PPA:

1. Check packaging.conf, set maintainer and GnuPG key ID.
To turn off signatures for local pbuilder build: SKIP_PACKAGE_SIGNING=1
Note: Launchpad PPA uploads only works with signed packages!

2. Call ./renew-debian-changelog to automatically update the package
version, according to Git commit and current date. This will update
debian/changelog and configure.ac.

3. Build Ubuntu/Debian packages:
- Create package: ./build-deb <distribution>
distribution: focal, groovy, hirsute, unstable, etc.
- Create source package and build using pbuilder:
./build-deb <distribution>
The result will be in /var/cache/pbuilder/result
- Create packages for focal, groovy and hirsute + upload to PPA:
./make-ppa
Note: dput has to be configured properly (in ~/.dput.cf)!


24.03.2020 Thomas Dreibholz <dreibh@simula.no>
