import os
import re
import sys
import argparse
import difflib

def replace_namespaces_in_file(file_path, namespaces, dry_run=False):
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            content = file.read()

        original_content = content  # Keep a copy for comparison
        modified = False
        file_extension = os.path.splitext(file_path)[1].lower()

        # Replace specified namespaces only if not already prefixed with P4::
        for ns in namespaces:
            # Regex pattern to match 'namespace NS' not already prefixed with 'P4::'
            pattern = r'\bnamespace\s+(?!P4::)' + re.escape(ns) + r'\b'
            replacement = f'namespace P4::{ns}'
            new_content, count = re.subn(pattern, replacement, content)
            if count > 0:
                print(f"Replacing 'namespace {ns}' with 'namespace P4::{ns}' in {file_path}")
                content = new_content
                modified = True

        # Check if 'namespace P4::BFN' already exists
        bfn_namespace_pattern = r'\bnamespace\s+P4::BFN\b'
        bfn_namespace_exists = re.search(bfn_namespace_pattern, content)

        # Check if any of the target namespaces are present after replacement
        target_namespaces_present = any(
            re.search(r'\bnamespace\s+P4::' + re.escape(ns) + r'\b', content) for ns in namespaces
        )

        # If none of the target namespaces are present and 'namespace P4::BFN' doesn't exist, add it
        if not target_namespaces_present and not bfn_namespace_exists:
            print(f"No target namespaces found in {file_path}. Adding 'namespace P4::BFN'.")
            # Find all #include lines
            include_pattern = re.compile(r'^\s*#include\s+[\"<][^\">]+[\">]', re.MULTILINE)
            includes = list(include_pattern.finditer(content))
            if includes:
                last_include = includes[-1]
                insert_pos = last_include.end()
                # Insert namespace after the last include
                content = content[:insert_pos] + '\n\nnamespace P4::BFN {\n' + content[insert_pos:]
            else:
                # If no include lines, insert at the beginning
                content = 'namespace P4::BFN {\n' + content

            # Determine where to place the closing brace
            if file_extension in ['.h', '.hpp']:
                # Insert closing brace before the last #endif
                endif_pattern = re.compile(r'#endif\b.*$', re.MULTILINE)
                endif_matches = list(endif_pattern.finditer(content))
                if endif_matches:
                    last_endif = endif_matches[-1]
                    insert_pos = last_endif.start()
                    closing_namespace = '\n}  // namespace P4::BFN\n\n'
                    content = content[:insert_pos] + closing_namespace + content[insert_pos:]
                else:
                    # If no #endif found, append at the end
                    content += '\n}  // namespace P4::BFN\n'
            else:
                # For .cpp files, append at the end
                # Ensure the file ends with a newline
                if not content.endswith('\n'):
                    content += '\n'
                content += '\n}  // namespace P4::BFN\n'

            modified = True

        if modified:
            if dry_run:
                print(f"\n--- Changes that would be made to {file_path} ---")
                diff = difflib.unified_diff(
                    original_content.splitlines(),
                    content.splitlines(),
                    fromfile='original',
                    tofile='modified',
                    lineterm=''
                )
                for line in diff:
                    print(line)
                print(f"--- End of changes for {file_path} ---\n")
            else:
                with open(file_path, 'w', encoding='utf-8') as file:
                    file.write(content)
                print(f"Modified: {file_path}")
        else:
            if dry_run:
                print(f"No changes would be made to {file_path}.")
            else:
                print(f"No changes: {file_path}")

    except Exception as e:
        print(f"Error processing {file_path}: {e}")

def main():
    parser = argparse.ArgumentParser(description='Replace namespaces in .cpp and .h files and add namespace P4::BFN if needed.')
    parser.add_argument('directory', help='Directory to process')
    parser.add_argument('namespaces', nargs='+', help='List of namespaces to replace')
    parser.add_argument('--dry-run', action='store_true', help='Show changes without modifying files')

    args = parser.parse_args()

    directory = args.directory
    namespaces = args.namespaces
    dry_run = args.dry_run

    # Verify that the provided directory exists
    if not os.path.isdir(directory):
        print(f"Error: The directory '{directory}' does not exist.")
        sys.exit(1)

    # Iterate over all files in the directory and subdirectories
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h') or file.endswith('.hpp'):
                file_path = os.path.join(root, file)
                replace_namespaces_in_file(file_path, namespaces, dry_run=dry_run)

    print("\nReplacement complete.")

if __name__ == "__main__":
    main()
