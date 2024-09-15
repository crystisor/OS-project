#include "workingWithFile.h"

/*  
    #include "workingWithFile.h"
    This header contains all the structs used in my project and also all the functions that
    works with files.
    
    void check_arguments(int argc, char *argv)
    Checks command line call

    char *get_permissions(mode_t mode) 
    Gets the string for file perimissions

    bool check_permisions(int file_descriptor, struct fileData *fData)
    The function that checks if a file has no permissions then it is not safe

    void read_directories(char *dir_path, int file_descriptor, char *quarantine_dir)
    The recurssive function that iterattes through directories given as command line
    arguments and stores for every file attributes that are going to be written in snapshots

    int check_snapshot(char *dir_path, char *current_snapshot, char *current_snapshot1)
    This function has as argument the Output directory and checks if there are any snapshots
    for a directory and returns a flag.

    bool compare_snapshots(char *snap_path, char *snap1_path)
    Compares to snapshots of the same directory and updates them accordingly.

    pid_t create_snapshot(int mode, char* current_dir, char *snap, char *snap1, int file_descriptor, char *quarantine_dir)
    Creates snapshot for a directory creating a child process for each snapshot that has to be created
    
    void traverse_dirs(int argc, char **argv)
    This is the main function of the program.It goes through directories and calls create_snapshot() for
    each command line argument.

    void time_in_seconds_to_time(time_t seconds, int *h, int *m, int *s)
    Converts the time in seconds computed for each file in each directory.Time is in seconds since 1970.

    void open_snapshot_file(const char *file_path, int *fd_pointer)
    Opens a snapshot with different modes and returns it's file_descriptor

    void write_snapshot(int fd, struct fileData *fileD)
    Writes inside the snapshot attributes of a file from a dir

    void move_file_quarantine(char *dir_path, struct fileData *fData, char *quarantine_dir)
    Depending on what the script returns, if it is found dangerous, moves the file inside
    izolated_space_dir

    void analyze_file(char *dir_path, struct fileData *fData, char *quarantine_dir)
    If a file is found with no permissions, this function is called.It calls the bash script
    that checks the insides of a file without opening it.The grandchild processes are 
    communicating with child processes through pipes.If the exit code of the script indicates
    a SAFE file then it's name is printed in terminal, else move_file_quarantine is called.
*/

void check_arguments(int argc, char **argv)
{
    if (argc < 2)
    {
        perror("Too few arguments...\n");
        exit(1);
    }
    if (argc > 9)
    {
        perror("Too many arguments...\n");
        exit(1);
    }
    if (strcmp(argv[1], "-o") != 0)
    {
        perror("-o argument required...\n");
        exit(1);
    }
    if (strcmp(argv[3], "-s") != 0)
    {
        perror("-s argument requiered...\n");
        exit(1);
    }
    if (strcmp(argv[4], "izolated_space_dir") != 0)
    {
        perror("quarantine argument required...\n");
        exit(1);
    }
    for (int i = 5; i < argc - 1; i++)
    {
        if (strcmp(argv[i],argv[i + 1]) == 0)
        {
            perror("All arguments must be different...\n");
            exit(1);
        }
    }
}

char *get_permissions(mode_t mode) 
{
    char *permission = (char*)calloc(sizeof(char), 12);
    int i = 0;
        permission[i++] = (S_ISDIR(mode)) ? 'd' : '-';
        permission[i++] = (mode & S_IRUSR) ? 'r' : '-';
        permission[i++] = (mode & S_IWUSR) ? 'w' : '-';
        permission[i++] = (mode & S_IXUSR) ? 'x' : '-';
        permission[i++] = (mode & S_IRGRP) ? 'r' : '-';
        permission[i++] = (mode & S_IWGRP) ? 'w' : '-';
        permission[i++] = (mode & S_IXGRP) ? 'x' : '-';
        permission[i++] = (mode & S_IROTH) ? 'r' : '-';
        permission[i++] = (mode & S_IWOTH) ? 'w' : '-';
        permission[i++] = (mode & S_IXOTH) ? 'x' : '-';
    return permission;
}
bool check_permisions(int file_descriptor, struct fileData *fData)
{
    int count = 0;
    for (int i = 0; fData->mode[i] != '\0'; i++)
    {
        if (fData->mode[i] == '-')
            count++;
    }
    if (count == strlen(fData->mode))
        return false;
    else
        return true;
}
void read_directories(char *dir_path, int file_descriptor, char *quarantine_dir)
{
    DIR *dir;
    dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("Error opening dir...\n");
        exit(2);
    }
    struct dirent *entry; // for readdir
    struct stat file_stat; // for files inside dirs

    while ((entry = readdir(dir)) != NULL) // readdir returns pointer to dirent struct
    {
        char full_dir_path[1024];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name,"..") == 0)
            continue;
        snprintf(full_dir_path, sizeof(full_dir_path), "%s/%s", dir_path, entry->d_name);

        if (lstat(full_dir_path, &file_stat) == -1) //second argument is pointer to stat struct, so you take address
        {
            perror("Unable to get the file status...\n");
            continue;
        }

        struct fileData f_data;
        strcpy(f_data.name, entry->d_name);
        strcpy(f_data.mode, get_permissions(file_stat.st_mode));
        f_data.modificationTime = file_stat.st_mtime;
        f_data.inode = file_stat.st_ino;
        f_data.lastAccessTime = file_stat.st_atime;
        f_data.size = file_stat.st_size;
        
        if (check_permisions(file_descriptor, &f_data) == true)             // safe
            write_snapshot(file_descriptor, &f_data);
        else
            analyze_file(full_dir_path, &f_data, quarantine_dir);
        
        if (S_ISDIR(file_stat.st_mode))
            read_directories(full_dir_path, file_descriptor, quarantine_dir);
    }
    closedir(dir);
}
int check_snapshot(char *dir_path, char *current_snapshot, char *current_snapshot1)
{
    int mode = 0;
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("Error opening dir...\n");
        exit(3);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char full_path[1024];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name,"..") == 0)
            continue;
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (lstat(full_path, &file_stat) == -1)
        {
            perror("Error getting file info...\n");
            exit(1);
        }
        if (S_ISDIR(file_stat.st_mode))
            continue;
        if (strcmp(entry->d_name, current_snapshot1) == 0)
        {
            mode = -1;
        }
        if (strcmp(entry->d_name, current_snapshot) == 0)
        {
            mode = 1;
        }
    }
    closedir(dir);
    return mode;
}

bool compare_snapshots(char *snap_path, char *snap1_path)
{
    int fd1 = open(snap_path, O_RDONLY);
    if (fd1 == -1)
    {
        perror("Error opening file...\n");
        exit(4);
    }
    int fd2 = open(snap1_path, O_RDONLY);
    if (fd2 == -1)
    {
        perror("Error opening file...\n");
        exit(4);
    }

    char ch1, ch2;
    ssize_t bytes_read1, bytes_read2; // chunk of data

    do
    {
        bytes_read1 = read(fd1, &ch1, sizeof(char));
        if (bytes_read1 == -1)
        {
            perror("Error reading char from snap 1");
            exit(5);
        }
        bytes_read2 = read(fd2, &ch2, sizeof(char));
        if (bytes_read2 == -1)
        {
            perror("Error reading char from snap 2");
            exit(5);
        }
        if (ch1 != ch2)
        {
            close(fd1);
            close(fd2);
            return false;
        }
    }while(bytes_read1 > 0 && bytes_read2 > 0); // 0 for EOF

    close(fd1);
    close(fd2);
    if (bytes_read1 == 0 && bytes_read2 == 0)
        return true;
    return false;
}
pid_t create_snapshot(int mode, char* current_dir, char *snap, char *snap1, int file_descriptor, char *quarantine_dir)
{
    pid_t pid;
    pid = fork();
    if (pid < 0) 
    {
        perror("fork error...\n");
        exit(6);
    } 
    else if (pid == 0) 
    {
        printf("Child process: %d\n", getpid());

        if (mode == 0)
        {
            open_snapshot_file(snap, &file_descriptor);
            read_directories(current_dir, file_descriptor, quarantine_dir);
            printf("Snapshot: %s created successfully\n", snap + 7);
            close(file_descriptor);
        }
        else if (mode == 1)
        {
            open_snapshot_file(snap1, &file_descriptor);
            read_directories(current_dir, file_descriptor, quarantine_dir);
            printf("Snapshot: %s created successfully\n", snap1 + 7);
            close(file_descriptor);
        }
        else if (mode == -1)
        {     
            if (compare_snapshots(snap, snap1) == true)
                remove(snap1), printf("Equal snapshots for: %s => removed second snapshot\n", current_dir);
            else
            {
                remove(snap);
                rename(snap1, snap);
                printf("Different snapshots for: %s => removed first snapshot\n", current_dir);
            }  
        }
        exit(0);
    }
    return pid;
}

void traverse_dirs(int argc, char **argv)
{
    for (int i = 5; i < argc; i++)          // ./Project -o Output -s Isolated dir1 dir 2 ...
    {
        pid_t pid;

        char test_snapshot_name[BUFFERSIZE];
        char test_snapshot1_name[BUFFERSIZE];
        snprintf(test_snapshot_name, BUFFERSIZE, "%s_%s", "snapshot", argv[i]);
        snprintf(test_snapshot1_name, BUFFERSIZE, "%s1_%s", "snapshot", argv[i]);
        int ok = check_snapshot(argv[2], test_snapshot_name, test_snapshot1_name);
        
        int fd = 0;
        char test_snapshot[BUFFERSIZE];
        snprintf(test_snapshot, BUFFERSIZE, "%s/%s_%s", argv[2], "snapshot", argv[i]);
        char test_snapshot1[BUFFERSIZE];
        snprintf(test_snapshot1, BUFFERSIZE, "%s/%s1_%s", argv[2], "snapshot", argv[i]);

        pid = create_snapshot(ok, argv[i], test_snapshot, test_snapshot1, fd, argv[4]);
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) == true)
            printf("The child process in traverse_dirs with PID: %d has ended with the code: %d\n", pid, status);
        else
            printf("The process child in traverse_dirs with PID: %d has not ended\n", pid);
    }
}
int main(int argc, char **argv)
{
    check_arguments(argc, argv);
    traverse_dirs(argc, argv);
    
    return 0;
}
