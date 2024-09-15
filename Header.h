#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>


#define BUFFERSIZE 1024


struct fileData
{
    char name[120];
    char mode[12];
    time_t modificationTime;
    time_t lastAccessTime;
    ino_t inode;
    off_t size;
};

struct time
{
    int hours;
    int minutes;
    int seconds;
};

void time_in_seconds_to_time(time_t seconds, int *h, int *m, int *s)
{
    *h = seconds / 3600;
    *m = (seconds % 3600) / 60;
    *s = seconds % 60;
}

void open_snapshot_file(const char *file_path, int *fd_pointer)
{
    if ((*fd_pointer = open(file_path, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH)) < 0)
    {
        printf("Error opening file...\n");
        exit(7);
    }
}

void write_snapshot(int fd, struct fileData *fileD)
{
    char buffer[BUFFERSIZE];
    int hM, mM, sM;
    time_in_seconds_to_time(fileD->modificationTime, &hM, &mM, &sM); // for modification time
    int hA, mA, sA;
    time_in_seconds_to_time(fileD->lastAccessTime, &hA, &mA, &sA);

    snprintf(buffer, BUFFERSIZE, "Name of the file: %s\nMode of the file: %s\nModification time: %d:%d:%d\nInode: %lu\nLast access time: %d:%d:%d\nSize: %lu\n\n", fileD->name, fileD->mode, hM, mM, sM, fileD->inode, hA, mA, sA, fileD->size);
    write(fd, buffer, strlen(buffer));
}

void move_file_quarantine(char *file_path, struct fileData *fData, char *quarantine_dir)
{
    char new_file_path[256];
    snprintf(new_file_path, sizeof(new_file_path), "%s/%s", quarantine_dir, fData->name);
    rename(file_path, new_file_path);
    printf("Moved %s to %s\n", file_path, quarantine_dir);
    return;
}

void analyze_file(char *file_path, struct fileData *fData, char *quarantine_dir)
{
    pid_t pid;
    int pfd[2];
    if (pipe(pfd) < 0)
    {
        perror("error pipe...\n");
        exit(8);
    }
    pid = fork();
    int status;
    char cmd[256];
    
    if (pid < 0)
    {
        perror("Error creating grandchild proces...\n");
        exit(9);
    }
    else if (pid == 0)
    {
        close(pfd[0]);
        snprintf(cmd, sizeof(cmd), "./script.sh %s", file_path);
        if (dup2(pfd[1], 1) == -1)
        {
            perror("error dup...\n");
            exit(10);
        }
        close(pfd[1]);
        if (execl("/bin/bash", "/bin/bash", "-c", cmd, NULL) == -1)
        {
            perror("error executing script...\n");
            exit(11);
        }
        printf("%s\n", cmd);
    }
    close(pfd[1]);
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) == true)
    {
        if (WEXITSTATUS(status) != 1)
        {
            char buff[100];
            size_t bytes_read = read(pfd[0], buff, sizeof(buff) - 1);
            if (bytes_read > 0)
            {
                buff[bytes_read - 1] = '\0';
                printf("%s\n", buff);
                if (strcmp(buff, "SAFE") != 0)
                    move_file_quarantine(file_path, fData, quarantine_dir);
            }
            printf("The grandchiild process in analyze_file with PID: %d has ended with the code: %d\n", pid, status);
        }
        else
            printf("The grandchild process in analyze_file with PID: %d has not ended\n", pid); 
    }
    close(pfd[0]);
}
