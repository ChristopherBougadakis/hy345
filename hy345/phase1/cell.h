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

#define PATH_MAX 4096



char *cell_read_line();


int main(int ac, char **av);


typedef struct token{

   char *value; 
  struct token *next;
  struct token *prev;     

}token_list;


char **token_array(struct token *head , int count);
int count_tokens(struct token *head);

void print_tokens(struct token *head);
char *trim_line(char *line);
struct token *list_tokens(struct token *head , char *line);

void free_tokens(struct token *head);