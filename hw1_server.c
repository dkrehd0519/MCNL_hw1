#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define PATH "./"

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock;
    int clnt_sock;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    struct dirent **namelist = NULL;
    int count = 0, idx = 0;
    char path[1024] = {};
    char allPath[4096] = {};
    struct stat fileStat;

    count = scandir(PATH, &namelist, NULL, NULL);

    if(count < 0){
        error_handling("scandir error");
    }
    
    int counter = 0;
    for (idx = 0; idx < count; idx++) {
        char purePath[1024] = {};
        snprintf(purePath, sizeof(purePath), "%s%s", PATH, namelist[idx]->d_name);

        if (stat(purePath, &fileStat) < 0) {
            perror("stat error");
            continue;
        }

        if (strcmp(namelist[idx]->d_name, ".") == 0 || strcmp(namelist[idx]->d_name, "..") == 0)
            continue;

        counter++;
        snprintf(path, sizeof(path), "%d. %s (%ld bytes)\n",
                counter, namelist[idx]->d_name, fileStat.st_size);
        strcat(allPath, path);
    }


    if(argc != 2) {
        printf("Usage : %s <port> \n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if(listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1)
        error_handling("accept() error");

    int len = strlen(allPath);
    write(clnt_sock, &len, sizeof(len));
    write(clnt_sock, allPath, len);

    int selectedNum = 0;
    read(clnt_sock, &selectedNum, sizeof(selectedNum));

    printf("Client selected: %d\n", selectedNum);

    char fileToSend[1024] = {};
    int realIdx = 0, skip = 0;

    for (idx = 0; idx < count; idx++) {
        if (strcmp(namelist[idx]->d_name, ".") == 0 || strcmp(namelist[idx]->d_name, "..") == 0)
            continue;
        skip++;
        if (skip == selectedNum) {
            snprintf(fileToSend, sizeof(fileToSend), "%s%s", PATH, namelist[idx]->d_name);
            break;
        }
    }

    if (fileToSend[0] == '\0') {
        error_handling("Invalid file number.");
    }

    const char *justName = namelist[idx]->d_name;

    int nameLen = strlen(justName);
    write(clnt_sock, &nameLen, sizeof(nameLen));

    write(clnt_sock, justName, nameLen);

    FILE *fp = fopen(fileToSend, "rb");
    if (fp == NULL)
        error_handling("file()");

    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    rewind(fp);
    write(clnt_sock, &filesize, sizeof(filesize));

    char buf[1024];
    int read_bytes;
    while ((read_bytes = fread(buf, 1, sizeof(buf), fp)) > 0) {
        write(clnt_sock, buf, read_bytes);
    }

    fclose(fp);

    close(clnt_sock);
    close(serv_sock);

    return 0;
}

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
