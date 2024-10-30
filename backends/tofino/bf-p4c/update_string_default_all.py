import argparse
import difflib
import os
import sys


def process_file(filename, dry_run=False):
    with open(filename, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    modified = False
    new_lines = []

    for line_num, line in enumerate(lines):
        # Check if the line contains any of the specified substrings
        if any(substr in line for substr in ('%', 'LOG', 'std::string', 'BUG_CHECK')):
            new_lines.append(line)
            continue  # Skip modification for this line

        # Initialize variables for processing the line
        output = []
        state = 'code'
        index = 0
        length = len(line)
        at_line_start = True
        context_stack = []
        paren_depth = 0

        while index < length:
            c = line[index]

            if state == 'code':
                if at_line_start and c.strip() == '':
                    output.append(c)
                    index += 1
                    continue
                elif at_line_start and c == '#':
                    state = 'preprocessor'
                    at_line_start = False
                    output.append(c)
                elif c == '/' and index + 1 < length:
                    next_char = line[index + 1]
                    if next_char == '/':
                        state = 'single_line_comment'
                        output.append(c)
                        index += 1
                        output.append(line[index])
                    elif next_char == '*':
                        state = 'multi_line_comment'
                        output.append(c)
                        index += 1
                        output.append(line[index])
                    else:
                        at_line_start = False
                        output.append(c)
                elif c == '"':
                    state = 'string_literal'
                    string_literal = c
                    at_line_start = False
                else:
                    output.append(c)
                    at_line_start = c == '\n'
            elif state == 'preprocessor':
                output.append(c)
                if c == '\n':
                    state = 'code'
                    at_line_start = True
                else:
                    at_line_start = False
            elif state == 'single_line_comment':
                output.append(c)
                if c == '\n':
                    state = 'code'
                    at_line_start = True
                else:
                    at_line_start = False
            elif state == 'multi_line_comment':
                output.append(c)
                if c == '*' and index + 1 < length and line[index + 1] == '/':
                    output.append(line[index + 1])
                    index += 1
                    state = 'code'
                at_line_start = c == '\n'
            elif state == 'string_literal':
                string_literal += c
                if c == '\\':
                    # Skip escaped characters inside string literal
                    if index + 1 < length:
                        index += 1
                        c = line[index]
                        string_literal += c
                elif c == '"':
                    # End of string literal
                    index += 1
                    # Check for user-defined literal suffix
                    has_suffix = False
                    if index < length and line[index] == '_':
                        suffix = ''
                        suffix_start = index
                        while index < length and (line[index].isalnum() or line[index] == '_'):
                            suffix += line[index]
                            index += 1
                        string_literal += line[suffix_start:index]
                        has_suffix = True

                    # Decide whether to append '_cs'
                    if not has_suffix:
                        # Append '_cs' to the string literal
                        output.append(string_literal)
                        output.append('_cs')
                        modified = True
                    else:
                        # Already has a suffix; do not modify
                        output.append(string_literal)

                    state = 'code'
                    continue  # Skip incrementing index at the end of loop
                at_line_start = False
            index += 1

        new_lines.append(''.join(output))

    if modified:
        if dry_run:
            print(f"Would modify: {filename}")
            original_lines = ''.join(lines).splitlines()
            modified_lines = ''.join(new_lines).splitlines()
            diff = difflib.unified_diff(
                original_lines,
                modified_lines,
                fromfile=f'{filename} (original)',
                tofile=f'{filename} (modified)',
                lineterm='',
            )
            print('\n'.join(diff))
        else:
            with open(filename, 'w', encoding='utf-8') as f:
                f.writelines(new_lines)
            print(f"Modified: {filename}")
    else:
        print(f"No changes in: {filename}")


def process_directory(directory, dry_run=False):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.cpp'):
                filepath = os.path.join(root, file)
                process_file(filepath, dry_run=dry_run)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Update string literals in .cpp files.')
    parser.add_argument('directory', nargs='?', default='.', help='Directory to process')
    parser.add_argument(
        '-n', '--dry-run', action='store_true', help='Dry run without modifying files'
    )

    args = parser.parse_args()

    process_directory(args.directory, dry_run=args.dry_run)
