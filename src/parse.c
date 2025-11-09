#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/parse.h"

int output_file(int fd, struct dbheader_t *header,
                struct employee_t *employees) {
  if (fd < 0) {
    printf("Invalid FD\n");
    return STATUS_ERROR;
  }

  header->version = htons(header->version);
  header->count = htons(header->count);
  header->magic = htonl(header->magic);
  header->filesize = htonl(header->filesize);

  lseek(fd, 0, SEEK_SET);

  if (write(fd, header, sizeof(struct dbheader_t)) !=
      sizeof(struct dbheader_t)) {
    perror("write");
    return STATUS_ERROR;
  }

  return STATUS_GOOD;
}

int create_db_header(struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  header->version = 0x1;
  header->count = 0;
  header->magic = HEADER_MAGIC;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;

  return STATUS_GOOD;
}

/* Read a dbheader from a file and validate the fields */
int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Invalid FD\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  if (read(fd, header, sizeof(struct dbheader_t)) !=
      sizeof(struct dbheader_t)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->magic = ntohl(header->magic);
  header->filesize = ntohl(header->filesize);

  if (header->version != 1) {
    printf("Improper header version\n");
    free(header);
    return STATUS_ERROR;
  }

  if (header->magic != HEADER_MAGIC) {
    printf("Improper header magic\n");
    free(header);
    return STATUS_ERROR;
  }

  struct stat dbstat = {0};
  if (fstat(fd, &dbstat) < 0) {
    printf("Invalid file\n");
    free(header);
    return STATUS_ERROR;
  }

  if (header->filesize != dbstat.st_size) {
    printf("Corrupted db header\n");
    free(header);
    return STATUS_ERROR;
  }

  *headerOut = header;

  return STATUS_GOOD;
}
