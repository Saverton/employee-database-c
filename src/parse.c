#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/parse.h"

int update_employee_hours(struct dbheader_t *dbhdr,
                          struct employee_t *employees, char *updatestring) {
  if (dbhdr == NULL)
    return STATUS_ERROR;
  if (employees == NULL)
    return STATUS_ERROR;
  if (updatestring == NULL)
    return STATUS_ERROR;

  char *name = strtok(updatestring, ",");
  if (name == NULL)
    return STATUS_ERROR;

  char *hours = strtok(NULL, ",");
  if (hours == NULL)
    return STATUS_ERROR;

  int i = 0;
  for (; i < dbhdr->count; i++) {
    if (strncmp(employees[i].name, name, sizeof(employees[i].name)) == 0) {
      employees[i].hours = atoi(hours);
    }
  }

  return STATUS_GOOD;
}

int remove_employees(struct dbheader_t *dbhdr, struct employee_t **employees,
                     char *removename) {
  if (dbhdr == NULL)
    return STATUS_ERROR;
  if (employees == NULL)
    return STATUS_ERROR;
  if (*employees == NULL)
    return STATUS_ERROR;
  if (removename == NULL)
    return STATUS_ERROR;

  struct employee_t *e = *employees;

  int newcount = dbhdr->count;
  bool *remove = calloc(dbhdr->count, sizeof(bool));
  if (remove == NULL)
    return STATUS_ERROR;

  int i = 0;
  for (; i < dbhdr->count; i++) {
    if (strncmp(e[i].name, removename, sizeof(e[i].name)) == 0) {
      remove[i] = true;
      newcount--;
    } else {
      remove[i] = false;
    }
  }

  struct employee_t *e2 = calloc(newcount, sizeof(struct employee_t));
  if (e2 == NULL) {
    free(remove);
    return STATUS_ERROR;
  }

  i = 0;     // read from old arr
  int j = 0; // write to new arr
  for (; i < dbhdr->count; i++) {
    if (remove[i] == true) {
      continue;
    }

    if (j >= newcount) {
      printf("unexpected state\n");
      free(remove);
      free(e2);
      return STATUS_ERROR;
    }

    e2[j] = e[i];
    j++;
  }

  *employees = e2;
  dbhdr->count = newcount;

  free(remove);
  free(e);

  return STATUS_GOOD;
}

int list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
  if (dbhdr == NULL)
    return STATUS_ERROR;
  if (employees == NULL)
    return STATUS_ERROR;

  for (int i = 0; i < dbhdr->count; i++) {
    printf("Employee %d\n", i);
    printf("\tName: %s\n", employees[i].name);
    printf("\tAddress: %s\n", employees[i].address);
    printf("\tHours: %d\n", employees[i].hours);
  }

  return STATUS_GOOD;
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees,
                 char *addstring) {
  if (dbhdr == NULL)
    return STATUS_ERROR;
  if (employees == NULL)
    return STATUS_ERROR;
  if (*employees == NULL)
    return STATUS_ERROR;
  if (addstring == NULL)
    return STATUS_ERROR;

  char *name = strtok(addstring, ",");
  if (name == NULL)
    return STATUS_ERROR;

  char *addr = strtok(NULL, ",");
  if (addr == NULL)
    return STATUS_ERROR;

  char *hours = strtok(NULL, ",");
  if (hours == NULL)
    return STATUS_ERROR;

  struct employee_t *e =
      reallocarray(*employees, dbhdr->count + 1, sizeof(struct employee_t));
  if (e == NULL)
    return STATUS_ERROR;

  dbhdr->count++;

  struct employee_t *new_employee = &e[dbhdr->count - 1];

  strncpy(new_employee->name, name, sizeof(new_employee->name));
  strncpy(new_employee->address, addr, sizeof(new_employee->address));
  new_employee->hours = atoi(hours);

  *employees = e;

  return STATUS_GOOD;
}

int read_employees(int fd, struct dbheader_t *dbhdr,
                   struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("Invalid FD\n");
    return STATUS_ERROR;
  }

  struct employee_t *employees =
      calloc(dbhdr->count, sizeof(struct employee_t));
  if (employees == NULL) {
    printf("Malloc failed\n");
    return -1;
  }

  read(fd, employees, dbhdr->count * sizeof(struct employee_t));

  for (int i = 0; i < dbhdr->count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;

  return STATUS_GOOD;
}

int output_file(int fd, struct dbheader_t *header,
                struct employee_t *employees) {
  if (fd < 0) {
    printf("Invalid FD\n");
    return STATUS_ERROR;
  }

  int realcount = header->count;
  int newsize =
      sizeof(struct dbheader_t) + (realcount * sizeof(struct employee_t));

  header->version = htons(header->version);
  header->count = htons(header->count);
  header->magic = htonl(header->magic);
  header->filesize = htonl(newsize);

  lseek(fd, 0, SEEK_SET);

  if (write(fd, header, sizeof(struct dbheader_t)) !=
      sizeof(struct dbheader_t)) {
    perror("write");
    return STATUS_ERROR;
  }

  for (int i = 0; i < realcount; i++) {
    employees[i].hours = htonl(employees[i].hours);
    if (write(fd, &employees[i], sizeof(struct employee_t)) !=
        sizeof(struct employee_t)) {
      perror("write");
      return STATUS_ERROR;
    }
  }

  if (ftruncate(fd, newsize) != 0) {
    perror("ftruncate");
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
