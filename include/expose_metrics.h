#ifndef EXPOSE_METRICS_H
#define EXPOSE_METRICS_H

/**
 * @file expose_metrics.h
 * @brief Header file for exposing system metrics via Prometheus.
 *
 * This header file declares functions and data structures used to initialize and expose various system metrics
 * such as CPU usage, memory usage, disk stats, and network traffic through Prometheus gauges. These metrics are
 * collected from the system and made available via an HTTP server for monitoring purposes.
 *
 * The metrics include usage statistics for CPU, memory, disk, network, battery, and more. The metrics are updated
 * safely in a multi-threaded environment using mutexes to ensure thread safety.
 *
 * @date 09/10/2024
 * @author 1v6n
 */

#include "metrics.h"
#include <errno.h>
#include <prom.h>
#include <promhttp.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIFO_FIRST_FIT "/tmp/fragmentation_fifo_FIRST_FIT"
#define FIFO_BEST_FIT "/tmp/fragmentation_fifo_BEST_FIT"
#define FIFO_WORST_FIT "/tmp/fragmentation_fifo_WORST_FIT"

typedef struct
{
    const char* name;
    const char* description;
    prom_gauge_t** metric;
    void (*update_function)(void); // Pointer to the update function
} MetricInfo;

extern MetricInfo all_metrics[];

/**
 * @brief Updates a Prometheus gauge metric with thread safety.
 *
 * @param gauge The Prometheus gauge metric to update.
 * @param value The value to set for the metric.
 */
void update_gauge(prom_gauge_t* gauge, double value);

/**
 * @brief Updates the CPU usage metric.
 */
void update_cpu_gauge(void);

/**
 * @brief Updates the memory usage metric.
 */
void update_memory_gauge(void);

/**
 * @brief Thread function to expose metrics via HTTP on port 8000.
 * @param arg Unused argument.
 * @return NULL
 */
void* expose_metrics(const void* arg);

/**
 * @brief Initializes mutex and metrics.
 */
void init_metrics(const char* selected_metrics[], size_t num_metrics);

/**
 * @brief Destroys the mutex.
 */
void destroy_mutex(void);

/**
 * @brief Updates the disk usage metric.
 */
void update_disk_gauge(void);

/**
 * @brief Updates the running processes metric.
 */
void update_running_processes_gauge(void);

/**
 * @brief Updates the CPU temperature metric.
 */
void update_cpu_temperature(void);

/**
 * @brief Updates the battery voltage metric.
 */
void update_battery_voltage(void);

/**
 * @brief Updates the battery current metric.
 */
void update_battery_current(void);

/**
 * @brief Updates the CPU frequency metric.
 */
void update_cpu_frequency(void);

/**
 * @brief Updates the CPU fan speed metric.
 */
void update_cpu_fan_speed(void);

/**
 * @brief Updates the GPU fan speed metric.
 */
void update_gpu_fan_speed(void);

/**
 * @brief Updates the process states metrics.
 */
void update_process_states_gauge(void);

/**
 * @brief Updates the memory metrics.
 */
void update_memory_metrics(void);

/**
 * @brief Updates the network traffic metrics.
 */
void update_network_traffic_metric(void);

/**
 * @brief Updates the context switches metric.
 */
void update_context_switches_metric(void);

/**
 * @brief Updates the disk stats metrics.
 */
void update_disk_stats_metrics(void);

void update_fragmentation_first_fit_metric(void);
void update_fragmentation_best_fit_metric(void);
void update_fragmentation_worst_fit_metric(void);

double read_number_from_fifo(const char* fifo_path);

/**
 * @brief Prints all available metrics with their names and descriptions.
 */
void show_available_metrics(void);

#endif // EXPOSE_METRICS_H
