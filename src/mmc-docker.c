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
#define INIT(store) char *_temp_buf = (store); \
    int _temp_limit = sizeof((store))
#define ADD(str) \
    do { _temp_buf = append(_temp_buf, (str), &_temp_limit); } while (0)
#define COMPLETE do { *_temp_buf = '\0'; } while (0)
#define SECURE(x) do { (x)[sizeof((x)) - 1] = '\0'; } while (0)

char *
get_env_or_default(char *name, char *def)
{
    char *value;
    return ((value = getenv(name)) ? value : def);
}

char *
get_env_or_default2(char *name, char *def, int *has_value)
{
    char *value;
    *has_value = (value = getenv(name)) != NULL;
    return has_value ? value : def;
}

int
env_is_true(char *value)
{
    return !strcmp(value, "1");
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

#define DEFAULT_EXE "docker"
#define DEFAULT_PREFIX "sebgod/mercury-"
#define DEFAULT_CHANNEL "stable"
#define DEFAULT_VERSION "latest"
#define DEFAULT_CROSS_ARCH "i686-w64-mingw32"
#ifdef _WIN32
    #define DEFAULT_CROSS "1"
#else
    #define DEFAULT_CROSS "0"
#endif

int
main(int argc, char *argv[])
{
    char cmd[CMD_LINE_LEN];
    INIT(cmd);
    char *env_tmp;
    char *env_docker_exe;
    char *env_docker_interactive;
    char *env_docker_prefix;
    char *env_docker_channel;
    char *env_docker_version;
    char *env_docker_cross;
    char *env_docker_cross_arch;
    char path_pwd[FILENAME_MAX] = {0};
    char path_tmp[FILENAME_MAX] = {0};
    int is_interactive, is_cross;

    env_docker_interactive =
        get_env_or_default("MERCURY_DOCKER_INTERACTIVE", "0");
    is_interactive = env_is_true(env_docker_interactive);

    {
        int len;
        env_tmp = get_env_or_default("MERCURY_TMP",
                get_env_or_default("TMP", "/tmp"));
        len = strlen(env_tmp);
        len = MIN(len, sizeof(path_tmp) - 1);
        strncpy(path_tmp, env_tmp, len);
        unix_path(path_tmp);
    }

    {
        COND_PERROR_RET(!GetCurrentDir(path_pwd, sizeof(path_pwd)),
                "cannot obtain current working directory");
        unix_path(path_pwd);
        SECURE(path_pwd);
    }

    env_docker_exe =
        get_env_or_default("MERCURY_DOCKER_EXE", DEFAULT_EXE);
    env_docker_prefix =
        get_env_or_default("MERCURY_DOCKER_PREFIX", DEFAULT_PREFIX);
    env_docker_channel =
        get_env_or_default("MERCURY_DOCKER_CHANNEL", DEFAULT_CHANNEL);
    env_docker_version =
        get_env_or_default("MERCURY_DOCKER_VERSION", DEFAULT_VERSION);
    env_docker_cross =
        get_env_or_default("MERCURY_DOCKER_CROSS", DEFAULT_CROSS);
    env_docker_cross_arch =
        get_env_or_default2("MERCURY_DOCKER_CROSS_ARCH", DEFAULT_CROSS_ARCH,
                &is_cross);
    is_cross = !is_interactive && (is_cross || env_is_true(env_docker_cross));

    ADD(env_docker_exe);
    ADD(" run -i --read-only=true");
    if (is_interactive) {
        ADD(" --entrypoint bash");
    }
#ifndef _WIN32
    ADD(" -u `id -u`");
#endif
    {
        /* mount volumes */
        ADD(" -v "); ADD(path_tmp); ADD(":/tmp:rw");
        ADD(" -v "); ADD(path_pwd); ADD(":/var/tmp/mercury:rw");
    }

    {
        /* composing the repository reference */
        ADD(" ");
        ADD(env_docker_prefix);
        ADD(env_docker_channel);
        if (is_cross) {
            ADD("-cross");
        }
        ADD(":"); ADD(env_docker_version);
        if (is_cross) {
            ADD("-"); ADD(env_docker_cross_arch);
        }
    }
    if (is_interactive) {
        ADD(" -i");
    } else {
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

        {
            int i;
            for (i = 1; i < argc; i++) {
                ADD(" "); ADD(argv[i]);
            }
        }
    }
    COMPLETE;
    
    SECURE(cmd);
    system(cmd);
    return EXIT_SUCCESS;
}
