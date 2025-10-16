
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
    if (!line)
        return NULL;

    struct token *tail = NULL;
    head = NULL;

    size_t len = strlen(line);
    char temp_string[PATH_MAX]; // temporary buffer
    int temp_index = 0;

    for (size_t i = 0; i <= len; i++)
    {
        char c = line[i];

        // Check for operators or end of line
        if ((c == ';') || (c == '&' && line[i + 1] == '&') || (c == '|' && line[i + 1] == '|') || c == '\0')
        {
            if (temp_index > 0)
            {
                temp_string[temp_index] = '\0';

                // create new node
                struct token *new_node = malloc(sizeof(struct token));
                if (!new_node)
                {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }

                new_node->value = strdup(temp_string);
                new_node->operator_id = 0; // default
                new_node->next = NULL;
                new_node->prev = tail;

                if (tail)
                    tail->next = new_node;
                else
                    head = new_node;

                tail = new_node;
                temp_index = 0;
            }

            // assign operator_id if operator found
            if (c == ';' && tail)
            {
                tail->operator_id = 1;
            }
            else if (c == '&' && line[i + 1] == '&' && tail)
            {
                tail->operator_id = 2;
                i++; // skip second &
            }
            else if (c == '|' && line[i + 1] == '|' && tail)
            {
                tail->operator_id = 3;
                i++; // skip second |
            }

            continue;
        }

        // Skip whitespace outside tokens
        if (c == ' ' || c == '\t' || c == '\n')
            continue;

        // Normal character → add to temp_string
        temp_string[temp_index++] = c;
    }

    return head;
}
// print the linked list of tokens
void print_token_list(struct token *head)
{
    struct token *current = head;
    while (current != NULL)
    {
        printf("Token: '%s', Operator ID: %d\n", current->value, current->operator_id);
        current = current->next;
    }
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
    struct token *current = head;
    int i = 0;

    char **array = malloc((count + 1) * sizeof(char *));
    if (!array)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

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
    char *username;
    const char *registration_number = "csd4558";
    struct passwd *pw = getpwuid(getuid());

    if (pw == NULL)
    {
        perror("getpwuid");
        username = "unknown";
    }
    else
    {
        username = pw->pw_name;
    }

    while (1)
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("getcwd() error");
            strcpy(cwd, "unknown");
        }

        printf("%s@-%s-hy345sh:%s $ ", username, registration_number, cwd);
        fflush(stdout);

        char *line = cell_read_line();
        token = list_tokens(NULL, line);
        print_token_list(token);

        free(line);

        if (!token)
            continue;

        int count = count_tokens(token);
        char **argv = token_array(token, count);

        exec_command(token, argv, count);

        free_array(argv);
        free_tokens(token);
    }

    return 0;
}

void exec_command(struct token *node, char **array, int count)
{
    if (!node)
        return;

    pid_t pid;
    int status;

    // Build argv for current command (always include current node)
    struct token *current = node;
    int argc = 0;

    // Count how many contiguous tokens (usually 1 unless your tokenizer splits arguments later)
    while (current && current->operator_id == 0)
    {
        argc++;
        current = current->next;
    }

    // If current command has an operator, still execute it
    if (argc == 0)
        argc = 1;

    // Build argv for this command
    char **argv = malloc((argc + 1) * sizeof(char *));
    if (!argv)
    {
        perror("malloc");
        return;
    }

    current = node;
    for (int i = 0; i < argc; i++)
    {
        argv[i] = strdup(current->value);
        if (!argv[i])
        {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        current = current->next;
    }
    argv[argc] = NULL;

    // Execute this command
    pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
    }
    else if (pid == 0)
    {
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        waitpid(pid, &status, 0);
    }

    // Free temporary argv
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);

    // Handle operator recursion
    struct token *next = node->next; // NOT current — node->next is the next command
    int op = node->operator_id;

    if (op == 1) // ;
    {
        exec_command(next, array, count);
    }
    else if (op == 2) // &&
    {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            exec_command(next, array, count);
    }
    else if (op == 3) // ||
    {
        if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0))
            exec_command(next, array, count);
    }
}
