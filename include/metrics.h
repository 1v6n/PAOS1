#ifndef METRICS_H
#define METRICS_H

/**
 * @file metrics.h
 * @brief Header file for retrieving system metrics from the /proc filesystem.
 *
 * This header file declares functions used to retrieve various system metrics such as CPU usage, memory usage, disk
 * stats, CPU temperature, and more from the /proc filesystem. These metrics are essential for monitoring system
 * performance and health.
 *
 * The functions include methods to read and parse data from the /proc filesystem, providing detailed statistics on
 * system resources. The metrics are used to track the performance and status of the system in real-time.
 *
 * @date 09/10/2024
 * @author 1v6n
 */

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h> // Required for retrieving file system stats
#include <unistd.h>

#define SLEEP_TIME 1                      /**< Sleep time in seconds for the main loop. */
#define COMMAND_SIZE 512                  /**< Size of the command buffer. */
#define BUFFER_SIZE 1024                  /**< Buffer size for reading files. */
#define DISKSTATS_PATH "/proc/diskstats"  /**< Path to the disk stats file. */
#define RETURN_ERROR -1                   /**< Return value for functions that encounter an error. */
#define PROC_STAT_PATH "/proc/stat"       /**< Path to the stat file. */
#define PROC_NET_DEV_PATH "/proc/net/dev" /**< Path to the network device file. */
#define NETWORK_INTERFACE "wlp4s0"        /**< Network interface to monitor. */
#define PROC_MEMINFO_PATH "/proc/meminfo" /**< Path to the meminfo file. */
#define PROC_STAT_PATH "/proc/stat"       /**< Path to the stat file. */
#define ROOT_PATH "/"                     /**< Root path for the file system. */
#define HWMON_CPU_TEMP_PATH "/sys/class/hwmon/hwmon4/temp1_input"             /**< Path to the CPU temperature file. */
#define HWMON_BATTERY_VOLTAGE_PATH "/sys/class/hwmon/hwmon2/in0_input"        /**< Path to the battery voltage file. */
#define HWMON_BATTERY_CURRENT_PATH "/sys/class/hwmon/hwmon2/curr1_input"      /**< Path to the battery current file. */
#define CPU_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq" /**< Path to the CPU frequency file. */
#define CPU_FAN_SPEED_PATH "/sys/class/hwmon/hwmon5/fan1_input"               /**< Path to the CPU fan speed file. */
#define GPU_FAN_SPEED_PATH "/sys/class/hwmon/hwmon5/fan2_input"               /**< Path to the GPU fan speed file. */
#define UNIT_CONVERSION 1000.0           /**< Unit conversion factor for file paths. */
#define CONVERT_TO_MB 1024.0             /**< Conversion factor for memory values. */
#define PERCENTAGE 100.0                 /**< Conversion factor for percentage values. */
#define PROC_DIR_PATH "/proc"            /**< Path to the /proc directory. */
#define STAT_FILE_FORMAT "/proc/%s/stat" /**< Format string for the stat file. */

/**
 * @brief Reads the value from the specified file.
 *
 * This function reads the value from the specified file and returns it as a double.
 *
 * @param path The path to the file to read.
 * @return The value read from the file, or -1.0 in case of an error.
 */
static double read_value(const char* path);

/**
 * @brief Retrieves the memory usage percentage from /proc/meminfo.
 *
 * Reads total and available memory values from /proc/meminfo and calculates
 * the percentage of memory currently in use.
 *
 * @return Memory usage as a percentage (0.0 to 100.0), or -1.0 in case of error.
 */
double get_memory_usage();

/**
 * @brief Retrieves the CPU usage percentage from /proc/stat.
 *
 * Reads CPU time values from /proc/stat and calculates the percentage of CPU usage
 * over a time interval.
 *
 * @return CPU usage as a percentage (0.0 to 100.0), or -1.0 in case of error.
 */
double get_cpu_usage();

/**
 * @brief Retrieves the disk usage percentage.
 *
 * Uses the `statvfs()` function to obtain file system statistics and calculates
 * the percentage of disk space used.
 *
 * @return Disk usage as a percentage, or -1.0 in case of error.
 */
double get_disk_usage();

/**
 * @brief Retrieves the current CPU temperature.
 *
 * This function reads the CPU temperature from the hardware monitor file and returns the value.
 *
 * @return The CPU temperature in degrees Celsius, or -1.0 in case of an error.
 */
double get_cpu_temperature();

/**
 * @brief Retrieves the current battery voltage.
 *
 * This function reads the battery voltage from the hardware monitor file and returns the value.
 *
 * @return The battery voltage, or -1.0 in case of an error.
 */
double get_battery_voltage();

/**
 * @brief Retrieves the current battery current.
 *
 * This function reads the battery current from the hardware monitor file and returns the value.
 *
 * @return The battery current, or -1.0 in case of an error.
 */
double get_battery_current();

/**
 * @brief Retrieves the current CPU frequency.
 *
 * This function reads the current CPU frequency from the system file and returns the value.
 *
 * @return The CPU frequency, or -1.0 in case of an error.
 */
double get_cpu_frequency();

/**
 * @brief Retrieves the current CPU fan speed.
 *
 * This function reads the CPU fan speed from the hardware monitor file and returns the value.
 *
 * @return The CPU fan speed in RPM, or -1.0 in case of an error.
 */
double get_cpu_fan_speed();

/**
 * @brief Retrieves the current GPU fan speed.
 *
 * This function reads the GPU fan speed from the hardware monitor file and returns the value.
 *
 * @return The GPU fan speed in RPM, or -1.0 in case of an error.
 */
double get_gpu_fan_speed();

/**
 * @brief Retrieves the total, suspended, ready, and blocked processes.
 *
 * Reads the state of all processes from /proc and counts the total, suspended,
 * ready, and blocked processes.
 *
 * @param total Pointer to store the total number of processes.
 * @param suspended Pointer to store the number of suspended processes.
 * @param ready Pointer to store the number of ready processes.
 * @param blocked Pointer to store the number of blocked processes.
 */
void get_process_states(int* total, int* suspended, int* ready, int* blocked);

/**
 * @brief Retrieves the total memory available in the system.
 *
 * Reads the total memory from /proc/meminfo and returns the value in MB.
 *
 * @return Total memory in MB, or -1.0 in case of error.
 */
double get_total_memory();

/**
 * @brief Retrieves the amount of memory currently used in the system.
 *
 * Reads the memory usage from /proc/meminfo and calculates the used memory in MB.
 *
 * @return Used memory in MB, or -1.0 in case of error.
 */
double get_used_memory();

/**
 * @brief Retrieves the available memory in the system.
 *
 * Reads the available memory from /proc/meminfo and returns the value in MB.
 *
 * @return Available memory in MB, or -1.0 in case of error.
 */
double get_available_memory();

/**
 * @brief Retrieves the number of context switches.
 *
 * Reads the number of context switches from /proc/stat.
 *
 * @return The number of context switches, or -1 in case of error.
 */
int get_context_switches();

/**
 * @brief Structure to hold disk statistics.
 */
typedef struct
{
    unsigned long long io_time;          /**< Time spent on I/O in milliseconds. */
    unsigned long long writes_completed; /**< Total writes completed. */
    unsigned long long reads_completed;  /**< Total reads completed. */
} DiskStats;

/**
 * @brief Retrieves disk statistics including I/O time and the number of completed read and write operations.
 *
 * Reads the disk statistics from /proc/diskstats and returns them in a DiskStats structure.
 *
 * @return A DiskStats struct containing I/O time, writes completed, and reads completed.
 */
DiskStats get_disk_stats();

/**
 * @brief Structure to hold network traffic statistics.
 */
typedef struct
{
    unsigned long long rx_bytes;        /**< Total bytes received. */
    unsigned long long tx_bytes;        /**< Total bytes transmitted. */
    unsigned long long rx_errors;       /**< Total receive errors. */
    unsigned long long tx_errors;       /**< Total transmit errors. */
    unsigned long long dropped_packets; /**< Total dropped packets. */
} NetworkStats;

/**
 * @brief Retrieves network traffic statistics for the specified network interface.
 *
 * Reads the network traffic statistics from /proc/net/dev and returns the total bytes received,
 * transmitted, and errors encountered for the "wlp4s0" interface.
 *
 * @return A NetworkStats struct containing network traffic statistics.
 */
NetworkStats get_network_traffic();

#endif // METRICS_H
