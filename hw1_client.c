#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

void error_handling(char *message);

int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[4096];
    int str_len = 0;
    int idx = 0, read_len = 0;

    if(argc != 3){
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
    error_handling("connect() error!");

    int total = 0, msg_len = 0;
    
    read(sock, &msg_len, sizeof(msg_len));
    while (total < msg_len) {
        int read_len = read(sock, &message[total], msg_len - total);
        total += read_len;
    }
    message[msg_len] = '\0';

    printf("%s\n", message);

    int sellectNum;

    printf("chose file num: (0 to quit)");
    scanf("%d", &sellectNum);

    write(sock, &sellectNum, sizeof(sellectNum));

    int nameLen = 0;
    read(sock, &nameLen, sizeof(nameLen));

    char filename[256] = {};
    read(sock, filename, nameLen);
    filename[nameLen] = '\0'; 


    int filesize = 0;
    read(sock, &filesize, sizeof(filesize));

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
        error_handling("fopen failed");

    int received = 0;
    char buf[1024];
    while (received < filesize) {
        int n = read(sock, buf, sizeof(buf));
        if (n <= 0)
            break;
        fwrite(buf, 1, n, fp);
        received += n;
    }

    fclose(fp);
    printf("file saved.'\n");

}

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}