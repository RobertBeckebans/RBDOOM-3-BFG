import os
import re
import threading
from pathlib import Path
from functools import wraps
from termcolor import colored

def simplify_void_parameters(source: str) -> tuple[str, int]:
    """
    Normalises empty‑parameter lists in C/C++ function declarations:
        ( void )  -> ()
        (void)    -> ()
        ( )       -> ()
    """
    void_param_pattern = re.compile(
    r'\(\s*void\s*\)'   #  (void)  or ( void )
    r'|'                #  or
    r'\(\s+\)',         #  ( ) – min. 1 Space between ( )
    flags=re.IGNORECASE
)
    simplified, subs = void_param_pattern.subn('()', source)
    if subs:
        print(colored(f"  › simplified {subs} '(void)' parameter list(s)", "cyan"))
    return (simplified, subs)

def simplify_void_parameters_file(file_path: str):
    content = None
    for encoding in ['utf-8', 'latin1', 'windows-1252']:
        try:
            with open(file_path, 'r', encoding=encoding) as file:
                content = file.read()
            #print(colored(f"Processing {file_path} with {encoding} encoding ...", "magenta"))
            break
        except UnicodeDecodeError:
            print(colored(f"Failed to read {file_path} with {encoding} encoding", "red"))
            continue
    
    if content is None:
        print(colored(f"Skipping {file_path}: Unable to decode with any supported encoding", "red"))
        return
    
    cleaned_content, count = simplify_void_parameters(content)
    if count > 0:
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(cleaned_content)
        print(f"Processed {count} comments in {file_path}")

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
    #print(f"Processing directory: {target_directory}")

    for root, dirs, files in os.walk(target_directory):
        # Skip directories (by name only, not full path)
        if any(os.path.basename(root).lower() == d for d in skip_dirs):
            print(f"Skipping directory: {root}")
            dirs[:] = []  # prevent recursion
            continue

        for file in files:
            if file.endswith(('.c', '.cpp', '.h')):
                file_path = os.path.abspath(os.path.join(root, file)).lower()
                if file_path in skip_files:
                    print(f"Skipping file: {file_path}")
                    continue

                content = None
                for encoding in ['utf-8', 'latin1', 'windows-1252']:
                    try:
                        with open(file_path, 'r', encoding=encoding) as file:
                            content = file.read()
                        #print(colored(f"Processing {file_path} with {encoding} encoding ...", "magenta"))
                        break
                    except UnicodeDecodeError:
                        print(colored(f"Failed to read {file_path} with {encoding} encoding", "red"))
                        continue
                
                if content is None:
                    print(colored(f"Skipping {file_path}: Unable to decode with any supported encoding", "red"))
                    return
                
                cleaned_content, count = simplify_void_parameters(content)
                if count > 0:
                    with open(file_path, 'w', encoding='utf-8') as file:
                        file.write(cleaned_content)
                    print(f"Processed {count} comments in {file_path}")

if __name__ == "__main__":

    if 0:
        # Debugging
        simplify_void_parameters_file('engine/client/cl_keys.c')
    else:
        skip_dirs = {'extern', 'libs', 'thirdparty'}
        skip_files = {}
        process_directory(skip_dirs, skip_files)