#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define NULL 0x00
#define MAX_SIZE 16
#define MIN(x, y) ((x < y) ? (x) : (y))

unsigned int get_file_length(FILE* fp);

void print_error(char* message);

int main(int argc, char** argv)
{
    char data_buffer[MAX_SIZE];

    unsigned char ack;

    int sock;
    unsigned int file_size;
    unsigned int current_size = 0;

    struct sockaddr_in des_ip;

    if(argc < 2)
        print_error("usage: ntransfer [filename]");

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock == -1)
        print_error("socket error");

    des_ip.sin_family = AF_INET;
    des_ip.sin_port = htons(4444);
    des_ip.sin_addr.s_addr = inet_addr("172.31.160.1");

    if(connect(sock, (struct sockaddr*)&des_ip, sizeof(des_ip)) == -1)
        print_error("connecting error");

    FILE* fp = fopen(argv[1], "rb");
    file_size = get_file_length(fp);

    if(fp == NULL)
        print_error("error opening file");

    // send file size
    send(sock, &file_size, 4, 0);

    if(recv(sock, &ack, 1, 0) != 1)
        print_error("error receiving ack");

    while(current_size < file_size)
    {
        unsigned int size = MIN(file_size - current_size, MAX_SIZE);

        fread(data_buffer, sizeof(char), size, fp);

        if(send(sock, &data_buffer, size, 0) != size)
            print_error("error sending data");

        if(recv(sock, &ack, 1, 0) != 1)
            print_error("error receiving ack");

        current_size += size;
    }

    fclose(fp);
    close(sock);

    printf("Send Complete~!\n");
}

unsigned int get_file_length(FILE* fp)
{
    unsigned int size;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("%ld", size);

    return size;
}

void print_error(char* message)
{
    printf("%s\n", message);

    exit(0);
}