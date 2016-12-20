[TODO: this file is probably out-of-date]

# Setting up Eclipse

Use the following installation steps to setup an Eclipse development environment for p4c.
Note that the following assumes you have already performed the preliminary environment
bootstrap, as described in [README](README.md).

1. Download Eclipse Luna for C/C++ from [Eclipse webpage](http://www.eclipse.org/luna/)
  * Go to Downloads -> Eclipse IDS for C/C++ Developers -> Linux 64-bit

2. `tar xvfz eclipse-cpp-luna-SR2-linux-gtk-x86_64.tar.gz`

3. Move `eclipse` folder to desired location.

4. Open Eclipse

5. Create workspace

6. Go to Help -> Eclipse Marketplace...
  * Search for c++
  * Check that Eclipse CDT (C/C++ Development Tooling) is installed (it was for me)
  * If not, install it.

7. Create a new C++ project from File -> New -> Project
  * Select C/C++ -> Makefile Project with Existing Code, click Next
  * For the Project Name enter something like `p4c`
  * Navigate to the directory that contains the `p4c` directory, something like `/home/mike/p4c`
  * Check the C and C++ boxes
  * Select `GNU Autotools Toolchain` as the Toolchain for Indexer Settings

8. Setup C++11 support
  * Right click your project and click Properties
  * Under C/C++ General, click `Preprocessor Include Paths, Macros etc.`
  * Select the `Providers` tab
  * Highlight the `CDT GCC Built-in Compiler Settings`
  * Uncheck `Use global provider shared between projects` box
  * In the box labeled `Command to get compiler specs`, add `-std=c++11`
  * Move `CDT GCC Built-in Compiler Settings` to the top by repeatedly clicking `Move Up`
  * Click `Apply` and then `OK`

9. Add C++11 support to `Window`
  * Go to Window -> Preferences
  * Go to C/C++ -> Build -> Settings
  * Click the `Discovery` tab
  * Highlight `CDT GCC Built-in Compiler Settings [Shared]`
  * In the box labeled `Command to get compiler specs`, add `-std=c++11`
  * Click `Apply`

9.5. Change the build location
  * Project -> Properties
  * Select C/C++ build
  * In the build location choose "FileSystem" and browse to the p4c/build folder.

10. Add additional include files needed by your sub-project to include path
  * Right click your project and click Properties
  * Highlight C/C++ General -> Paths and Symbols
  * In the `Includes` tab, highlight `GNU C++`
  * Click `Add` and then `File system...` and navigate to header files that you need to include
  * Click `OK`

11. Right click project and go to `Index`
  * Select `Re-resolve Unresolved Includes`

12. Right click project and go to `Index`
  * Select `Rebuild`
  * Wait for indexer to complete

13. Close Eclipse and then open it again.

14. Setup build
  * This seemed to "just work" for me.  I clicked build, and it ran make.

15. Setup Run
  * Under `Run Configurations` I was able to navigate to this file and add command arguments.

16. Setup editor
   * Window/Preferences/Text editor -> insert spaces for tabs
