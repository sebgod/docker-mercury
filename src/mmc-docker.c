#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define STRINGIFY2(s) #s
#define STRINGIFY(s) STRINGIFY2((s))

#define AT(str) __FILE__ ":" STRINGIFY(__LINE) ": " STRINGIFY((str))

#define COND_ERR_EXIT1(cond, fmt, param) do { \
    if ((cond)) { \
        fprintf(stderr, AT(fmt), (param)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)
#define COND_PERROR_RET(cond, prefix) do { \
    if ((cond)) { \
        perror(AT((prefix))); \
        return EXIT_FAILURE; \
    } \
} while (0)

#define CMD_LINE_LEN (2048 - 1)
#define ADD(str) do { buf = append(buf, (str), &limit); } while (0)
#define COMPLETE do { *buf = '\0'; } while (0)

char *
append(char *str, char *end, int *limit)
{
    int len = strlen(end);
    COND_ERR_EXIT1(len >= *limit, "command line limit of %d reached",
        CMD_LINE_LEN);
    
    strncpy(str, end, len);
    return str + len;
}

#ifdef _WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

void
unix_path(char *val, char *def)
{
    int i, len;

    if (!val) {
        val = def;
    }
    len = val ? strlen(val) : 0;
    COND_ERR_EXIT1(len <= 0, "%s", "value is empty");
    
    if (val[0] != '/') {
        /* e.g. c:\ -> /c/ */
        val[1] = tolower(val[0]);
        val[0] = '/';
        for (i = 2; i < len; i++) {
            if (val[i] == '\\') {
                val[i] = '/';
            }
        }
    }
}

int
main(int argc, char *argv[])
{
    char cmd[CMD_LINE_LEN]; // command line length limit in WinXP
    char *buf = cmd;
    char *env_docker, *env_temp;
    char pwd[FILENAME_MAX] = {0};
    char tmp[FILENAME_MAX] = {0};
    int limit = CMD_LINE_LEN;
    int i, len;

    env_temp = getenv("MERCURY_TMP");
    if (!env_temp) {
        env_temp = getenv("TEMP");
    }
    if (!env_temp) {
        env_temp = "/tmp";
    }
    len = strlen(env_temp);
    len = MIN(len, sizeof(tmp));
    strncpy(tmp, env_temp, len);

    COND_PERROR_RET(!GetCurrentDir(pwd, sizeof(pwd)),
            "cannot obtain current working directory");
    tmp[sizeof(tmp) - 1] = '\0';
    pwd[sizeof(pwd) - 1] = '\0';

    unix_path(pwd, NULL);
    unix_path(tmp, "/tmp");

    env_docker = getenv("MERCURY_DOCKER");
    if (!env_docker) {
        env_docker = "sebgod/mercury-stable:latest";
    }
    
    ADD("docker run -it --read-only=true");
#ifndef _WIN32
    ADD(" -u `id -u`");
#endif
    ADD(" -v "); ADD(tmp); ADD(":/tmp:rw");
    ADD(" -v "); ADD(pwd); ADD(":/var/tmp/mercury:rw");
    ADD(" "); ADD(env_docker);
#ifdef _WIN32
    /* disable symlinks since they do not work properly on
     * Windows by default */
    ADD(" --no-use-symlinks");
#endif
    /* as the shared libraries are within the container,
     * disable shared linkage for now.
     * TODO: use a volume for the Mercury libraries */
    ADD(" --linkage static");
    for (i = 1; i < argc; i++) {
        ADD(" "); ADD(argv[i]);
    }
    COMPLETE;
    
    system(cmd);
    return EXIT_SUCCESS;
}
