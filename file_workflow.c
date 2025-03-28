#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

//#define IS_STACK

void clean_up_alloc_for_buffer(char **buffer)
{
    printf("\tCleaning up (free) buffer...\n");
    free(*buffer);
}

void clean_up_open_for_fd(int *fd)
{
    printf("\tCleaning up (close) fd...\n");
    if (close(*fd) == -1) {
        perror("\tGot error");
    }
}

void file_workflow(const char *file_name)
{
#ifndef IS_STACK
    #define DATA_SIZE (3)
    char *data __attribute__ ((__cleanup__(clean_up_alloc_for_buffer))) = NULL;
    char *data_read __attribute__ ((__cleanup__(clean_up_alloc_for_buffer))) = NULL;
#else
    char data[] = { '1', '2', '3' };
    #define DATA_SIZE (sizeof(data) / sizeof(data[0]))
    char data_read[DATA_SIZE] = {};
#endif
    int fd  __attribute__ ((__cleanup__(clean_up_open_for_fd))) = -1;
    int result = -1;
   
    printf("\n");
    printf("File name: %s\n", file_name);
    printf("Data size: %lu\n", (unsigned long int)DATA_SIZE);
    printf("\n");

    printf("Prepare to writing data file...\n");
#ifndef IS_STACK
    data = malloc(DATA_SIZE);
    if (data == NULL) {
        perror("\tGot error");
        return;
    }
    srand(time(0));
    for (unsigned int i = 0; i < DATA_SIZE; ++i) {
        data[i] = rand();
    }

    data_read = calloc(1, DATA_SIZE);
    if (data_read == NULL) {
        perror("\tGot error");
        return;
    }
#endif

    printf("Opening file...\n");
    fd = open(file_name,
              O_RDWR | O_CREAT | O_TRUNC | O_SYNC,
              S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (fd == -1) {
        perror("\tGot error");
        if (errno == ENOSPC || errno == EDQUOT) {
            printf("\tError: ENOSPC/EDQUOT\n");
        }
        return;
    }

    printf("Writing data to file...\n");
    ssize_t len = DATA_SIZE;
    ssize_t ret = 0;
    char *buf = data;
    while (len != 0
        && ((ret = write(fd, buf, len)) != DATA_SIZE)) {
        printf("\tError: Not all data had been written (ret = %zd)\n", ret);
        if (ret == -1) {
            perror("\tGot error");
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOSPC || errno == EDQUOT) {
               printf("\tError: No Space\n");
                break;
            }
            return;
        }
        len -= ret;
        buf += ret;
        printf("\tError: Not full len wrote\n");
    }

    printf("Seek file...\n");
    off_t ret_lseek = lseek(fd, 0, SEEK_SET);
    if (ret_lseek == -1) {
        perror("\tGot error");
        return;
    }
   
    printf("Reading file...\n");
    buf = data_read;
    len = DATA_SIZE;
    while (len != 0
        && (ret = read(fd, buf, len)) != DATA_SIZE) {
        if (ret == -1) {
            perror("\tGot error");
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        len -= ret;
        buf += ret;
        printf("\tError: Not full len read\n");
    }
   
    printf("Printing file...\n");
    if (DATA_SIZE <= 20) {
        for (ssize_t i = 0; i < DATA_SIZE; ++i) {
            printf("0x%x|%c ", data_read[i], data_read[i]);
        }
    }
    printf("\n");

#ifdef __STDC_LIB_EXT1__
    (void)memset_s(data_read, DATA_SIZE, 0x00, DATA_SIZE);
#endif
   
    printf("Closing file...\n");
    // auto closing
   
    printf("End of file writing\n");
    printf("\n");
   
    return;
}

int main()
{
    file_workflow("test.txt");

    return 0;
}
