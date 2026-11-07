// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/android/android_common.h"
#include "common/android/id_cache.h"
#include "common/assert.h"
#include "common/fs/fs_android.h"
#include "common/string_util.h"

namespace Common::FS::Android {

void RegisterCallbacks(JNIEnv* env, jclass clazz) {
    env->GetJavaVM(&g_jvm);
    native_library = clazz;

    s_get_parent_directory = env->GetStaticMethodID(native_library, "getParentDirectory",
                                                    "(Ljava/lang/String;)Ljava/lang/String;");
    s_get_filename = env->GetStaticMethodID(native_library, "getFilename",
                                            "(Ljava/lang/String;)Ljava/lang/String;");
    s_get_size = env->GetStaticMethodID(native_library, "getSize", "(Ljava/lang/String;)J");
    s_is_directory = env->GetStaticMethodID(native_library, "isDirectory", "(Ljava/lang/String;)Z");
    s_file_exists = env->GetStaticMethodID(native_library, "exists", "(Ljava/lang/String;)Z");
    s_open_content_uri = env->GetStaticMethodID(native_library, "openContentUri",
                                                "(Ljava/lang/String;Ljava/lang/String;)I");
}

void UnRegisterCallbacks() {
    s_get_parent_directory = nullptr;
    s_get_filename = nullptr;

    s_get_size = nullptr;
    s_is_directory = nullptr;
    s_file_exists = nullptr;

    s_open_content_uri = nullptr;
}

bool IsContentUri(const std::string& path) {
    constexpr std::string_view prefix = "content://";
    if (path.size() < prefix.size()) [[unlikely]] {
        return false;
    }

    return path.find(prefix) == 0;
}

s32 OpenContentUri(const std::string& filepath, OpenMode openmode) {
    if (s_open_content_uri == nullptr)
        return -1;

    const char* mode = "";
    switch (openmode) {
    case OpenMode::Read:
        mode = "r";
        break;
    default:
        UNIMPLEMENTED();
        return -1;
    }
    auto env = Common::Android::GetEnvForThread();
    jstring j_filepath = Common::Android::ToJString(env, filepath);
    jstring j_mode = Common::Android::ToJString(env, mode);
    s32 fd = env->CallStaticIntMethod(native_library, s_open_content_uri, j_filepath, j_mode);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        fd = -1;
    }
    if (j_filepath) {
        env->DeleteLocalRef(j_filepath);
    }
    if (j_mode) {
        env->DeleteLocalRef(j_mode);
    }
    return fd;
}

u64 GetSize(const std::string& filepath) {
    if (s_get_size == nullptr) {
        return 0;
    }
    auto env = Common::Android::GetEnvForThread();
    jstring j_filepath = Common::Android::ToJString(env, filepath);
    jlong size = env->CallStaticLongMethod(native_library, s_get_size, j_filepath);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        size = 0;
    }
    if (j_filepath) {
        env->DeleteLocalRef(j_filepath);
    }
    return static_cast<u64>(size);
}

bool IsDirectory(const std::string& filepath) {
    if (s_is_directory == nullptr) {
        return false;
    }
    auto env = Common::Android::GetEnvForThread();
    jstring j_filepath = Common::Android::ToJString(env, filepath);
    jboolean is_dir = env->CallStaticBooleanMethod(native_library, s_is_directory, j_filepath);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        is_dir = JNI_FALSE;
    }
    if (j_filepath) {
        env->DeleteLocalRef(j_filepath);
    }
    return static_cast<bool>(is_dir);
}

bool Exists(const std::string& filepath) {
    if (s_file_exists == nullptr) {
        return false;
    }
    auto env = Common::Android::GetEnvForThread();
    jstring j_filepath = Common::Android::ToJString(env, filepath);
    jboolean exists = env->CallStaticBooleanMethod(native_library, s_file_exists, j_filepath);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        exists = JNI_FALSE;
    }
    if (j_filepath) {
        env->DeleteLocalRef(j_filepath);
    }
    return static_cast<bool>(exists);
}

std::string GetParentDirectory(const std::string& filepath) {
    if (s_get_parent_directory == nullptr) {
        return {};
    }
    auto env = Common::Android::GetEnvForThread();
    jstring j_filepath = Common::Android::ToJString(env, filepath);
    jstring j_return = static_cast<jstring>(env->CallStaticObjectMethod(
        native_library, s_get_parent_directory, j_filepath));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (j_filepath) {
            env->DeleteLocalRef(j_filepath);
        }
        return {};
    }
    if (j_filepath) {
        env->DeleteLocalRef(j_filepath);
    }
    if (!j_return) {
        return {};
    }
    std::string result = Common::Android::GetJString(env, j_return);
    env->DeleteLocalRef(j_return);
    return result;
}

std::string GetFilename(const std::string& filepath) {
    if (s_get_filename == nullptr) {
        return {};
    }
    auto env = Common::Android::GetEnvForThread();
    jstring j_filepath = Common::Android::ToJString(env, filepath);
    jstring j_return = static_cast<jstring>(env->CallStaticObjectMethod(
        native_library, s_get_filename, j_filepath));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (j_filepath) {
            env->DeleteLocalRef(j_filepath);
        }
        return {};
    }
    if (j_filepath) {
        env->DeleteLocalRef(j_filepath);
    }
    if (!j_return) {
        return {};
    }
    std::string result = Common::Android::GetJString(env, j_return);
    env->DeleteLocalRef(j_return);
    return result;
}

} // namespace Common::FS::Android
