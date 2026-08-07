#define LOG_TAG "CameraParamsShim"
#define _GNU_SOURCE 1
#include <utils/Log.h>
#include <dlfcn.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/ioctl.h>
#include <stdarg.h>

namespace android {
class CameraParameters {
public:
    static const char PIXEL_FORMAT_YUV420P_I420[];
    __attribute__((visibility("default")))
    const char *get(const char *key) const;
};
const char CameraParameters::PIXEL_FORMAT_YUV420P_I420[] = "yuv420p";
}
typedef const char *(*camera_parameters_get_t)(const void *thiz, const char *key);
static camera_parameters_get_t g_orig_camera_parameters_get = NULL;
static pthread_once_t g_resolve_once = PTHREAD_ONCE_INIT;
static void camera_params_shim_resolve_original(void)
{
    g_orig_camera_parameters_get = reinterpret_cast<camera_parameters_get_t>(
        dlsym(RTLD_NEXT, "_ZNK7android15CameraParameters3getEPKc"));
    if (g_orig_camera_parameters_get == NULL) {
        ALOGE("CameraParametersShim: failed to resolve original CameraParameters::get symbol: %s", dlerror());
    } else {
        ALOGI("CameraParametersShim: resolved original CameraParameters::get symbol");
    }
}
namespace android {
const char *CameraParameters::get(const char *key) const
{
    pthread_once(&g_resolve_once, camera_params_shim_resolve_original);
    const char *original_value = NULL;
    if (g_orig_camera_parameters_get != NULL) {
        original_value = g_orig_camera_parameters_get(this, key);
    }
    if (original_value != NULL && strcmp(original_value, "yuv422i-uyvy") == 0) {
        ALOGI("CameraParametersShim: remapping unsupported color format \"yuv422i-uyvy\" to \"yuv422i-yuyv\" for key \"%s\"", key != NULL ? key : "(null)");
        return "yuv422i-yuyv";
    }
    return original_value;
}
}

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

#define SHIM_ION_NUM_HEAP_IDS 32

typedef int (*ioctl_t)(int, int, ...);
typedef void *(*mmap_t)(void *, size_t, int, int, int, off_t);

static ioctl_t g_orig_ioctl = NULL;
static mmap_t g_orig_mmap = NULL;
static pthread_once_t g_ion_resolve_once = PTHREAD_ONCE_INIT;

static void camera_ion_shim_resolve_original(void)
{
    g_orig_ioctl = reinterpret_cast<ioctl_t>(dlsym(RTLD_NEXT, "ioctl"));
    g_orig_mmap = reinterpret_cast<mmap_t>(dlsym(RTLD_NEXT, "mmap"));
    if (g_orig_ioctl == NULL) {
        ALOGE("CameraIonShim: failed to resolve original ioctl symbol: %s", dlerror());
    }
    if (g_orig_mmap == NULL) {
        ALOGE("CameraIonShim: failed to resolve original mmap symbol: %s", dlerror());
    }
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
        ALOGE("CameraIonShim: ION_IOC_MAP failed for handle=%d len=%zu errno=%d", handle, len, errno);
        return -1;
    }

    probe = g_orig_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, map_data.fd, 0);
    if (probe == MAP_FAILED) {
        ALOGE("CameraIonShim: probe mmap failed for handle=%d fd=%d len=%zu errno=%d", handle, map_data.fd, len, errno);
        close(map_data.fd);
        return -1;
    }

    munmap(probe, len);
    close(map_data.fd);
    return 0;
}

static int camera_ion_shim_try_heap(int fd, size_t len, size_t align, unsigned int flags,
                                     unsigned int heap_id, shim_ion_user_handle_t *out_handle)
{
    struct shim_ion_allocation_data alloc_data;
    int rc;

    memset(&alloc_data, 0, sizeof(alloc_data));
    alloc_data.len = len;
    alloc_data.align = align;
    alloc_data.heap_id_mask = (1u << heap_id);
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

extern "C" int ioctl(int fd, int request, ...)
{
    va_list ap;
    void *arg;

    pthread_once(&g_ion_resolve_once, camera_ion_shim_resolve_original);

    va_start(ap, request);
    arg = va_arg(ap, void *);
    va_end(ap);

    if (g_orig_ioctl == NULL) {
        errno = ENOSYS;
        return -1;
    }

    if (request != SHIM_ION_IOC_ALLOC) {
        return g_orig_ioctl(fd, request, arg);
    }

    struct shim_ion_allocation_data *req = reinterpret_cast<struct shim_ion_allocation_data *>(arg);
    int rc = g_orig_ioctl(fd, request, arg);

    if (rc >= 0 && camera_ion_shim_validate_handle(fd, req->len, req->handle) == 0) {
        return rc;
    }

    if (rc >= 0) {
        ALOGE("CameraIonShim: alloc succeeded but mmap validation failed, heap_id_mask=0x%x len=%zu, freeing and scanning",
              req->heap_id_mask, req->len);
        camera_ion_shim_free_handle(fd, req->handle);
    } else {
        ALOGE("CameraIonShim: ION_IOC_ALLOC failed heap_id_mask=0x%x len=%zu errno=%d, scanning heaps",
              req->heap_id_mask, req->len, errno);
    }

    unsigned int heap_id;
    for (heap_id = 0; heap_id < SHIM_ION_NUM_HEAP_IDS; heap_id++) {
        if ((req->heap_id_mask & (1u << heap_id)) != 0) {
            continue;
        }
        shim_ion_user_handle_t candidate_handle;
        if (camera_ion_shim_try_heap(fd, req->len, req->align, req->flags, heap_id, &candidate_handle) == 0) {
            ALOGI("CameraIonShim: fallback heap_id=%u works for len=%zu", heap_id, req->len);
            req->handle = candidate_handle;
            errno = 0;
            return 0;
        }
    }

    ALOGE("CameraIonShim: no ION heap could satisfy alloc+mmap for len=%zu", req->len);
    errno = ENODEV;
    return -1;
}

extern "C" __attribute__((constructor)) void camera_params_shim_ctor(void)
{
    ALOGI("libcamera_params_shim loaded: android::CameraParameters::PIXEL_FORMAT_YUV420P_I420 resolved");
}
