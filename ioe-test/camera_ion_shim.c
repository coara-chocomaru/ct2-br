#define LOG_TAG "CameraIonShim"
#define _GNU_SOURCE 1
#include <cutils/log.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/ioctl.h>

typedef int shim_ion_user_handle_t;

struct shim_ion_allocation_data {
    size_t len;
    size_t align;
    unsigned int heap_id_mask;
    unsigned int flags;
    shim_ion_user_handle_t handle;
};

struct shim_ion_fd_data {
    shim_ion_user_handle_t handle;
    int fd;
};

struct shim_ion_handle_data {
    shim_ion_user_handle_t handle;
};

#define SHIM_ION_IOC_MAGIC 'I'
#define SHIM_ION_IOC_ALLOC _IOWR(SHIM_ION_IOC_MAGIC, 0, struct shim_ion_allocation_data)
#define SHIM_ION_IOC_FREE  _IOWR(SHIM_ION_IOC_MAGIC, 1, struct shim_ion_handle_data)
#define SHIM_ION_IOC_MAP   _IOWR(SHIM_ION_IOC_MAGIC, 2, struct shim_ion_fd_data)
#define SHIM_ION_IOC_SHARE _IOWR(SHIM_ION_IOC_MAGIC, 4, struct shim_ion_fd_data)

#define SHIM_ION_FLAG_CACHED 1
#define SHIM_ION_NUM_HEAP_IDS 32
#define SHIM_ION_ALIGN_BLOCK 0x100000

#define SHIM_MAX_ION_DEVICE_FDS 4
#define SHIM_MAX_ALLOC_RECORDS 16
#define SHIM_MAX_MAPPED_FD_RECORDS 16

typedef int (*ioctl_t)(int, int, ...);
typedef void *(*mmap_t)(void *, size_t, int, int, int, off_t);
typedef int (*open_t)(const char *, int, ...);

static ioctl_t g_orig_ioctl = NULL;
static mmap_t g_orig_mmap = NULL;
static open_t g_orig_open = NULL;
static pthread_once_t g_resolve_once = PTHREAD_ONCE_INIT;

static unsigned int g_forced_heap_mask = 0;
static pthread_once_t g_prop_once = PTHREAD_ONCE_INIT;

static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

static int g_ion_device_fds[SHIM_MAX_ION_DEVICE_FDS];
static int g_ion_device_fd_count = 0;

struct shim_alloc_record {
    int valid;
    shim_ion_user_handle_t handle;
    size_t len;
};
static struct shim_alloc_record g_alloc_records[SHIM_MAX_ALLOC_RECORDS];
static int g_alloc_record_next = 0;

struct shim_mapped_fd_record {
    int valid;
    int fd;
    size_t len;
    int consumed;
};
static struct shim_mapped_fd_record g_mapped_fd_records[SHIM_MAX_MAPPED_FD_RECORDS];
static int g_mapped_fd_record_next = 0;

static void camera_ion_shim_resolve_original(void)
{
    g_orig_ioctl = (ioctl_t)dlsym(RTLD_NEXT, "ioctl");
    g_orig_mmap = (mmap_t)dlsym(RTLD_NEXT, "mmap");
    g_orig_open = (open_t)dlsym(RTLD_NEXT, "open");
    if (g_orig_ioctl == NULL) {
        ALOGE("failed to resolve original ioctl symbol: %s", dlerror());
    }
    if (g_orig_mmap == NULL) {
        ALOGE("failed to resolve original mmap symbol: %s", dlerror());
    }
    if (g_orig_open == NULL) {
        ALOGE("failed to resolve original open symbol: %s", dlerror());
    }
}

static void camera_ion_shim_resolve_property(void)
{
    char value[PROPERTY_VALUE_MAX];
    unsigned long parsed;

    memset(value, 0, sizeof(value));
    property_get("persist.camera.ion.heap_mask", value, "0");
    parsed = strtoul(value, NULL, 0);
    g_forced_heap_mask = (unsigned int)parsed;

    if (g_forced_heap_mask != 0) {
        ALOGI("persist.camera.ion.heap_mask override active: 0x%x", g_forced_heap_mask);
    }
}

static void camera_ion_shim_track_device_fd(int fd)
{
    pthread_mutex_lock(&g_registry_mutex);
    if (g_ion_device_fd_count < SHIM_MAX_ION_DEVICE_FDS) {
        g_ion_device_fds[g_ion_device_fd_count] = fd;
        g_ion_device_fd_count++;
    }
    pthread_mutex_unlock(&g_registry_mutex);
    ALOGI("tracked /dev/ion device fd=%d", fd);
}

static int camera_ion_shim_is_device_fd(int fd)
{
    int i;
    int result = 0;

    pthread_mutex_lock(&g_registry_mutex);
    for (i = 0; i < g_ion_device_fd_count; i++) {
        if (g_ion_device_fds[i] == fd) {
            result = 1;
            break;
        }
    }
    pthread_mutex_unlock(&g_registry_mutex);
    return result;
}

static void camera_ion_shim_record_alloc(shim_ion_user_handle_t handle, size_t len)
{
    pthread_mutex_lock(&g_registry_mutex);
    g_alloc_records[g_alloc_record_next].valid = 1;
    g_alloc_records[g_alloc_record_next].handle = handle;
    g_alloc_records[g_alloc_record_next].len = len;
    g_alloc_record_next = (g_alloc_record_next + 1) % SHIM_MAX_ALLOC_RECORDS;
    pthread_mutex_unlock(&g_registry_mutex);
}

static int camera_ion_shim_lookup_alloc_len(shim_ion_user_handle_t handle, size_t *out_len)
{
    int i;
    int found = 0;

    pthread_mutex_lock(&g_registry_mutex);
    for (i = 0; i < SHIM_MAX_ALLOC_RECORDS; i++) {
        if (g_alloc_records[i].valid && g_alloc_records[i].handle == handle) {
            *out_len = g_alloc_records[i].len;
            found = 1;
        }
    }
    pthread_mutex_unlock(&g_registry_mutex);
    return found;
}

static void camera_ion_shim_record_mapped_fd(int fd, size_t len)
{
    pthread_mutex_lock(&g_registry_mutex);
    g_mapped_fd_records[g_mapped_fd_record_next].valid = 1;
    g_mapped_fd_records[g_mapped_fd_record_next].fd = fd;
    g_mapped_fd_records[g_mapped_fd_record_next].len = len;
    g_mapped_fd_records[g_mapped_fd_record_next].consumed = 0;
    g_mapped_fd_record_next = (g_mapped_fd_record_next + 1) % SHIM_MAX_MAPPED_FD_RECORDS;
    pthread_mutex_unlock(&g_registry_mutex);
}

static int camera_ion_shim_find_mapped_fd_candidate(size_t len, int exclude_fd)
{
    int i;
    int candidate = -1;

    pthread_mutex_lock(&g_registry_mutex);
    for (i = 0; i < SHIM_MAX_MAPPED_FD_RECORDS; i++) {
        if (g_mapped_fd_records[i].valid &&
            !g_mapped_fd_records[i].consumed &&
            g_mapped_fd_records[i].len == len &&
            g_mapped_fd_records[i].fd != exclude_fd) {
            candidate = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_registry_mutex);
    return candidate;
}

static int camera_ion_shim_free_handle(int fd, shim_ion_user_handle_t handle)
{
    struct shim_ion_handle_data free_data;
    memset(&free_data, 0, sizeof(free_data));
    free_data.handle = handle;
    return g_orig_ioctl(fd, SHIM_ION_IOC_FREE, &free_data);
}

static int camera_ion_shim_validate_handle(int fd, size_t len, shim_ion_user_handle_t handle)
{
    struct shim_ion_fd_data map_data;
    void *probe;
    int map_rc;

    memset(&map_data, 0, sizeof(map_data));
    map_data.handle = handle;
    map_rc = g_orig_ioctl(fd, SHIM_ION_IOC_MAP, &map_data);
    if (map_rc < 0) {
        return -1;
    }

    probe = g_orig_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, map_data.fd, 0);
    if (probe == MAP_FAILED) {
        close(map_data.fd);
        return -1;
    }

    munmap(probe, len);
    close(map_data.fd);
    return 0;
}

static int camera_ion_shim_try_combo(int fd, size_t len, size_t align, unsigned int flags,
                                      unsigned int heap_id_mask, shim_ion_user_handle_t *out_handle)
{
    struct shim_ion_allocation_data alloc_data;
    int rc;

    memset(&alloc_data, 0, sizeof(alloc_data));
    alloc_data.len = len;
    alloc_data.align = align;
    alloc_data.heap_id_mask = heap_id_mask;
    alloc_data.flags = flags;

    rc = g_orig_ioctl(fd, SHIM_ION_IOC_ALLOC, &alloc_data);
    if (rc < 0) {
        return -1;
    }

    if (camera_ion_shim_validate_handle(fd, len, alloc_data.handle) != 0) {
        camera_ion_shim_free_handle(fd, alloc_data.handle);
        return -1;
    }

    *out_handle = alloc_data.handle;
    return 0;
}

static size_t camera_ion_shim_round_align(size_t align)
{
    if (align >= SHIM_ION_ALIGN_BLOCK) {
        return align;
    }
    return SHIM_ION_ALIGN_BLOCK;
}

static int camera_ion_shim_grid_search(int fd, size_t len, size_t orig_align, unsigned int orig_flags,
                                        unsigned int skip_mask, shim_ion_user_handle_t *out_handle,
                                        unsigned int *out_heap_id)
{
    unsigned int heap_id;
    size_t align_variants[2];
    unsigned int flags_variants[2];
    int align_count;
    int flags_count;
    int i;
    int j;

    align_variants[0] = orig_align;
    align_variants[1] = camera_ion_shim_round_align(orig_align);
    align_count = (align_variants[0] == align_variants[1]) ? 1 : 2;

    flags_variants[0] = orig_flags;
    flags_variants[1] = orig_flags & ~((unsigned int)SHIM_ION_FLAG_CACHED);
    flags_count = (flags_variants[0] == flags_variants[1]) ? 1 : 2;

    for (heap_id = 0; heap_id < SHIM_ION_NUM_HEAP_IDS; heap_id++) {
        unsigned int candidate_mask = (1u << heap_id);
        if ((skip_mask & candidate_mask) != 0) {
            continue;
        }
        for (i = 0; i < align_count; i++) {
            for (j = 0; j < flags_count; j++) {
                if (camera_ion_shim_try_combo(fd, len, align_variants[i], flags_variants[j],
                                               candidate_mask, out_handle) == 0) {
                    *out_heap_id = heap_id;
                    return 0;
                }
            }
        }
    }

    return -1;
}

extern int open(const char *pathname, int flags, ...)
{
    va_list ap;
    mode_t mode;
    int fd;

    pthread_once(&g_resolve_once, camera_ion_shim_resolve_original);

    if (g_orig_open == NULL) {
        errno = ENOSYS;
        return -1;
    }

    mode = 0;
    if ((flags & O_CREAT) != 0) {
        va_start(ap, flags);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
        fd = g_orig_open(pathname, flags, mode);
    } else {
        fd = g_orig_open(pathname, flags);
    }

    if (fd >= 0 && pathname != NULL && strstr(pathname, "/dev/ion") != NULL) {
        camera_ion_shim_track_device_fd(fd);
    }

    return fd;
}

extern int ioctl(int fd, int request, ...)
{
    va_list ap;
    void *arg;

    pthread_once(&g_resolve_once, camera_ion_shim_resolve_original);

    va_start(ap, request);
    arg = va_arg(ap, void *);
    va_end(ap);

    if (g_orig_ioctl == NULL) {
        errno = ENOSYS;
        return -1;
    }

    if (request == SHIM_ION_IOC_MAP || request == SHIM_ION_IOC_SHARE) {
        struct shim_ion_fd_data *fd_data = (struct shim_ion_fd_data *)arg;
        int rc = g_orig_ioctl(fd, request, arg);
        if (rc >= 0) {
            size_t len;
            if (camera_ion_shim_lookup_alloc_len(fd_data->handle, &len)) {
                camera_ion_shim_record_mapped_fd(fd_data->fd, len);
                ALOGI("tracked mapped fd=%d handle=%d len=%zu via %s",
                      fd_data->fd, fd_data->handle, len,
                      request == SHIM_ION_IOC_MAP ? "ION_IOC_MAP" : "ION_IOC_SHARE");
            }
        }
        return rc;
    }

    if (request != SHIM_ION_IOC_ALLOC) {
        return g_orig_ioctl(fd, request, arg);
    }

    pthread_once(&g_prop_once, camera_ion_shim_resolve_property);

    {
        struct shim_ion_allocation_data *req = (struct shim_ion_allocation_data *)arg;
        int rc = g_orig_ioctl(fd, request, arg);
        shim_ion_user_handle_t candidate_handle;
        unsigned int found_heap_id;

        if (rc >= 0 && camera_ion_shim_validate_handle(fd, req->len, req->handle) == 0) {
            camera_ion_shim_record_alloc(req->handle, req->len);
            return rc;
        }

        if (rc >= 0) {
            ALOGE("baseline alloc succeeded but mmap validation failed heap_id_mask=0x%x len=%zu align=%zu flags=0x%x, retrying",
                  req->heap_id_mask, req->len, req->align, req->flags);
            camera_ion_shim_free_handle(fd, req->handle);
        } else {
            ALOGE("baseline ION_IOC_ALLOC failed heap_id_mask=0x%x len=%zu align=%zu flags=0x%x errno=%d, retrying",
                  req->heap_id_mask, req->len, req->align, req->flags, errno);
        }

        if (g_forced_heap_mask != 0 && g_forced_heap_mask != req->heap_id_mask) {
            if (camera_ion_shim_try_combo(fd, req->len, req->align, req->flags, g_forced_heap_mask, &candidate_handle) == 0) {
                ALOGI("forced heap_mask=0x%x worked for len=%zu", g_forced_heap_mask, req->len);
                req->handle = candidate_handle;
                camera_ion_shim_record_alloc(candidate_handle, req->len);
                errno = 0;
                return 0;
            }
            ALOGE("forced heap_mask=0x%x failed for len=%zu, falling back to grid search", g_forced_heap_mask, req->len);
        }

        if (camera_ion_shim_grid_search(fd, req->len, req->align, req->flags, req->heap_id_mask,
                                         &candidate_handle, &found_heap_id) == 0) {
            ALOGI("grid search succeeded heap_id=%u len=%zu", found_heap_id, req->len);
            req->handle = candidate_handle;
            camera_ion_shim_record_alloc(candidate_handle, req->len);
            errno = 0;
            return 0;
        }

        ALOGE("no heap_id/align/flags combination could satisfy alloc+mmap for len=%zu", req->len);
        errno = ENODEV;
        return -1;
    }
}

extern void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    void *result;
    int candidate_index;
    int candidate_fd;
    int is_device_fd;

    pthread_once(&g_resolve_once, camera_ion_shim_resolve_original);

    if (g_orig_mmap == NULL) {
        errno = ENOSYS;
        return MAP_FAILED;
    }

    result = g_orig_mmap(addr, length, prot, flags, fd, offset);
    if (result != MAP_FAILED) {
        return result;
    }

    if (errno != ENODEV) {
        return result;
    }

    is_device_fd = camera_ion_shim_is_device_fd(fd);
    ALOGE("mmap ENODEV fd=%d length=%zu is_ion_device_fd=%d, searching mapped-fd registry for substitution",
          fd, length, is_device_fd);

    candidate_index = camera_ion_shim_find_mapped_fd_candidate(length, fd);
    if (candidate_index < 0) {
        ALOGE("no registry candidate found for mmap ENODEV fd=%d length=%zu", fd, length);
        errno = ENODEV;
        return MAP_FAILED;
    }

    pthread_mutex_lock(&g_registry_mutex);
    candidate_fd = g_mapped_fd_records[candidate_index].fd;
    pthread_mutex_unlock(&g_registry_mutex);

    result = g_orig_mmap(addr, length, prot, flags, candidate_fd, offset);
    if (result == MAP_FAILED) {
        ALOGE("mmap fd substitution candidate_fd=%d also failed errno=%d", candidate_fd, errno);
        errno = ENODEV;
        return MAP_FAILED;
    }

    pthread_mutex_lock(&g_registry_mutex);
    g_mapped_fd_records[candidate_index].consumed = 1;
    pthread_mutex_unlock(&g_registry_mutex);

    ALOGI("mmap ENODEV recovered via fd substitution original_fd=%d candidate_fd=%d length=%zu",
          fd, candidate_fd, length);

    return result;
}

static void camera_ion_shim_log_process_identity(void)
{
    int fd;
    char cmdline[256];
    ssize_t n;

    memset(cmdline, 0, sizeof(cmdline));
    fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd >= 0) {
        n = read(fd, cmdline, sizeof(cmdline) - 1);
        if (n < 0) {
            n = 0;
        }
        cmdline[n] = '\0';
        close(fd);
    } else {
        strcpy(cmdline, "(unknown)");
    }

    ALOGI("libcamera_ion_shim loaded into pid=%d process=\"%s\"", getpid(), cmdline);
}

__attribute__((constructor)) static void camera_ion_shim_ctor(void)
{
    camera_ion_shim_log_process_identity();
}
