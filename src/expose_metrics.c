/**
 * @file expose_metrics.c
 * @brief Functions for initializing and exposing system metrics through Prometheus.
 *
 * This file contains the core functionality to initialize and expose various system metrics such as CPU usage,
 * memory usage, disk stats, and network traffic via Prometheus gauges. These metrics are collected from the
 * system and exposed through an HTTP server for monitoring purposes.
 *
 * The metrics include usage statistics for CPU, memory, disk, network, battery, and more,
 * and are safely updated in a multi-threaded environment using mutexes.
 *
 * @author 1v6n
 * @date 09/10/2024
 */

#include "expose_metrics.h"
#define METRICS_FILE "/tmp/monitor_metrics"

bool keep_running = true; /**< Control variable for the main loop. */
pthread_mutex_t lock;     /**< Mutex for thread synchronization. */

static prom_gauge_t* cpu_usage_metric;         /**< Prometheus gauge for tracking CPU usage. */
static prom_gauge_t* memory_usage_metric;      /**< Prometheus gauge for tracking memory usage. */
static prom_gauge_t* disk_usage_metric;        /**< Prometheus gauge for tracking disk usage. */
static prom_gauge_t* running_processes_metric; /**< Prometheus gauge for tracking the number of running processes. */
static prom_gauge_t* cpu_temp_metric;          /**< Prometheus gauge for tracking CPU temperature. */
static prom_gauge_t* battery_voltage_metric;   /**< Prometheus gauge for tracking battery voltage. */
static prom_gauge_t* battery_current_metric;   /**< Prometheus gauge for tracking battery current. */
static prom_gauge_t* cpu_frequency_metric;     /**< Prometheus gauge for tracking CPU frequency. */
static prom_gauge_t* cpu_fan_speed_metric;     /**< Prometheus gauge for tracking CPU fan speed. */
static prom_gauge_t* gpu_fan_speed_metric;     /**< Prometheus gauge for tracking GPU fan speed. */
static prom_gauge_t* total_processes_metric;   /**< Prometheus gauge for tracking the total number of processes. */
static prom_gauge_t*
    suspended_processes_metric;                /**< Prometheus gauge for tracking the number of suspended processes. */
static prom_gauge_t* ready_processes_metric;   /**< Prometheus gauge for tracking the number of ready processes. */
static prom_gauge_t* blocked_processes_metric; /**< Prometheus gauge for tracking the number of blocked processes. */
static prom_gauge_t* total_memory_metric;      /**< Prometheus gauge for tracking the total memory in the system. */
static prom_gauge_t* used_memory_metric;       /**< Prometheus gauge for tracking the used memory in the system. */
static prom_gauge_t* available_memory_metric;  /**< Prometheus gauge for tracking the available memory in the system. */
static prom_gauge_t* context_switches_metric;  /**< Prometheus gauge for tracking the number of context switches. */
static prom_gauge_t* io_time_metric;           /**< Prometheus gauge for tracking the time spent on I/O operations. */
static prom_gauge_t*
    writes_completed_metric;                 /**< Prometheus gauge for tracking the total number of writes completed. */
static prom_gauge_t* reads_completed_metric; /**< Prometheus gauge for tracking the total number of reads completed. */
static prom_gauge_t* rx_bytes_metric;  /**< Prometheus gauge for tracking the total received bytes in the network. */
static prom_gauge_t* tx_bytes_metric;  /**< Prometheus gauge for tracking the total transmitted bytes in the network. */
static prom_gauge_t* rx_errors_metric; /**< Prometheus gauge for tracking the total receive errors in the network. */
static prom_gauge_t* tx_errors_metric; /**< Prometheus gauge for tracking the total transmit errors in the network. */
static prom_gauge_t* dropped_packets_metric; /**< Prometheus gauge for tracking the total number of dropped packets. */

MetricInfo all_metrics[] = {
    {"rx_bytes_total", "Total received bytes", &rx_bytes_metric, &update_network_traffic_metric},
    {"tx_bytes_total", "Total transmitted bytes", &tx_bytes_metric, &update_network_traffic_metric},
    {"rx_errors_total", "Total receive errors", &rx_errors_metric, &update_network_traffic_metric},
    {"tx_errors_total", "Total transmit errors", &tx_errors_metric, &update_network_traffic_metric},
    {"dropped_packets_total", "Total dropped packets", &dropped_packets_metric, &update_network_traffic_metric},
    {"io_time_ms", "Time spent on I/O in milliseconds", &io_time_metric, &update_disk_stats_metrics},
    {"writes_completed_total", "Total writes completed", &writes_completed_metric, &update_disk_stats_metrics},
    {"reads_completed_total", "Total reads completed", &reads_completed_metric, &update_disk_stats_metrics},
    {"total_memory_mb", "Total memory in MB", &total_memory_metric, &update_memory_metrics},
    {"used_memory_mb", "Used memory in MB", &used_memory_metric, &update_memory_metrics},
    {"available_memory_mb", "Available memory in MB", &available_memory_metric, &update_memory_metrics},
    {"context_switches", "Context switches", &context_switches_metric, &update_context_switches_metric},
    {"cpu_usage_percentage", "CPU usage in percentage", &cpu_usage_metric, &update_cpu_gauge},
    {"memory_usage_percentage", "Memory usage in percentage", &memory_usage_metric, &update_memory_gauge},
    {"disk_usage_percentage", "Disk usage in percentage", &disk_usage_metric, &update_disk_gauge},
    {"running_processes_total", "Total running processes", &running_processes_metric, &update_running_processes_gauge},
    {"cpu_temperature_celsius", "CPU temperature in Celsius", &cpu_temp_metric, &update_cpu_temperature},
    {"battery_voltage_volts", "Battery voltage in volts", &battery_voltage_metric, &update_battery_voltage},
    {"battery_current_amperes", "Battery current in amperes", &battery_current_metric, &update_battery_current},
    {"cpu_frequency_megahertz", "CPU frequency in MHz", &cpu_frequency_metric, &update_cpu_frequency},
    {"cpu_fan_speed_rpm", "CPU fan speed in RPM", &cpu_fan_speed_metric, &update_cpu_fan_speed},
    {"gpu_fan_speed_rpm", "GPU fan speed in RPM", &gpu_fan_speed_metric, &update_gpu_fan_speed},
    {"total_processes", "Total number of processes", &total_processes_metric, &update_process_states_gauge},
    {"suspended_processes", "Suspended processes", &suspended_processes_metric, &update_process_states_gauge},
    {"ready_processes", "Ready processes", &ready_processes_metric, &update_process_states_gauge},
    {"blocked_processes", "Blocked processes", &blocked_processes_metric, &update_process_states_gauge},
    {NULL, NULL, NULL} // Sentinel value to mark the end of the array
};
void update_gauge(prom_gauge_t* metric, double value)
{
    pthread_mutex_lock(&lock);
    prom_gauge_set(metric, value, NULL);
    pthread_mutex_unlock(&lock);
}

void update_cpu_gauge(void)
{
    double usage = get_cpu_usage();
    if (usage >= 0)
    {
        update_gauge(cpu_usage_metric, usage);
    }
    else
    {
        fprintf(stderr, "Error obtaining CPU usage\n");
    }
}

void update_memory_gauge(void)
{
    double usage = get_memory_usage();
    if (usage >= 0)
    {
        update_gauge(memory_usage_metric, usage);
    }
    else
    {
        fprintf(stderr, "Error obtaining memory usage\n");
    }
}

void update_disk_gauge(void)
{
    double usage = get_disk_usage();
    if (usage >= 0)
    {
        update_gauge(disk_usage_metric, usage);
    }
    else
    {
        fprintf(stderr, "Error obtaining disk usage\n");
    }
}

void update_running_processes_gauge(void)
{
    FILE* fp = fopen(PROC_STAT_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/stat");
        return;
    }

    char buffer[BUFFER_SIZE];
    unsigned long running_processes = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "procs_running %lu", &running_processes) == 1)
        {
            break;
        }
    }
    fclose(fp);

    update_gauge(running_processes_metric, (double)running_processes);
}

void update_process_states_gauge(void)
{
    int total, suspended, ready, blocked;
    get_process_states(&total, &suspended, &ready, &blocked);

    pthread_mutex_lock(&lock);
    prom_gauge_set(total_processes_metric, total, NULL);
    prom_gauge_set(suspended_processes_metric, suspended, NULL);
    prom_gauge_set(ready_processes_metric, ready, NULL);
    prom_gauge_set(blocked_processes_metric, blocked, NULL);
    pthread_mutex_unlock(&lock);
}

void update_cpu_temperature(void)
{
    double cpu_temp = get_cpu_temperature();
    if (cpu_temp >= 0)
    {
        update_gauge(cpu_temp_metric, cpu_temp);
    }
    else
    {
        fprintf(stderr, "Error getting CPU temperature\n");
    }
}

void update_battery_voltage(void)
{
    double voltage = get_battery_voltage();
    if (voltage >= 0)
    {
        update_gauge(battery_voltage_metric, voltage);
    }
    else
    {
        fprintf(stderr, "Error getting battery voltage\n");
    }
}

void update_battery_current(void)
{
    double current = get_battery_current();
    if (current >= 0)
    {
        update_gauge(battery_current_metric, current);
    }
    else
    {
        fprintf(stderr, "Error getting battery current\n");
    }
}

void update_cpu_frequency(void)
{
    double freq = get_cpu_frequency();
    if (freq >= 0)
    {
        update_gauge(cpu_frequency_metric, freq);
    }
    else
    {
        fprintf(stderr, "Error getting CPU frequency\n");
    }
}

void update_cpu_fan_speed(void)
{
    double speed = get_cpu_fan_speed();
    if (speed >= 0)
    {
        update_gauge(cpu_fan_speed_metric, speed);
    }
    else
    {
        fprintf(stderr, "Error getting CPU fan speed\n");
    }
}

void update_gpu_fan_speed(void)
{
    double speed = get_gpu_fan_speed();
    if (speed >= 0)
    {
        update_gauge(gpu_fan_speed_metric, speed);
    }
    else
    {
        fprintf(stderr, "Error getting GPU fan speed\n");
    }
}

void update_memory_metrics(void)
{
    update_gauge(total_memory_metric, get_total_memory());
    update_gauge(used_memory_metric, get_used_memory());
    update_gauge(available_memory_metric, get_available_memory());
}

void update_network_traffic_metric(void)
{
    NetworkStats stats = get_network_traffic();
    update_gauge(rx_bytes_metric, (double)stats.rx_bytes);
    update_gauge(tx_bytes_metric, (double)stats.tx_bytes);
    update_gauge(rx_errors_metric, (double)stats.rx_errors);
    update_gauge(tx_errors_metric, (double)stats.tx_errors);
    update_gauge(dropped_packets_metric, (double)stats.dropped_packets);
}

void update_context_switches_metric(void)
{
    update_gauge(context_switches_metric, get_context_switches());
}

void update_disk_stats_metrics(void)
{
    DiskStats stats = get_disk_stats();
    if (stats.io_time >= 0)
    {
        update_gauge(io_time_metric, (double)stats.io_time);
        update_gauge(writes_completed_metric, (double)stats.writes_completed);
        update_gauge(reads_completed_metric, (double)stats.reads_completed);
    }
    else
    {
        fprintf(stderr, "Error obtaining disk stats\n");
    }
}

void* expose_metrics(const void* arg)
{
    (void)arg;

    promhttp_set_active_collector_registry(NULL);

    struct MHD_Daemon* daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, 8000, NULL, NULL);
    if (daemon == NULL)
    {
        fprintf(stderr, "Error starting HTTP server\n");
        return NULL;
    }

    while (keep_running)
    {
        sleep(1);
    }

    MHD_stop_daemon(daemon);
    return NULL;
}

void init_metrics(const char* selected_metrics[], size_t num_metrics)
{
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "Error initializing mutex\n");
    }

    if (prom_collector_registry_default_init() != 0)
    {
        fprintf(stderr, "Error initializing Prometheus registry\n");
    }

    // Iterate over the selected metrics array and create/register the metrics
    for (size_t i = 0; i < num_metrics; i++)
    {
        const char* metric_name = selected_metrics[i];

        // Find the metric in the all_metrics array
        for (MetricInfo* info = all_metrics; info->name != NULL; info++)
        {
            if (strcmp(metric_name, info->name) == 0)
            {
                *(info->metric) = prom_gauge_new(info->name, info->description, 0, NULL);
                prom_collector_registry_must_register_metric((prom_metric_t*)*(info->metric));
                break;
            }
        }
    }
}

void show_available_metrics(void)
{
    FILE* file = fopen(METRICS_FILE, "w");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    for (MetricInfo* info = all_metrics; info->name != NULL; info++)
    {
        fprintf(file, "Metric: %s\n", info->name);
    }

    fclose(file);
}

void destroy_mutex(void)
{
    pthread_mutex_destroy(&lock);
}