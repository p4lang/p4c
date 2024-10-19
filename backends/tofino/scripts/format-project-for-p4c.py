import os
import shutil
import sys


def copy_directories_and_files(source, dest):
    # Check if the source directory exists
    if not os.path.exists(source):
        print(f"Source directory '{source}' does not exist.")
        return

    # Create the destination directory if it does not exist
    if not os.path.exists(dest):
        os.makedirs(dest)
    else:
        # Remove existing files in the destination directory
        for item in os.listdir(dest):
            item_path = os.path.join(dest, item)
            if os.path.isfile(item_path):
                os.remove(item_path)
            elif os.path.isdir(item_path):
                shutil.rmtree(item_path)

    # Directories to copy
    directories_to_copy = ['bf-p4c', 'cmake', 'compiler_interfaces']

    # Files to copy
    files_to_copy = [
        'CMakeLists.txt',
        'LICENSE',
        'README',
        'bootstrap_bfn_env.sh',
        'CPackOptions.cmake',
        'LICENSE.in',
        'README.md',
        'bootstrap_bfn_compilers.sh',
        'bootstrap_ptf.sh',
        'doc.Jenkinsfile',
        'ubuntu20.04.setup',
        'Doxyfile.in',
        'LICENSES.txt',
        'git_sha_version.h.in',
    ]

    # Iterate over the specified directories in the source
    for item in directories_to_copy:
        source_item = os.path.join(source, item)
        dest_item = os.path.join(dest, item)

        # Copy directories if they exist in the source
        if os.path.exists(source_item) and os.path.isdir(source_item):
            shutil.copytree(source_item, dest_item, dirs_exist_ok=True)
        else:
            print(f"Directory '{item}' does not exist in the source.")

    # Iterate over the specified files in the source
    for item in files_to_copy:
        source_item = os.path.join(source, item)
        dest_item = os.path.join(dest, item)

        # Copy files if they exist in the source
        if os.path.exists(source_item) and os.path.isfile(source_item):
            shutil.copy2(source_item, dest_item)
        else:
            print(f"File '{item}' does not exist in the source.")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <source_directory> <destination_directory>")
        sys.exit(1)

    source = sys.argv[1]
    dest = os.path.join(sys.argv[2], 'backends/tofino2')

    copy_directories_and_files(source, dest)
    print("Copying completed.")
