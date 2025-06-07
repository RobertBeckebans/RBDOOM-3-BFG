import os
import re
from pathlib import Path
from termcolor import colored

def remove_useless_comments(file_path):
    """
    Removes comments like /* ========= <Class::Method> ========= */ from a C++ file
    and converts descriptions to Doxygen \brief comments.
    Handles constructors, destructors, operators, and multi-line descriptions.
    
    Args:
        file_path (str): Path to the C++ source or header file.
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
    
    print(colored(f"\nProcessing {file_path} ...", "cyan"))

    # Main pattern for comments and signatures (human-readable)
    pattern = r'''(?xms)  # Verbose mode, multi-line, dot-all
        /\* \s* =+ \s*                    # Opening comment: /* === */
        (?P<method> [\w:]+                # Class name and ::, followed by:
            (?:
                ::[\w~]+              |   # Method or destructor (e.g., Class::Method, Class::~Method)
                ::operator[^\s\(\)]*  |   # Operator (e.g., Class::operator+)
                ::[\w]+                   # Other method names
            )
        )
        \s* =+ \s*                        # Second === line
        (?P<desc> (?:(?!=+\s*\*/).|\s)*?) # Description, excluding closing === (non-greedy)
        (?:\s* =+ \s*)? \*/               # Optional closing === and comment end
        \s* \r?\n                         # Newline(s) after comment
        (?P<sig>                          # Function signature
            (?:
                [\w\s*\*&:<>,()\[\]]*     # Return type, including pointers and templates
                (?:
                    \s*\* |               # Pointer (e.g., char*)
                    \s*\(\s*\*\s*[^\)]*\)\s*\([^\)]*\)  # Function pointer (e.g., void(*)(char*))
                )?
                \s+
            )?                            # Return type is optional
            (?P=method)                   # Match method name again
            \s* \([^\)]*\)                # Parameter list
            \s* (?:const\s*)?             # Optional const
            (?:override\s*)?              # Optional override
            [{;]                          # Opening brace or semicolon
        )
    '''

    #wanted_pattern_old = r'/\*\s*=+\s*([\w:]+(?:::[\w~]+|::operator[^\s\(\)]*|::[\w]+))\s*=+\s*([\s\S]*?)\s*\*/\s*\n\s*((?:[\w\s\*&:<>,()\[\]]+\s+)?\1\s*\([^\)]*\)\s*(?:const\s*)?(?:override\s*)?(?:{|\;))'
    wanted_pattern = r'''/\*\s*=+\s*
        ([\w:]+
            (?:::[\w~]+|
            ::operator[^\s\(\)]*|
            ::[\w]+
            )
        )
        \s*=+\s*
        ([\s\S]*?)\s*\*/
        \s*\n\s*
        (
            (?:
                [\w\s\*&:<>,()\[\]]+\s+
            )?\1
            \s*\([^\)]*\)
            \s*(?:const\s*)?
            (?:override\s*)?
            (?:{|\;)
        )
    '''


    # Comment pattern for detecting unmatched comments
    comment_pattern = r'''(?xms)
        /\* \s* =+ \s*                    # Opening comment /* ===
        (?P<method> [\w:]+                # Class::Method or similar
            (?:
                ::[\w~]+              |
                ::operator[^\s\(\)]*  |
                ::[\w]+
            )
        )
        \s* =+ \s*                        # Second === line
        (?P<desc> (?:(?!=+\s*\*/).|\s)*?) # Description, excluding closing ===
        (?:\s* =+ \s*)? \*/               # Optional closing === and comment end
    '''
    comment_pattern = r'/\*\s*=+\s*([\w:]+(?:::[\w~]+|::operator[^\s\(\)]*|::[\w]+))\s*=+\s*([\s\S]*?)\s*\*/'

    # Fallback pattern to catch all comments with '='
    fallback_pattern = r'''(?xms)
        /\*+\s*=+[^\n]*\n+[\s\S]*?\s*\*/
    '''
    fallback_comments = re.finditer(fallback_pattern, content)
    for match in fallback_comments:
        comment_text = match.group(0)
        if 'idCVarSystemLocal' in comment_text:
            print(colored(f"Fallback comment detected in {file_path}:", "yellow"))
            print(f"  Raw comment: '{comment_text}'")
            print(f"  Following text: {content[match.end():match.end()+100][:50]}...")

    unmatched_comments = re.finditer(comment_pattern, content)
    for match in unmatched_comments:
        class_method = match.group(1)
        description = match.group(2).rstrip()
        #class_method = match.group('method').strip()
        #description = match.group('desc').rstrip()
        # Check if this comment matches the full pattern
        full_text = match.group(0) + '\n' + content[match.end():match.end()+200]
        if not re.match(wanted_pattern, full_text):
            print(colored(f"Unmatched comment in {file_path}:", "red"))
            print(f"  Class::Method: {class_method}")
            print(f"  Description: '{description}'")
            print(f"  Following text: {content[match.end():match.end()+100][:50]}...")

    def replacement(match):
        #class_method = match.group('method')
        #description = match.group('desc').rstrip()
        #full_signature = match.group('sig')
        class_method = match.group(1)
        description = match.group(2).rstrip()
        full_signature = match.group(3)
        
        # Debug matched comment
        print(colored(f"Match in {file_path}:", "green"))
        print(f"  Class::Method: {class_method}")
        print(f"  Description: '{description}'")
        print(f"  Signature: {full_signature}")
        
        # If a description is present, convert to Doxygen \brief comment
        if description and description.strip():
            # Format multi-line description for Doxygen
            lines = description.splitlines()
            doxygen_lines = [f' * {line.strip()}' if line.strip() else ' *' for line in lines]
            doxygen_comment = '/**\n' + '\n'.join([' * \\brief ' + doxygen_lines[0].lstrip(' *')] + doxygen_lines[1:]) + '\n */\n    '
            return f'{doxygen_comment}{full_signature}'
        else:
            # Without description, remove the comment
            return f'{full_signature}'
    
    # Search and replace the comments
    cleaned_content, count = re.subn(wanted_pattern, replacement, content, flags=re.MULTILINE | re.VERBOSE)
    
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
    remove_useless_comments('framework/CVarSystem.cpp')