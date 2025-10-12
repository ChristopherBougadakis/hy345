
#include "cell.h"
// read a line from stdin
char *cell_read_line()
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if ((read = getline(&line, &len, stdin)) == -1)
    {
        if (feof(stdin))
        {
            exit(0); // We received an EOF
        }
        else
        {
            perror("readline");
            exit(1);
        }
    }
    printf("%s", line);
    return line;
}

// takes splits it into tokens int the double linked list


struct token *list_tokens(struct token *head, char *line) {
    char *token_str = strtok(line, " \t\n,"); // delimiters: space, tab, newline, comma
    struct token *current = NULL;
    struct token *tail = NULL;

    while (token_str != NULL) {
        // Create a new node
        struct token *new_node = malloc(sizeof(struct token));
        if (!new_node) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }

        new_node->value = strdup(token_str);
        new_node->next = NULL;
        new_node->prev = tail;

        if (tail != NULL)
            tail->next = new_node;
        else
            head = new_node;  // first node

        tail = new_node;

        // Move to next token
        token_str = strtok(NULL, " \t\n,");
    }

    return head;
}

void print_tokens(struct token *head) {
    struct token *current = head;
    int i = 0;

    while (current != NULL) {
        printf("Token %d: %s\n", i, current->value);
        current = current->next;
        i++;
    }
}

// main function




 int main(int ac, char **av)
{

    char *username;
    const char *registration_number = "csd4558";
    struct passwd *pw = getpwuid(getuid());
    username = pw ? pw->pw_name : "unknown";
    if (pw == NULL)
    {
        perror("getpwuid");
        return 1;
    }
    char *line;
    while (1)
    {

        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s@-%s-hy345sh:%s $ ", username, registration_number, cwd);
        }
        else
        {
            perror("getcwd() error");
        }

        line = cell_read_line();
        print_tokens(list_tokens(NULL, line));
        free(line);
    }



    return 0;
}



