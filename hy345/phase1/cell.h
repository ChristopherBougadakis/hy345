#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#define PATH_MAX 4096



char *cell_read_line();


int main(int ac, char **av);


typedef struct token{
  int operator_id; // 0: no operator, 1: ; , 2: && , 3: ||
  char *value; 
  struct token *next;
  struct token *prev;     

}token_list;

void print_token_list(struct token *head);
char **token_array(struct token *head , int count);
int count_tokens(struct token *head);

void print_tokens(struct token *head);
char *trim_line(char *line);
struct token *list_tokens(struct token *head , char *line);
void    exec_command(struct token *node, char **array, int count);
void free_tokens(struct token *head);
