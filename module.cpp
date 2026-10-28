#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <android/log.h>
#include <limits.h>
#include <stdio.h>
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

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        // 1. الحصول على واصف الملف (FD) لمجلد الوحدة (يتجاوز قيود SELinux)
        int dir_fd = api->getModuleDir();
        if (dir_fd < 0) {
            LOGE("Failed to get module directory FD");
            return;
        }

        // 2. فتح المكتبة باستخدام openat
        int fd = openat(dir_fd, "libgadget.so", O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            LOGE("Failed to open libgadget.so");
            close(dir_fd);
            return;
        }

        // 3. استخدام مسار /proc/self/fd لتجاوز قيود المسار
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

        // 4. تحميل المكتبة في الذاكرة
        void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            LOGE("dlopen failed: %s", dlerror());
        } else {
            LOGD("Successfully loaded libgadget.so");
        }
        
        // إغلاق الواصفات
        close(fd);
        close(dir_fd);
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(GadgetModule)
