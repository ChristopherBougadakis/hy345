
#include "cell.h"

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
    }

    return 0;
}