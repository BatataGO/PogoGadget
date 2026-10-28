#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <android/log.h>
#include <limits.h>
#include <stdio.h>
#include <cstring>
#include <cerrno>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "PogoGadget", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PogoGadget", __VA_ARGS__)

class GadgetModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        // جلب واصف الملف (FD) هنا (داخل onLoad حيث نملك صلاحيات الروت) وحفظه
        this->module_dir_fd = api->getModuleDir();
        if (this->module_dir_fd < 0) {
            LOGE("Failed to get module directory FD in onLoad");
        }
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto nice_name = env->GetStringUTFChars(args->nice_name, nullptr);
        if (strcmp(nice_name, "com.nianticlabs.pokemongo") != 0) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
        env->ReleaseStringUTFChars(args->nice_name, nice_name);
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        LOGD("Injected into Pokemon GO, loading Gadget...");
        
        int fd = -1;
        
        // استخدام واصف الملف الذي حصلنا عليه في onLoad
        if (module_dir_fd >= 0) {
            fd = openat(module_dir_fd, "libgadget.so", O_RDONLY | O_CLOEXEC);
        } else {
            // خطة بديلة في حال فشل getModuleDir
            LOGD("module_dir_fd is invalid, trying absolute path...");
            fd = open("/data/adb/modules/pogogadget/libgadget.so", O_RDONLY | O_CLOEXEC);
        }

        if (fd < 0) {
            LOGE("Failed to open libgadget.so. Errno: %d", errno);
            return;
        }

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

        void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            LOGE("dlopen failed: %s", dlerror());
        } else {
            LOGD("Successfully loaded libgadget.so");
        }
        
        close(fd);
    }

private:
    Api *api;
    JNIEnv *env;
    int module_dir_fd = -1;
};

REGISTER_ZYGISK_MODULE(GadgetModule)
