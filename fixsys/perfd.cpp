#include <android/log.h>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

#define LOG_TAG "perfd"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static bool write_sysfs(const std::string &path, const std::string &value) {
    int fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        LOGE("open failed: %s (%s)", path.c_str(), strerror(errno));
        return false;
    }
    ssize_t written = write(fd, value.c_str(), value.size());
    close(fd);
    if (written != static_cast<ssize_t>(value.size())) {
        LOGE("write failed: %s = %s (%s)", path.c_str(), value.c_str(), strerror(errno));
        return false;
    }
    LOGI("set %s = %s", path.c_str(), value.c_str());
    return true;
}

static std::string read_sysfs(const std::string &path) {
    FILE *fp = fopen(path.c_str(), "r");
    if (fp == nullptr) {
        return std::string();
    }
    char buf[256];
    std::string result;
    if (fgets(buf, sizeof(buf), fp) != nullptr) {
        result = buf;
        while (!result.empty() &&
               (result[result.size() - 1] == '\n' || result[result.size() - 1] == '\r')) {
            result.erase(result.size() - 1, 1);
        }
    }
    fclose(fp);
    return result;
}

static bool contains_token(const std::string &haystack, const std::string &token) {
    return haystack.find(token) != std::string::npos;
}

static std::vector<std::string> list_dir(const std::string &base, const std::string &prefix) {
    std::vector<std::string> result;
    DIR *dir = opendir(base.c_str());
    if (dir == nullptr) {
        return result;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        if (prefix.empty() || name.compare(0, prefix.size(), prefix) == 0) {
            result.push_back(name);
        }
    }
    closedir(dir);
    return result;
}

static void optimize_cpufreq() {
    std::vector<std::string> cpus = list_dir("/sys/devices/system/cpu", "cpu");
    for (const auto &cpu : cpus) {
        if (cpu.size() < 4 || !isdigit(static_cast<unsigned char>(cpu[3]))) {
            continue;
        }
        std::string base = "/sys/devices/system/cpu/" + cpu + "/cpufreq";
        std::string available = read_sysfs(base + "/scaling_available_governors");
        std::string max_freq = read_sysfs(base + "/cpuinfo_max_freq");
        if (contains_token(available, "performance")) {
            write_sysfs(base + "/scaling_governor", "performance");
        } else if (contains_token(available, "ondemand")) {
            write_sysfs(base + "/scaling_governor", "ondemand");
            if (!max_freq.empty()) {
                write_sysfs(base + "/scaling_min_freq", max_freq);
                write_sysfs(base + "/scaling_max_freq", max_freq);
            }
        } else if (contains_token(available, "userspace")) {
            write_sysfs(base + "/scaling_governor", "userspace");
            if (!max_freq.empty()) {
                write_sysfs(base + "/scaling_min_freq", max_freq);
                write_sysfs(base + "/scaling_max_freq", max_freq);
                write_sysfs(base + "/scaling_setspeed", max_freq);
            }
        }
    }
}

static void optimize_gpufreq() {
    std::vector<std::string> cores;
    cores.push_back("gpu0");
    cores.push_back("gpu1");
    cores.push_back("gpu2");
    for (const auto &core : cores) {
        std::string base = "/sys/devices/platform/galcore/gpu/" + core + "/gpufreq";
        std::string available = read_sysfs(base + "/scaling_available_governors");
        std::string max_freq = read_sysfs(base + "/cpuinfo_max_freq");
        if (max_freq.empty()) {
            max_freq = read_sysfs(base + "/scaling_max_freq");
        }
        if (max_freq.empty()) {
            continue;
        }
        bool governor_set = false;
        if (available.empty() || contains_token(available, "performance")) {
            governor_set = write_sysfs(base + "/scaling_governor", "performance");
        }
        if (!governor_set && (available.empty() || contains_token(available, "ondemand"))) {
            governor_set = write_sysfs(base + "/scaling_governor", "ondemand");
        }
        if (!governor_set) {
            write_sysfs(base + "/scaling_governor", "userspace");
        }
        write_sysfs(base + "/scaling_min_freq", max_freq);
        write_sysfs(base + "/scaling_max_freq", max_freq);
        write_sysfs(base + "/scaling_setspeed", max_freq);
    }
}

static std::string long_to_string(long value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", value);
    return std::string(buf);
}

static long parse_khz(const std::string &hz_str) {
    if (hz_str.empty()) {
        return 0;
    }
    return strtol(hz_str.c_str(), nullptr, 10) / 1000;
}

static void optimize_devfreq() {
    std::vector<std::string> devices = list_dir("/sys/class/devfreq", "");
    for (const auto &dev : devices) {
        std::string base = "/sys/class/devfreq/" + dev;
        std::string max_freq_hz = read_sysfs(base + "/max_freq");
        if (dev.compare(0, 11, "devfreq-ddr") == 0) {
            long max_khz = parse_khz(max_freq_hz);
            if (max_khz > 0) {
                write_sysfs(base + "/disable_ddr_fc", "1");
                write_sysfs(base + "/ddr_freq", long_to_string(max_khz));
            }
            continue;
        }
        std::string available = read_sysfs(base + "/available_governors");
        if (contains_token(available, "performance")) {
            write_sysfs(base + "/governor", "performance");
        } else if (contains_token(available, "userspace")) {
            if (!max_freq_hz.empty()) {
                write_sysfs(base + "/governor", "userspace");
                write_sysfs(base + "/userspace/set_freq", max_freq_hz);
            }
        }
    }
}

static void optimize_io_scheduler() {
    std::vector<std::string> blocks = list_dir("/sys/block", "");
    for (const auto &block : blocks) {
        if (block.compare(0, 4, "loop") == 0 || block.compare(0, 3, "ram") == 0) {
            continue;
        }
        std::string sched_path = "/sys/block/" + block + "/queue/scheduler";
        std::string available = read_sysfs(sched_path);
        if (contains_token(available, "noop")) {
            write_sysfs(sched_path, "noop");
        } else if (contains_token(available, "deadline")) {
            write_sysfs(sched_path, "deadline");
        }
        write_sysfs("/sys/block/" + block + "/queue/read_ahead_kb", "512");
    }
}

static void optimize_vm() {
    write_sysfs("/proc/sys/vm/swappiness", "10");
    write_sysfs("/proc/sys/vm/vfs_cache_pressure", "50");
    write_sysfs("/proc/sys/vm/dirty_ratio", "20");
    write_sysfs("/proc/sys/vm/dirty_background_ratio", "5");
    write_sysfs("/proc/sys/vm/min_free_kbytes", "8192");
}

static void optimize_lmk() {
    write_sysfs("/sys/module/lowmemorykiller/parameters/minfree",
                "8192,10240,12288,16384,20480,24576");
    write_sysfs("/sys/module/lowmemorykiller/parameters/adj", "0,1,2,4,9,15");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    LOGI("perfd start");
    optimize_cpufreq();
    optimize_gpufreq();
    optimize_devfreq();
    optimize_io_scheduler();
    optimize_vm();
    optimize_lmk();
    LOGI("perfd done");
    return 0;
}
