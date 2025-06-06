import os
import re
from pathlib import Path

def remove_useless_comments(file_path):
    """
    Removes useless comments like /* ========= <Class::Method> ========= */ from a C++ file
    and converts additional descriptions into Doxygen \brief comments.
    Handles constructors, destructors, operators, and multi-line descriptions.
    
    Args:
        file_path (str): Path to the C++ file.
    """
    # Try reading the file with different encodings
    content = None
    for encoding in ['utf-8', 'latin1', 'windows-1252']:
        try:
            with open(file_path, 'r', encoding=encoding) as file:
                content = file.read()
            print(f"Read {file_path} with {encoding} encoding")
            break
        except UnicodeDecodeError:
            print(f"Failed to read {file_path} with {encoding} encoding")
            continue
    
    if content is None:
        print(f"Skipping {file_path}: Unable to decode with any supported encoding")
        return

    # Regex pattern for comments like /* ========= Class::Method ========= */
    # Handles constructors, destructors (~), operators, and multi-line descriptions
    pattern = r'/\*\s*=+\s*([\w:]+(?:::[\w~]+|::operator[^\s\(\)]+))\s*=+\s*([\s\S]*?)\s*\*/\s*\n\s*((?:[\w\s\*&:<>]+\s+)?\1\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?(?:{|\;))'
    
    def replacement(match):
        class_method = match.group(1)  # Class::Method, Class::~Method, or Class::operatorX
        description = match.group(2).strip()  # Additional description (multi-line)
        full_signature = match.group(3)  # Full function signature including ()
        
        # If a description is present, convert it to a Doxygen \brief comment
        if description:
            # Doxygen comment with indentation (4 spaces per IndentWidth)
            doxygen_comment = f'/** \\brief {description}\n */\n    '
            return f'{doxygen_comment}{full_signature}'
        else:
            # Without description, just remove the comment
            return f'{full_signature}'
    
    # Search and replace the comments
    cleaned_content, count = re.subn(pattern, replacement, content, flags=re.MULTILINE)
    
    # If changes were made, overwrite the file
    if count > 0:
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(cleaned_content)
        print(f"Processed {count} comments in {file_path}")
    else:
        print(f"No matching comments found in {file_path}")

def process_directory():
    """
    Processes all .cpp and .h files in the current directory recursively, skipping 'extern', 'libs', and 'thirdparty' directories.
    """
    # Current directory of the script
    target_directory = os.path.dirname(os.path.abspath(__file__))
    print(f"Processing directory: {target_directory}")
    
    # Directories to skip
    skip_dirs = {'extern', 'libs', 'thirdparty'}
    
    for root, dirs, files in os.walk(target_directory):
        # Skip specified directories
        if os.path.basename(root) in skip_dirs:
            print(f"Skipping directory: {root}")
            dirs[:] = []  # Prevent descending into skipped directories
            continue
        
        for file in files:
            if file.endswith(('.cpp', '.h')):
                file_path = os.path.join(root, file)
                remove_useless_comments(file_path)

if __name__ == "__main__":
    process_directory()