import os
import re
import threading
from pathlib import Path
from functools import wraps
from termcolor import colored

# Timeout decorator to prevent regex hanging
def timeout(seconds):
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            result = [None]
            exception = [None]
            
            def target():
                try:
                    result[0] = func(*args, **kwargs)
                except Exception as e:
                    exception[0] = e
            
            thread = threading.Thread(target=target)
            thread.daemon = True
            thread.start()
            thread.join(seconds)
            
            if thread.is_alive():
                print(colored(f"Warning: Function {func.__name__} timed out after {seconds} seconds", "yellow"))
                return None
            if exception[0]:
                raise exception[0]
            return result[0]
        
        return wrapper
    return decorator

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

    # Main pattern for comments and signatures (human-readable)
    main_pattern = r'''(?xms)  # Verbose mode, multi-line, dot-all
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
                [\w\s*\*&:<>]+            # Return type, including pointers and templates
                \s*\*?                    # Optional pointer (e.g., char*)
                \s+
            )?                            # Return type is optional
            (?P<sig_method> [\w:]+        # Signature method name (may differ from comment)
                (?:
                    ::[\w~]+          |
                    ::operator[^\s\(\)]* |
                    ::[\w]+
                )
            )
            \s* \(                        # Opening parenthesis for parameter list
                (?:
                    [^()]+ |              # Non-parentheses content
                    \([^()]*\)            # Balanced parentheses (e.g., for function pointers)
                )*                        # Zero or more parameters
            \)                            # Closing parenthesis
            \s* (?:const\s*)?             # Optional const
            (?:override\s*)?              # Optional override
            \s* (?:/\*.*?\*/|//[^\n]*)?   # Optional trailing comment (/* */ or //)
            \s* [{;]                      # Opening brace or semicolon
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

    # Fallback pattern to catch all comments with '='
    if 0:
        fallback_pattern = r'''(?xms)
            /\*+\s*=+[^\n]*\n+[\s\S]*?\s*\*/
        '''
        fallback_comments = re.finditer(fallback_pattern, content)
        for match in fallback_comments:
            comment_text = match.group(0)
            if 'idMultiplayerGame' in comment_text or 'idCVarSystemLocal' in comment_text:
                print(f"Fallback comment detected in {file_path}:")
                print(f"  Raw comment: '{comment_text}'")
                print(f"  Following text: {content[match.end():match.end()+100][:50]}...")

    # Wrap re.match with timeout to prevent hanging
    #@timeout(5)
    #def safe_match(pattern, text):
    #    return re.match(pattern, text)

    unmatched_comments = re.finditer(comment_pattern, content)
    for match in unmatched_comments:
        class_method = match.group('method').strip()
        description = match.group('desc').rstrip()
        # Check if this comment matches the full pattern
        full_text = match.group(0) + '\n' + content[match.end():match.end()+200]
        match_result = re.match(main_pattern, full_text)
        if not match_result:
            print(colored(f"Unmatched comment in {file_path}:", "red"))
            print(f"  Class::Method: {class_method}")
            print(f"  Description: '{description}'")
            print(f"  Following text: {content[match.end():match.end()+100][:50]}...")
            print(f"  Full text attempted: {full_text[:100]}...")
        # try:
        #     #match_result = safe_match(main_pattern, full_text)
        #     match_result = re.match(main_pattern, full_text)
        #     if not match_result:
        #         print(colored(f"Unmatched comment in {file_path}:", "red"))
        #         print(f"  Class::Method: {class_method}")
        #         print(f"  Description: '{description}'")
        #         print(f"  Following text: {content[match.end():match.end()+100][:50]}...")
        #         print(f"  Full text attempted: {full_text[:100]}...")
        # except TimeoutError:
        #     print(colored(f"Regex timeout in {file_path} for comment:", "red"))
        #     print(f"  Class::Method: {class_method}")
        #     print(f"  Description: '{description}'")
        #     print(f"  Full text attempted: {full_text[:100]}...")

    def replacement(match):
        class_method = match.group('method')
        description = match.group('desc').rstrip()
        sig_method = match.group('sig_method')
        full_signature = match.group('sig')
        
        # Validate that sig_method is reasonably close to method (same class)
        class_prefix = class_method.rsplit('::', 1)[0]
        sig_class_prefix = sig_method.rsplit('::', 1)[0]
        if class_prefix != sig_class_prefix:
            print(colored(f"Warning: Class mismatch in {file_path}:", "red"))
            print(f"  Comment method: {class_method}")
            print(f"  Signature method: {sig_method}")
            return match.group(0)  # Skip replacement if class differs
        
        # Debug matched comment
        print(colored(f"Match in {file_path}:", "green"))
        print(f"  Class::Method: {class_method}")
        print(f"  Signature Method: {sig_method}")
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
    cleaned_content, count = re.subn(main_pattern, replacement, content, flags=re.MULTILINE | re.VERBOSE)
    
    # If changes were made, overwrite the file
    if count > 0:
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(cleaned_content)
        print(f"Processed {count} comments in {file_path}")
    else:
        print(f"No matching comments found in {file_path}")

def process_directory(skip_dirs=None, skip_files=None):
    """
    Recursively processes all .cpp and .h files in the current directory,
    skipping:
      - directories listed in 'skip_dirs'
      - files listed in 'skip_files'
    """
    import os

    # Normalize skip_dirs and skip_files
    if skip_dirs is None:
        skip_dirs = set()
    else:
        skip_dirs = {d.lower() for d in skip_dirs}

    if skip_files is None:
        skip_files = set()
    else:
        skip_files = {os.path.abspath(f).lower() for f in skip_files}

    target_directory = os.path.dirname(os.path.abspath(__file__))
    print(f"Processing directory: {target_directory}")

    for root, dirs, files in os.walk(target_directory):
        # Skip directories (by name only, not full path)
        if any(os.path.basename(root).lower() == d for d in skip_dirs):
            print(f"Skipping directory: {root}")
            dirs[:] = []  # prevent recursion
            continue

        for file in files:
            if file.endswith(('.cpp', '.h')):
                file_path = os.path.abspath(os.path.join(root, file)).lower()
                if file_path in skip_files:
                    print(f"Skipping file: {file_path}")
                    continue
                remove_useless_comments(file_path)

if __name__ == "__main__":

    if 1:
        # Debugging
        remove_useless_comments('d3xp/MultiplayerGame.cpp')
    else:
        skip_dirs = {'extern', 'libs', 'thirdparty'}
        skip_files = {
            'cm/CollisionModel_load.cpp',
            'cm/CollisionModel_rotate.cpp',
            'd3xp/Achievements.cpp',
            'd3xp/Actor.cpp',
            'd3xp/AFEntity.cpp',
            'd3xp/EnvironmentProbe.cpp',
            'd3xp/Game_local.cpp',
        }
        process_directory(skip_dirs, skip_files)