import os
import re
import shutil
from pathlib import Path

def remove_useless_comments(file_path):
    """
    Removes useless comments like /* ========= <Class::Method> ========= */ from a C++ file
    and converts additional descriptions into Doxygen \brief comments.
    
    Args:
        file_path (str): Path to the C++ file.
    """
    # Read the file
    with open(file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    # Regex pattern for comments like /* ========= Class::Method ========= */ with optional description
    pattern = r'/\*\s*=+\s*([\w:]+)\s*=+\s*(.*?)\s*\*/\s*\n\s*([\w\s\*&:<>]+)\s+\1\s*\([^)]*\)\s*(?:const)?\s*(?:override)?\s*(?:{|\;)'
    
    def replacement(match):
        class_method = match.group(1)  # Class::Method
        description = match.group(2).strip()  # Additional description
        signature = match.group(3)  # Function signature before Class::Method
        
        # If a description is present, convert it to a Doxygen \brief comment
        if description:
            # Doxygen comment with indentation (4 spaces per IndentWidth)
            doxygen_comment = f'/** \\brief {description}\n */\n    '
            return f'{doxygen_comment}{signature} {class_method}('
        else:
            # Without description, just remove the comment
            return f'{signature} {class_method}('
    
    # Search and replace the comments
    cleaned_content, count = re.subn(pattern, replacement, content, flags=re.MULTILINE)
    
    # If changes were made, overwrite the file
    if count > 0:
        # Create a backup copy
        #backup_path = file_path + '.bak'
        #shutil.copy(file_path, backup_path)
        #print(f"Backup created: {backup_path}")
        
        # Write the cleaned content back
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