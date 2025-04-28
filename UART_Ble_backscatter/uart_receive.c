#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define UART_DEVICE "/dev/ttyS1"
#define CSV_FILE "uart_log.csv"
#define BUF_SIZE 256

int main() {
    int uart_fd = open(UART_DEVICE, O_RDONLY | O_NOCTTY);
    if (uart_fd < 0) {
        perror("Failed to open UART");
        return 1;
    }

    // Configure UART
    struct termios options;
    tcgetattr(uart_fd, &options);
    cfsetispeed(&options, B9600);  // Set baud rate to 9600
    options.c_cflag &= ~PARENB;    // No parity
    options.c_cflag &= ~CSTOPB;    // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;        // 8 data bits
    options.c_cflag |= CREAD;      // Enable receiver
    options.c_cflag |= CLOCAL;     // Local line, no modem control
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input
    tcsetattr(uart_fd, TCSANOW, &options);

    // Open CSV file
    FILE *csv_fp = fopen(CSV_FILE, "w");
    if (!csv_fp) {
        perror("Failed to open CSV file");
        close(uart_fd);
        return 1;
    }

    fprintf(csv_fp, "Timestamp,Data\n");

    char buf[BUF_SIZE];
    while (1) {
        int n = read(uart_fd, buf, BUF_SIZE - 1);
        if (n > 0) {
            buf[n] = '\0';  // Null-terminate
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
            fprintf(csv_fp, "\"%s\",\"%s\"\n", timestamp, buf);
            fflush(csv_fp);  // Write immediately
            printf("%s: %s\n", timestamp, buf);
        }
    }

    fclose(csv_fp);
    close(uart_fd);
    return 0;
}
