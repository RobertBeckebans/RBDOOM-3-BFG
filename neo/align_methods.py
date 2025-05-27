import re
import sys
import os

def align_method_names(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"ERROR: File not found: {file_path}")
        sys.exit(1)
    except Exception as e:
        print(f"ERROR: Failed to read file {file_path}: {e}")
        sys.exit(1)

    max_prefix_length = 0
    modified_lines = []
    in_enum = False

    # First pass: Identify lines to align and compute max prefix length
    for line in lines:
        stripped_line = line.strip()
        # Detect start of enum
        if re.match(r'\s*typedef\s+enum\s*{', stripped_line):
            in_enum = True
        # Detect end of enum
        elif in_enum and re.match(r'\s*}\s*\w+;', stripped_line):
            in_enum = False
        # Match enum declarations
        enum_match = re.match(r'(\s*)(\w+)\s*(=.*)', stripped_line) if in_enum else None
        # Match class method declarations
        method_match = re.match(r'(\s*(?:virtual\s+)?(?:[\w*\s*]*\s*))(\w+\([^)]*\)\s*.*)', stripped_line)
        # Match global variables
        global_match = re.match(r'(\s*extern\s+[\w*\s*]*\s*)(\w+\s*;.*)', stripped_line)
        
        if enum_match:
            prefix = enum_match.group(1) + enum_match.group(2)
            max_prefix_length = max(max_prefix_length, len(prefix))
            modified_lines.append((prefix, enum_match.group(3), line))
        elif method_match:
            prefix = method_match.group(1)
            max_prefix_length = max(max_prefix_length, len(prefix))
            modified_lines.append((prefix, method_match.group(2), line))
        elif global_match:
            prefix = global_match.group(1)
            max_prefix_length = max(max_prefix_length, len(prefix))
            modified_lines.append((prefix, global_match.group(2), line))
        else:
            modified_lines.append((None, None, line))

    # Second pass: Write the file with aligned lines
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            for prefix, suffix, line in modified_lines:
                if prefix and suffix:
                    # Add spaces to align names right
                    padded_prefix = prefix + ' ' * (max_prefix_length - len(prefix))
                    f.write(padded_prefix + suffix + '\n')
                else:
                    f.write(line)
    except Exception as e:
        print(f"ERROR: Failed to write file {file_path}: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("ERROR: Exactly one file path must be provided as an argument")
        print(f"Usage: {sys.argv[0]} <file_path>")
        sys.exit(1)
    # Normalize file path for Windows compatibility
    file_path = os.path.normpath(sys.argv[1])
    align_method_names(file_path)