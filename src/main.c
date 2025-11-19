#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/file.h"
#include "../include/parse.h"

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database file>\n", argv[0]);
  printf("\t -n   -  create new database file\n");
  printf("\t -f   -  (required) path to database file\n");
  printf("\t -a   -  add employee as CSV <name,address,hours>\n");
  printf("\t -l   -  list all employees\n");
  printf("\t -r   -  remove employee with <name>\n");
  printf("\t -u   -  update employee hours CSV <name,newhours>\n");
}

int main(int argc, char *argv[]) {
  int c;
  bool newfile = false;
  char *filepath = NULL;
  char *addstring = NULL;
  bool list = false;
  char *removename = NULL;
  char *updatestring = NULL;

  int dbfd = -1;
  struct dbheader_t *dbhdr = NULL;
  struct employee_t *employees = NULL;

  while ((c = getopt(argc, argv, "nf:a:lr:u:")) != -1) {
    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'a':
      addstring = optarg;
      break;
    case 'l':
      list = true;
      break;
    case 'r':
      removename = optarg;
      break;
    case 'u':
      updatestring = optarg;
      break;
    case '?':
      printf("Unknown option -%c\n", c);
      break;
    default:
      return -1;
    }
  }

  if (filepath == NULL) {
    printf("Filepath is a required argument\n");
    print_usage(argv);

    return 0;
  }

  if (newfile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to create database file\n");
      return -1;
    }

    if (create_db_header(&dbhdr) == STATUS_ERROR) {
      printf("Failed to create database header\n");
      close(dbfd);
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to open database file\n");
      return -1;
    }

    if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
      printf("Invalid db header\n");
      close(dbfd);
      return -1;
    }
  }

  if (read_employees(dbfd, dbhdr, &employees) != STATUS_GOOD) {
    printf("Failed to read employees from database\n");
    close(dbfd);
    return -1;
  }

  if (addstring && add_employee(dbhdr, &employees, addstring) != STATUS_GOOD) {
    printf("Failed to add new employee\n");
    close(dbfd);
    return -1;
  }

  if (updatestring &&
      update_employee_hours(dbhdr, employees, updatestring) != STATUS_GOOD) {
    printf("Failed to update employee hours\n");
    close(dbfd);
    return -1;
  }

  if (removename &&
      remove_employees(dbhdr, &employees, removename) != STATUS_GOOD) {
    printf("Failed to remove employee\n");
    close(dbfd);
    return -1;
  }

  if (list && list_employees(dbhdr, employees) != STATUS_GOOD) {
    printf("Error listing employees\n");
    close(dbfd);
    return -1;
  }

  if (output_file(dbfd, dbhdr, employees) != STATUS_GOOD) {
    printf("Failed to write database header to file\n");
    close(dbfd);
    return -1;
  }

  close(dbfd);

  return 0;
}
