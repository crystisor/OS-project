# OS_project


This project is a C-based program designed to monitor and track modifications to the contents of specified folders. It can generate snapshots of directories and compare them, enabling the user to detect changes in files, such as modifications, deletions, and permission changes. Files deemed unsafe can be moved to a quarantine directory for further analysis.

# Features
Snapshot Creation: Creates snapshots of the contents of directories, capturing file names, permissions, modification times, and other file attributes.
Snapshot Comparison: Compares two snapshots to detect changes in the contents or metadata of files.
Quarantine System: Automatically analyzes files, and if they are deemed unsafe, moves them to an isolated quarantine directory.
Process Forking: Uses child processes to handle snapshot creation, file analysis, and quarantine management, allowing concurrent operations.

# Requirements
* C Compiler: GCC or any other standard C compiler.

* POSIX-compliant Operating System: Linux or macOS (for proper directory and file management).

* Bash Script: This program assumes the existence of an external script (script.sh) for analyzing files to determine if they are safe or need to be quarantined.

# Instalation
Clone the repository:

    git clone https://github.com/your-username/folder-modification-tracker.git
Compile the code:

    gcc -o FolderTracker main.c workingWithFile.h
Make sure the bash script is inside the folder that contains the code and execute:

    ./FolderTracker -o <output_dir> -s izolated_space_dir <dir1> <dir2> ...


# Usage
The program requires several command-line arguments:

* -o: Specifies the output directory where the snapshot files will be stored.

* -s: Specifies the quarantine directory where unsafe files will be moved.

* dir1, dir2, ...: List of directories to monitor and track for changes.

**Example**

     ./FolderTracker -o OutputDir -s quarantineDir folder1 folder2 folder3

**Arguments**

* OutputDir: Directory to store snapshots.

* quarantineDir: Directory where unsafe files are moved.

* folder1, folder2, ...: Folders to track for changes.

**Bash script**

This script performs a series of checks on a file passed as a command-line argument to ensure its safety. It validates the file based on several criteria:
* File existance: 
    The script first checks if the provided argument is a valid, regular file.
* File properties: 
    It calculates the number of characters, words, and lines in the file.
* Non-ASCII Character Check
* Keyword Search: 
    The script searches for specific malicious words like "corrupted", "dangerous", "risk", "attack", "malware", and "malicious". If any of these words are detected, the file is marked as unsafe.
* Permissions: 
    For files deemed unsafe, read permissions are removed, and the script exits with a success code (0). If the file passes all checks, it is deemed "SAFE" and the script exits with an exit code of 2.


# License
This is an open-source project made in academic purpose as a project in one of my computer-science faculty subjects.

