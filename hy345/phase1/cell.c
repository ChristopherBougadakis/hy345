
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

    return line;
}

// takes splits it into tokens int the double linked list

struct token *list_tokens(struct token *head, char *line)
{
    char *token_str = strtok(line, " \t\n,"); // delimiters: space, tab, newline, comma

    struct token *tail = NULL;

    while (token_str != NULL)
    {
        // Create a new node
        struct token *new_node = malloc(sizeof(struct token));
        if (!new_node)
        {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }

        new_node->value = strdup(token_str);
        new_node->next = NULL;
        new_node->prev = tail;

        if (tail != NULL)
            tail->next = new_node;
        else
            head = new_node; // first node

        tail = new_node;

        // Move to next token
        token_str = strtok(NULL, " \t\n,");
    }

    return head;
}

// count number of tokens in the linked list
int count_tokens(struct token *head)
{
    struct token *current = head;
    int count = 0;

    while (current != NULL)
    {
        count++;
        current = current->next;
    }

    return count;
}

// convert linked list to array of strings
char **token_array(struct token *head, int count)
{
    char **array = malloc((count + 1) * sizeof(char *));
    if (!array)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    struct token *current = head;
    int i = 0;

    while (current != NULL)
    {
        array[i] = strdup(current->value); // Create a copy
        if (!array[i])
        {
            perror("strdup failed");
            exit(EXIT_FAILURE);
        }
        current = current->next;
        i++;
    }

    array[i] = NULL;
    return array;
}

void free_tokens(struct token *head)
{
    struct token *tmp;
    while (head)
    {
        tmp = head;
        head = head->next;
        free(tmp->value); // Free the strdup'd string
        free(tmp);        // Free the node itself
    }
}
void free_array(char **array)
{
    for (int i = 0; array[i] != NULL; i++)
    {
        free(array[i]);
    }
    free(array);
}
// main function

int main(int ac, char **av)
{
    struct token *token;
    __pid_t pid;
    char *username;
    const char *registration_number = "csd4558";
    struct passwd *pw = getpwuid(getuid());

    if (pw == NULL)
    {
        perror("getpwuid");
        username = "unknown"; // Don't exit, just use "unknown"
    }
    else
    {
        username = pw->pw_name;
    }

    char *line;
    while (1)
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("getcwd() error");
            strcpy(cwd, "unknown"); // Use a fallback
        }

        printf("%s@-%s-hy345sh:%s $ ", username, registration_number, cwd);
        fflush(stdout); // Force output

        line = cell_read_line();
        token = list_tokens(NULL, line);
        free(line);

        if (!token)
            continue;

        int count = count_tokens(token);
        char **argv = token_array(token, count);

        pid = fork();
        if (pid < 0)
        {
            perror("Fork failed");
            free(argv);
            free_tokens(token);
            continue; // Don't exit, just continue
        }
        else if (pid == 0)
        {
            if (execvp(argv[0], argv) == -1)
            {
                fprintf(stderr, "Command not found: %s\n", argv[0]);
                exit(1);
            }
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }

        free(argv);
        free_tokens(token);
    }
    return 0;
}