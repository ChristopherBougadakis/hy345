
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
    if (!line) return NULL;

    struct token *tail = NULL;
    head = NULL;

    size_t len = strlen(line);
    char temp_string[PATH_MAX]; // temporary buffer
    int temp_index = 0;

    for (size_t i = 0; i <= len; i++) {
        char c = line[i];

        // Check for operators or end of line
        if ((c == ';') || (c == '&' && line[i + 1] == '&') || (c == '|' && line[i + 1] == '|') || c == '\0') {
            if (temp_index > 0) {
                temp_string[temp_index] = '\0';
                
                // create new node
                struct token *new_node = malloc(sizeof(struct token));
                if (!new_node) { perror("malloc"); exit(EXIT_FAILURE); }

                new_node->value = strdup(temp_string);
                new_node->operator_id = 0; // default
                new_node->next = NULL;
                new_node->prev = tail;

                if (tail) tail->next = new_node;
                else head = new_node;

                tail = new_node;
                temp_index = 0;
            }

            // assign operator_id if operator found
            if (c == ';' && tail) {
                tail->operator_id = 1;
            } else if (c == '&' && line[i + 1] == '&' && tail) {
                tail->operator_id = 2;
                i++; // skip second &
            } else if (c == '|' && line[i + 1] == '|' && tail) {
                tail->operator_id = 3;
                i++; // skip second |
            }

            continue;
        }

        // Skip whitespace outside tokens
        if (c == ' ' || c == '\t' || c == '\n') continue;

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
{   struct token *current = head;
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
        print_token_list( token );
        free(line);
         // Debugging: print the token list

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