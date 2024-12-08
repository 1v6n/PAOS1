/**
 * @file main.c
 * @brief Entry point of the system. This file contains the main functionality for starting Prometheus and Grafana,
 * initializing and exposing system metrics.
 * @author 1v6n
 * @date 09/10/2024
 */

#include "expose_metrics.h"
#include "metrics.h"
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/monitor_fifo"
#define BUFFER_SIZE 256
#define MAX_METRICS 10
#define STATUS_FILE "/tmp/monitor_status"

/**
 * @brief Trims leading and trailing whitespace from a string.
 *
 * This function removes any leading and trailing whitespace characters
 * from the input string. It modifies the original string and returns
 * a pointer to the trimmed string.
 *
 * @param str The input string to be trimmed. It must be a null-terminated string.
 * @return A pointer to the trimmed string. If the input string is empty or contains
 *         only whitespace characters, a pointer to the null character is returned.
 */
char* trim_whitespace(char* str)
{
    char* end;

    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return str;
}

/**
 * @brief Updates the status by writing it to a file.
 *
 * This function opens the file specified by STATUS_FILE in write mode,
 * writes the provided status string to the file, and then closes the file.
 * If the file cannot be opened, an error message is printed to stderr.
 *
 * @param status The status string to be written to the file.
 */
void update_status(const char* status)
{
    FILE* file = fopen(STATUS_FILE, "w");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }
    fprintf(file, "%s\n", status);
    fclose(file);
}

/**
 * @brief Get the home directory of the current user.
 *
 * This function retrieves the home directory of the user running the program by using the `getpwuid()` function.
 *
 * @return A pointer to a string representing the home directory path of the current user. Returns `NULL` on failure.
 */
char* get_home_directory(void)
{
    struct passwd* pw = getpwuid(getuid());
    return pw ? pw->pw_dir : NULL;
}

/**
 * @brief Start the Grafana server.
 *
 * Constructs the command to launch the Grafana server with the appropriate configuration and home path.
 * The server is started as a background process using the `system()` function.
 */
void start_grafana(void)
{
    char* home_dir = get_home_directory();
    if (home_dir == NULL)
    {
        fprintf(stderr, "Failed to retrieve home directory\n");
        return;
    }

    char command[COMMAND_SIZE];
    snprintf(command, sizeof(command),
             "%s/grafana/bin/grafana server --config %s/grafana/conf/defaults.ini --homepath %s/grafana &", home_dir,
             home_dir, home_dir);

    int ret = system(command);
    if (ret == RETURN_ERROR)
    {
        fprintf(stderr, "Failed to start Grafana\n");
    }
    else
    {
        printf("Grafana started successfully\n");
    }
}

/**
 * @brief Starts the Prometheus server.
 *
 * This function uses the `system()` call to start the Prometheus server in the background.
 */
void start_prometheus(void)
{
    const char* home_dir = getenv("HOME");
    if (home_dir == NULL)
    {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return;
    }

    char prometheus_config_path[COMMAND_SIZE];
    snprintf(prometheus_config_path, sizeof(prometheus_config_path), "%s/prometheus/prometheus.yml", home_dir);

    char command[COMMAND_SIZE];
    snprintf(command, sizeof(command), "%s/prometheus/prometheus --config.file=%s &", home_dir, prometheus_config_path);
    system(command);
}

/**
 * @brief Creates a new thread to expose metrics via an HTTP server.
 *
 * This function creates a new thread using pthread_create to run the
 * expose_metrics function. If the thread creation fails, an error message
 * is printed to stderr.
 */
void create_threads(void)
{
    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0)
    {
        fprintf(stderr, "Error creating HTTP server thread\n");
        update_status("Error creating HTTP server thread");
        return EXIT_FAILURE;
    }
}

/**
 * @brief Starts the metrics monitoring process.
 *
 * This function initializes the metrics monitoring by setting up the necessary
 * update functions for each selected metric and creating the required threads.
 * It then enters an infinite loop where it periodically calls the update functions
 * for each metric.
 *
 * @param selected_metrics An array of strings representing the names of the metrics to monitor.
 * @param num_metrics The number of metrics to monitor.
 */
void start_metrics_monitoring(const char* selected_metrics[], size_t num_metrics)
{
    init_metrics(selected_metrics, num_metrics);

    create_threads();

    void (*update_functions[num_metrics])(void);

    for (size_t i = 0; i < num_metrics; i++)
    {
        const char* metric_name = selected_metrics[i];
        update_functions[i] = NULL;

        printf("Processing metric: '%s'\n", metric_name);

        for (MetricInfo* info = all_metrics; info->name != NULL; info++)
        {

            if (strcmp(metric_name, info->name) == 0)
            {
                update_functions[i] = info->update_function;
                break;
            }
        }
        if (update_functions[i] == NULL)
        {
            char status_message[BUFFER_SIZE];
            snprintf(status_message, sizeof(status_message), "Error: No update function found for metric '%s'",
                     metric_name);
            update_status(status_message);
            fprintf(stderr, "%s\n", status_message);
            fprintf(stderr, "Error: No update function found for metric '%s'\n", metric_name);
            return;
        }
    }

    update_status("Metrics monitoring started");

    while (true)
    {
        for (size_t i = 0; i < num_metrics; i++)
        {
            if (update_functions[i] != NULL)
            {
                update_functions[i]();
            }
            else
            {
                fprintf(stderr, "Error: update_functions[%zu] is NULL\n", i);
            }
        }
        sleep(SLEEP_TIME);
    }
}

/**
 * @brief Parses a comma-separated string into an array of metric names.
 *
 * @param input The input string containing metric names separated by commas.
 * @param output Array to store parsed metric names.
 * @param max_metrics Maximum number of metrics to parse.
 * @return The number of parsed metrics.
 */
size_t parse_metrics(const char* input, const char* output[], size_t max_metrics)
{
    size_t count = 0;
    char* token;
    char* input_copy = strdup(input);

    token = strtok(input_copy, ",");
    while (token != NULL && count < max_metrics)
    {
        char* trimmed_token = strdup(token);
        output[count++] = strdup(trim_whitespace(trimmed_token)); // Store each trimmed metric name
        free(trimmed_token);
        token = strtok(NULL, ",");
    }

    free(input_copy);
    return count;
}

/**
 * @brief Reads metrics from a FIFO and starts monitoring.
 *
 * This function opens and reads from a FIFO. The read data is parsed into
 * an array of metric names and used to start metric monitoring.
 */
void start_monitoring_from_fifo(void)
{
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST)
    {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    FILE* fifo_file = fopen(FIFO_PATH, "r");
    if (fifo_file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead = fread(buffer, sizeof(char), sizeof(buffer) - 1, fifo_file);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';

        const char* selected_metrics[MAX_METRICS];
        size_t num_metrics = parse_metrics(buffer, selected_metrics, MAX_METRICS);

        if (num_metrics > 0 && strcmp(selected_metrics[0], "1") == 0)
        {
            show_available_metrics();
            for (size_t i = 0; i < num_metrics; i++)
            {
                free((char*)selected_metrics[i]);
            }
            fclose(fifo_file);
            unlink(FIFO_PATH);
            return;
        }

        if (num_metrics > 0)
        {
            start_metrics_monitoring(selected_metrics, num_metrics);
        }
        for (size_t i = 0; i < num_metrics; i++)
        {
            free((char*)selected_metrics[i]);
        }
    }
    else
    {
        perror("fread");
    }

    fclose(fifo_file);
    unlink(FIFO_PATH);
}

/**
 * @brief Runs an executable with the given command.
 *
 * This function uses the `system()` call to run the specified command.
 * It prints an error message if the command fails to execute.
 *
 * @param command The command to run the executable.
 * @return Returns 0 on success, or -1 on failure.
 */
int run_executable(const char* command)
{
    int ret = system(command);
    if (ret == -1)
    {
        perror("system");
        return -1;
    }
    else if (ret != 0)
    {
        fprintf(stderr, "Command failed with exit code %d\n", WEXITSTATUS(ret));
        return -1;
    }
    return 0;
}

/**
 * @brief Main function of the system.
 *
 * The main function is responsible for starting both Prometheus and Grafana servers.
 * It also initializes system metrics and creates a thread to expose these metrics via an HTTP server.
 * The metrics are continuously updated every second in an infinite loop.
 *
 * @param argc The argument count.
 * @param argv The argument vector.
 * @return Returns `EXIT_SUCCESS` on successful execution or `EXIT_FAILURE` in case of errors.
 */
int main(int argc, char* argv[])
{
    start_grafana();
    tart_prometheus();
    show_available_metrics();
    update_status("Starting monitoring from FIFO");
    start_monitoring_from_fifo();
    return EXIT_SUCCESS;
}
