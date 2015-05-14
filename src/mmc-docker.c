/* vim: ft=c tw=78 ts=4 ff=unix sw=4 et
 * author: Sebastian Godelet <sebastian.godelet@outlook.com>
 *
 * mmc-docker enables consistent cross-platform (Windows and Linux) behaviour
 * for executing a Docker image containing the Mercury platform.
 *
 * Tested on:
 *  - a normal Linux bash shell
 *  - a Windows cmd.exe shell
 *  - a Windows MSys 32-bit bash shell
 */
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

/* command line length limit in WinXP (should be the smallest) */
#define CMD_LINE_LEN (2048 - 1)

/* complex string manipulation using weak pointers */
#define INIT(store) char *_temp_buf = (store)
#define ADD(str) do { _temp_buf = append(_temp_buf, (str), &limit); } while (0)
#define COMPLETE do { *_temp_buf = '\0'; } while (0)

char *
get_env_or_default(char *name, char *def)
{
    char *value;
    return ((value = getenv(name)) ? value : def);
}

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
unix_path(char *val)
{
    int len = val ? strlen(val) : 0;

    COND_ERR_EXIT1(len <= 0, "%s", "value is empty");
    
    if (val[0] != '/') {
        int i;
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

#define DEFAULT_PREFIX "sebgod/mercury-"
#define DEFAULT_VERSION "latest"
#ifdef _WIN32
    #define DEFAULT_CHANNEL "stable-cross"
    #define DEFAULT_SUFFIX  "-i686-w64-mingw32"
#else
    #define DEFAULT_CHANNEL "stable"
    #define DEFAULT_SUFFIX  ""
#endif

int
main(int argc, char *argv[])
{
    char cmd[CMD_LINE_LEN];
    INIT(cmd);
    char *env_docker_prefix;
    char *env_docker_channel;
    char *env_docker_version;
    char *env_docker_suffix;
    char *env_temp;
    char pwd[FILENAME_MAX] = {0};
    char tmp[FILENAME_MAX] = {0};
    int limit = CMD_LINE_LEN;
    int i, len;

    env_temp = get_env_or_default("MERCURY_TMP",
            get_env_or_default("TEMP", "/tmp"));
    len = strlen(env_temp);
    len = MIN(len, sizeof(tmp));
    strncpy(tmp, env_temp, len);

    COND_PERROR_RET(!GetCurrentDir(pwd, sizeof(pwd)),
            "cannot obtain current working directory");
    tmp[sizeof(tmp) - 1] = '\0';
    pwd[sizeof(pwd) - 1] = '\0';

    unix_path(pwd);
    unix_path(tmp);

    env_docker_prefix =
        get_env_or_default("MERCURY_DOCKER_PREFIX", DEFAULT_PREFIX);
    env_docker_channel =
        get_env_or_default("MERCURY_DOCKER_CHANNEL", DEFAULT_CHANNEL);
    env_docker_version =
        get_env_or_default("MERCURY_DOCKER_VERSION", DEFAULT_VERSION);
    env_docker_suffix =
        get_env_or_default("MERCURY_DOCKER_SUFFIX", DEFAULT_SUFFIX);
    
    ADD("docker run -it --read-only=true");
#ifndef _WIN32
    ADD(" -u `id -u`");
#endif
    ADD(" -v "); ADD(tmp); ADD(":/tmp:rw");
    ADD(" -v "); ADD(pwd); ADD(":/var/tmp/mercury:rw");
    /* composing the repository reference */
    ADD(" ");
    ADD(env_docker_prefix);
    ADD(env_docker_channel); ADD(":"); ADD(env_docker_version);
    ADD(env_docker_suffix);
#ifdef _WIN32
    /* disable symlinks since they do not work properly on
     * Windows by default */
    ADD(" --no-use-symlinks");
#endif
    /* since we might test out different versions + bitness, use
     * --use-grade-subdirs by default */
    ADD(" --use-grade-subdirs");
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
