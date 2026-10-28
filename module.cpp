#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <android/log.h>
#include <limits.h>
#include <stdio.h>
#include <cstring>
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

    // 1. فلترة التطبيقات: نعمل فقط على لعبة بوكيمون جو
    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto nice_name = env->GetStringUTFChars(args->nice_name, nullptr);
        if (strcmp(nice_name, "com.nianticlabs.pokemongo") != 0) {
            // إذا لم يكن التطبيق هو اللعبة، قم بإغلاق الوحدة لهذا التطبيق
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
        env->ReleaseStringUTFChars(args->nice_name, nice_name);
    }

    // 2. حقن المكتبة داخل اللعبة
    void postAppSpecialize(const AppSpecializeArgs *args) override {
        LOGD("Injected into Pokemon GO, loading Gadget...");
        
        int dir_fd = api->getModuleDir();
        if (dir_fd < 0) {
            LOGE("Failed to get module directory FD");
            return;
        }

        int fd = openat(dir_fd, "libgadget.so", O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            LOGE("Failed to open libgadget.so");
            close(dir_fd);
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
        close(dir_fd);
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(GadgetModule)
