/**
 * @file metrics.c
 * @brief This file contains functions for retrieving system metrics such as memory usage, CPU usage, disk usage, CPU
 * temperature, and others.
 * @author 1v6n
 * @date 09/10/2024
 */

#include "metrics.h"

static double read_value(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return RETURN_ERROR;
    }

    int value;
    if (fscanf(fp, "%d", &value) != 1)
    {
        fprintf(stderr, "Error reading value from %s\n", path);
        fclose(fp);
        return RETURN_ERROR;
    }

    fclose(fp);
    return value / UNIT_CONVERSION;
}

double get_memory_usage()
{
    FILE* fp = fopen(PROC_MEMINFO_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening " PROC_MEMINFO_PATH);
        return RETURN_ERROR;
    }

    char buffer[BUFFER_SIZE];
    double total_mem = 0, free_mem = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %llu kB", &total_mem) == 1)
        {
            continue;
        }
        if (sscanf(buffer, "MemAvailable: %llu kB", &free_mem) == 1)
        {
            break;
        }
    }

    fclose(fp);

    if (total_mem == 0 || free_mem == 0)
    {
        fprintf(stderr, "Error reading memory information from " PROC_MEMINFO_PATH "\n");
        return RETURN_ERROR;
    }

    double used_mem = (total_mem - free_mem);
    double mem_usage_percent = (used_mem / total_mem) * 100.0;

    return mem_usage_percent;
}

double get_cpu_usage()
{
    static double prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0, prev_iowait = 0, prev_irq = 0,
                  prev_softirq = 0, prev_steal = 0;
    double user, nice, system, idle, iowait, irq, softirq, steal;

    FILE* fp = fopen(PROC_STAT_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening " PROC_STAT_PATH);
        return RETURN_ERROR;
    }

    char buffer[BUFFER_SIZE * 4];
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        perror("Error reading " PROC_STAT_PATH);
        fclose(fp);
        return RETURN_ERROR;
    }
    fclose(fp);

    int ret = sscanf(buffer, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait,
                     &irq, &softirq, &steal);
    if (ret < 8)
    {
        fprintf(stderr, "Error parsing " PROC_STAT_PATH "\n");
        return RETURN_ERROR;
    }

    double prev_idle_total = prev_idle + prev_iowait;
    double idle_total = idle + iowait;
    double prev_non_idle = prev_user + prev_nice + prev_system + prev_irq + prev_softirq + prev_steal;
    double non_idle = user + nice + system + irq + softirq + steal;
    double prev_total = prev_idle_total + prev_non_idle;
    double total = idle_total + non_idle;
    double totald = total - prev_total;
    double idled = idle_total - prev_idle_total;

    if (totald == 0)
    {
        fprintf(stderr, "Totald is zero, cannot calculate CPU usage!\n");
        return RETURN_ERROR;
    }

    double cpu_usage_percent = ((totald - idled) / totald) * 100.0;

    prev_user = user;
    prev_nice = nice;
    prev_system = system;
    prev_idle = idle;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;

    return cpu_usage_percent;
}

double get_disk_usage()
{
    struct statvfs stat;

    if (statvfs(ROOT_PATH, &stat) != 0)
    {
        fprintf(stderr, "Error getting file system statistics\n");
        return RETURN_ERROR;
    }

    unsigned long total = stat.f_blocks * stat.f_frsize;
    unsigned long available = stat.f_bavail * stat.f_frsize;
    unsigned long used = total - available;

    double usage_percentage = ((double)used / (double)total) * PERCENTAGE;
    return usage_percentage;
}

double get_cpu_temperature()
{
    return read_value(HWMON_CPU_TEMP_PATH);
}

double get_battery_voltage()
{
    return read_value(HWMON_BATTERY_VOLTAGE_PATH);
}

double get_battery_current()
{
    return read_value(HWMON_BATTERY_CURRENT_PATH);
}

double get_cpu_frequency()
{
    return read_value(CPU_FREQ_PATH);
}

double get_cpu_fan_speed()
{
    return read_value(CPU_FAN_SPEED_PATH) * UNIT_CONVERSION;
}

double get_gpu_fan_speed()
{
    return read_value(GPU_FAN_SPEED_PATH) * UNIT_CONVERSION;
}

void get_process_states(int* total, int* suspended, int* ready, int* blocked)
{
    DIR* proc_dir = opendir(PROC_DIR_PATH);
    if (proc_dir == NULL)
    {
        perror("Error opening " PROC_DIR_PATH);
        return;
    }

    struct dirent* entry;
    *total = 0;
    *suspended = 0;
    *ready = 0;
    *blocked = 0;

    while ((entry = readdir(proc_dir)) != NULL)
    {
        if (isdigit(entry->d_name[0]))
        {
            char path[BUFFER_SIZE];
            snprintf(path, sizeof(path), STAT_FILE_FORMAT, entry->d_name);

            FILE* fp = fopen(path, "r");
            if (fp == NULL)
            {
                continue;
            }

            char state;
            if (fscanf(fp, "%*d %*s %c", &state) == 1)
            {
                (*total)++;
                if (state == 'S')
                {
                    (*suspended)++;
                }
                else if (state == 'R')
                {
                    (*ready)++;
                }
                else if (state == 'D')
                {
                    (*blocked)++;
                }
            }

            fclose(fp);
        }
    }

    closedir(proc_dir);
}

double get_total_memory()
{
    FILE* fp = fopen(PROC_MEMINFO_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening " PROC_MEMINFO_PATH);
        return RETURN_ERROR;
    }

    char buffer[BUFFER_SIZE];
    double total_mem = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %llu kB", &total_mem) == 1)
        {
            break;
        }
    }

    fclose(fp);
    return total_mem / CONVERT_TO_MB;
}

double get_used_memory()
{
    FILE* fp = fopen(PROC_MEMINFO_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening " PROC_MEMINFO_PATH);
        return RETURN_ERROR;
    }

    char buffer[BUFFER_SIZE];
    double total_mem = 0, free_mem = 0, buffers = 0, cached = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        sscanf(buffer, "MemTotal: %llu kB", &total_mem);
        sscanf(buffer, "MemFree: %llu kB", &free_mem);
        sscanf(buffer, "Buffers: %llu kB", &buffers);
        sscanf(buffer, "Cached: %llu kB", &cached);
    }

    fclose(fp);
    return (total_mem - free_mem - buffers - cached) / CONVERT_TO_MB;
}

double get_available_memory()
{
    FILE* fp = fopen(PROC_MEMINFO_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening " PROC_MEMINFO_PATH);
        return RETURN_ERROR;
    }

    char buffer[BUFFER_SIZE];
    double available_mem = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemAvailable: %llu kB", &available_mem) == 1)
        {
            break;
        }
    }

    fclose(fp);
    return available_mem / CONVERT_TO_MB;
}

NetworkStats get_network_traffic()
{
    FILE* fp = fopen(PROC_NET_DEV_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening " PROC_NET_DEV_PATH);
        return (NetworkStats){RETURN_ERROR, RETURN_ERROR, RETURN_ERROR, RETURN_ERROR, RETURN_ERROR};
    }

    char buffer[BUFFER_SIZE];
    unsigned long long rx_bytes = 0, tx_bytes = 0;
    unsigned long long rx_errors = 0, tx_errors = 0, dropped_packets = 0;

    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (strstr(buffer, NETWORK_INTERFACE) != NULL)
        {
            unsigned long long r_bytes, t_bytes, r_errors, t_errors, drop;

            int matched = sscanf(buffer, "%*[^:]: %llu %*d %llu %llu %*d %*d %*d %*d %llu %*d %llu", &r_bytes,
                                 &r_errors, &drop, &t_bytes, &t_errors);

            if (matched != 5)
            {
                fprintf(stderr, "sscanf failed to match expected format (matched = %d) for line: %s\n", matched,
                        buffer);
                continue;
            }

            rx_bytes = r_bytes;
            tx_bytes = t_bytes;
            rx_errors = r_errors;
            tx_errors = t_errors;
            dropped_packets = drop;

            break;
        }
    }

    fclose(fp);
    return (NetworkStats){rx_bytes, tx_bytes, rx_errors, tx_errors, dropped_packets};
}

int get_context_switches()
{
    FILE* fp = fopen(PROC_STAT_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/stat");
        return RETURN_ERROR;
    }

    char buffer[BUFFER_SIZE];
    int context_switches = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "ctxt %d", &context_switches) == 1)
        {
            break;
        }
    }

    fclose(fp);
    return context_switches;
}

DiskStats get_disk_stats()
{
    FILE* fp = fopen(DISKSTATS_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening " DISKSTATS_PATH);
        return (DiskStats){RETURN_ERROR, RETURN_ERROR, RETURN_ERROR};
    }

    char buffer[BUFFER_SIZE];
    unsigned long long io_time = 0, writes_completed = 0, reads_completed = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        long long it, wc, rc;
        if (sscanf(buffer, "%*d %*d %*s %lld %*d %*d %*d %lld %*d %lld", &rc, &wc, &it) == 3)
        {
            reads_completed += rc;
            writes_completed += wc;
            io_time += it;
        }
    }
    fclose(fp);
    return (DiskStats){io_time, writes_completed, reads_completed};
}
