#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/android_reboot.h>

static const uint64_t kRegisterAddress = 0xD4010014ULL;
static const uint32_t kFastbootMarker = 0x66617374U;
static const char kMemoryDevice[] = "/dev/mem";

static_assert((kRegisterAddress & 3ULL) == 0ULL, "register must be 32-bit aligned");
static_assert(sizeof(uint32_t) == 4, "32-bit register access is required");

struct RegisterMapping {
    int fd;
    void *base;
    size_t length;
    volatile uint32_t *reg;
};

static void mmio_barrier() {
#if defined(__aarch64__)
    __asm__ __volatile__("dsb sy" ::: "memory");
#elif defined(__arm__)
    __asm__ __volatile__("dsb" ::: "memory");
#else
    __sync_synchronize();
#endif
}

static void print_usage(const char *program) {
    fprintf(stderr,
            "Usage: %s [--check | --test-write]\n"
            "  --check       Read the PXA fastboot marker register only\n"
            "  --test-write  Write the marker, verify it, restore the original value\n"
            "  (no option)   Write the marker, verify it, reboot with reason bootloader\n",
            program);
}

static int parse_arguments(int argc, char **argv, bool *check_only,
                           bool *test_write) {
    *check_only = false;
    *test_write = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--check") == 0) {
            if (*test_write) return -1;
            *check_only = true;
        } else if (strcmp(argv[i], "--test-write") == 0) {
            if (*check_only) return -1;
            *test_write = true;
        } else if (strcmp(argv[i], "--help") == 0 ||
                   strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 1;
        } else {
            return -1;
        }
    }

    return 0;
}

static bool open_register_mapping(bool writable, RegisterMapping *mapping,
                                  int *error_number) {
    mapping->fd = -1;
    mapping->base = MAP_FAILED;
    mapping->length = 0;
    mapping->reg = NULL;

    const int open_flags = (writable ? O_RDWR : O_RDONLY) | O_SYNC | O_CLOEXEC;
    mapping->fd = open(kMemoryDevice, open_flags);
    if (mapping->fd < 0) {
        *error_number = errno;
        return false;
    }

    struct stat device_stat;
    if (fstat(mapping->fd, &device_stat) != 0) {
        *error_number = errno;
        return false;
    }

    if (!S_ISCHR(device_stat.st_mode)) {
        *error_number = ENODEV;
        return false;
    }

    const long page_size_long = sysconf(_SC_PAGESIZE);
    if (page_size_long <= 0) {
        *error_number = errno != 0 ? errno : EINVAL;
        return false;
    }

    const uint64_t page_size = static_cast<uint64_t>(page_size_long);
    if ((page_size & (page_size - 1ULL)) != 0ULL) {
        *error_number = EINVAL;
        return false;
    }

    const uint64_t page_mask = ~(page_size - 1ULL);
    const uint64_t map_address = kRegisterAddress & page_mask;
    const uint64_t register_offset = kRegisterAddress - map_address;

    if (register_offset + sizeof(uint32_t) > page_size) {
        *error_number = EINVAL;
        return false;
    }

    const int protection = PROT_READ | (writable ? PROT_WRITE : 0);
    mapping->base = mmap64(NULL,
                           static_cast<size_t>(page_size),
                           protection,
                           MAP_SHARED,
                           mapping->fd,
                           static_cast<off64_t>(map_address));
    if (mapping->base == MAP_FAILED) {
        *error_number = errno;
        return false;
    }

    mapping->length = static_cast<size_t>(page_size);
    mapping->reg = reinterpret_cast<volatile uint32_t *>(
            static_cast<char *>(mapping->base) + register_offset);
    return true;
}

static void close_register_mapping(RegisterMapping *mapping) {
    if (mapping->base != MAP_FAILED) {
        if (munmap(mapping->base, mapping->length) != 0) {
            fprintf(stderr, "Warning: munmap failed: %s\n", strerror(errno));
        }
        mapping->base = MAP_FAILED;
        mapping->reg = NULL;
        mapping->length = 0;
    }

    if (mapping->fd >= 0) {
        if (close(mapping->fd) != 0) {
            fprintf(stderr, "Warning: close(%s) failed: %s\n",
                    kMemoryDevice, strerror(errno));
        }
        mapping->fd = -1;
    }
}

static uint32_t read_register(volatile uint32_t *reg) {
    mmio_barrier();
    const uint32_t value = *reg;
    mmio_barrier();
    return value;
}

static bool write_register_and_verify(volatile uint32_t *reg,
                                      uint32_t value,
                                      uint32_t *read_back) {
    mmio_barrier();
    *reg = value;
    mmio_barrier();
    *read_back = *reg;
    mmio_barrier();
    return *read_back == value;
}

static bool restore_register(volatile uint32_t *reg, uint32_t original_value,
                             uint32_t *read_back) {
    return write_register_and_verify(reg, original_value, read_back);
}

int main(int argc, char **argv) {
    bool check_only = false;
    bool test_write = false;

    const int parse_result = parse_arguments(argc, argv, &check_only, &test_write);
    if (parse_result > 0) return 0;
    if (parse_result < 0) {
        print_usage(argv[0]);
        return 2;
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Error: root privileges are required (euid=%ld).\n",
                static_cast<long>(geteuid()));
        return 3;
    }

    RegisterMapping mapping;
    int access_error = 0;
    if (!open_register_mapping(!check_only, &mapping, &access_error)) {
        close_register_mapping(&mapping);
        fprintf(stderr, "Error: cannot access %s at 0x%08" PRIx64 ": %s\n",
                kMemoryDevice, kRegisterAddress,
                strerror(access_error != 0 ? access_error : EIO));
        return 4;
    }

    const uint32_t original_value = read_register(mapping.reg);
    printf("SoC register   : PXA fastboot trigger\n");
    printf("Access         : %s + mmap64\n", kMemoryDevice);
    printf("Physical addr  : 0x%08" PRIx64 " (32-bit)\n", kRegisterAddress);
    printf("Original value : 0x%08" PRIx32 "\n", original_value);

    if (check_only) {
        printf("Fastboot marker: 0x%08" PRIx32 "\n", kFastbootMarker);
        printf("Register state : %s\n",
               original_value == kFastbootMarker ? "MATCH" : "NOT ARMED");
        close_register_mapping(&mapping);
        return original_value == kFastbootMarker ? 0 : 1;
    }

    uint32_t marker_read_back = 0;
    const bool marker_written = write_register_and_verify(
            mapping.reg, kFastbootMarker, &marker_read_back);

    printf("Marker value   : 0x%08" PRIx32 "\n", kFastbootMarker);
    printf("Marker readback: 0x%08" PRIx32 "\n", marker_read_back);

    if (!marker_written) {
        uint32_t restored_value = 0;
        const bool restored = restore_register(
                mapping.reg, original_value, &restored_value);
        fprintf(stderr,
                "Error: marker verification failed; reboot cancelled.\n"
                "Restore       : %s (0x%08" PRIx32 ")\n",
                restored ? "OK" : "FAILED", restored_value);
        close_register_mapping(&mapping);
        return 5;
    }

    if (test_write) {
        uint32_t restored_value = 0;
        const bool restored = restore_register(
                mapping.reg, original_value, &restored_value);
        printf("Restore value : 0x%08" PRIx32 "\n", restored_value);
        close_register_mapping(&mapping);
        if (!restored) {
            fprintf(stderr, "Critical: original register value was not restored.\n");
            return 6;
        }
        printf("Test result   : write/read/restore OK; reboot skipped\n");
        return 0;
    }

    mmio_barrier();
    fflush(stdout);
    errno = 0;
    const int reboot_result = android_reboot(ANDROID_RB_RESTART2, 0, "bootloader");
    const int reboot_error = errno;

    if (reboot_result != 0) {
        uint32_t restored_value = 0;
        const bool restored = restore_register(
                mapping.reg, original_value, &restored_value);
        fprintf(stderr, "Error: android_reboot failed (return=%d", reboot_result);
        if (reboot_error != 0) {
            fprintf(stderr, ", errno=%d: %s", reboot_error, strerror(reboot_error));
        }
        fprintf(stderr, "). Restore %s (0x%08" PRIx32 ").\n",
                restored ? "OK" : "FAILED", restored_value);
        close_register_mapping(&mapping);
        return 7;
    }

    printf("Reboot request: reboot,bootloader accepted by Android init\n");
    fflush(stdout);
    close_register_mapping(&mapping);
    return 0;
}
