#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void error(const char* message) {
    if (errno != 0)
        perror(message);
    else
        fputs(message, stderr);
    exit(1);
}

int connectIO(const char* command, char* const argv[], const char* display) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
        error("socket error");

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, display, sizeof(addr.sun_path) - 1);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
        error("connect error");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    if (dup(fd) != STDIN_FILENO)
        error("dup error on stdin");
    if (dup(fd) != STDOUT_FILENO)
        error("dup error on stdout");
    return execvp(command, argv);
}

int main(int argc, char* argv[]) {
    if (argc < 2)
        error("usage: xc [command]\n");
    const char* display = getenv("DISPLAY");
    if (display == NULL)
        error("DISPLAY environment variable not set\n");
    const char* command = argv[1];
    char* const* arguments = &(argv[1]);
    return connectIO(command, arguments, display);
}
