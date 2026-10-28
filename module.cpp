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
        int dir_fd = api->getModuleDir();
        
        if (dir_fd >= 0) {
            fd = openat(dir_fd, "libgadget.so", O_RDONLY | O_CLOEXEC);
            close(dir_fd);
        }
        
        // خطة بديلة 1: فتح المسار المباشر
        if (fd < 0) {
            LOGD("getModuleDir failed, trying absolute path...");
            fd = open("/data/adb/modules/pogogadget/libgadget.so", O_RDONLY | O_CLOEXEC);
        }

        // خطة بديلة 2: مسار آخر محتمل
        if (fd < 0) {
            LOGD("Absolute path failed, trying module path...");
            fd = open("/data/adb/modules/pogogadget/zygisk/libgadget.so", O_RDONLY | O_CLOEXEC);
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
};

REGISTER_ZYGISK_MODULE(GadgetModule)
