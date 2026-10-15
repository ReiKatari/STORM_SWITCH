// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <set>
#include <ankerl/unordered_dense.h>
#include <utility>
#include "core/file_sys/vfs/vfs_layered.h"

namespace FileSys {

namespace {

std::string ToLowerAscii(std::string_view str) {
    std::string out(str);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

class CanonicalNamedVfsFile final : public VfsFile {
public:
    CanonicalNamedVfsFile(VirtualFile base_, std::string canonical_name_)
        : base(std::move(base_)), canonical_name(std::move(canonical_name_)) {}

    std::string GetName() const override {
        return canonical_name;
    }

    std::size_t GetSize() const override {
        return base->GetSize();
    }

    bool Resize(std::size_t new_size) override {
        return base->Resize(new_size);
    }

    VirtualDir GetContainingDirectory() const override {
        return base->GetContainingDirectory();
    }

    bool IsWritable() const override {
        return base->IsWritable();
    }

    bool IsReadable() const override {
        return base->IsReadable();
    }

    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override {
        return base->Read(data, length, offset);
    }

    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override {
        return base->Write(data, length, offset);
    }

    bool Rename(std::string_view new_name) override {
        canonical_name = new_name;
        return true;
    }

private:
    VirtualFile base;
    std::string canonical_name;
};

VirtualDir FindSubdirectoryCaseInsensitive(const VirtualDir& dir, std::string_view comp) {
    if (!dir) {
        return nullptr;
    }
    if (auto exact = dir->GetSubdirectory(comp); exact != nullptr) {
        return exact;
    }
    for (const auto& sd : dir->GetSubdirectories()) {
        if (EqualsIgnoreCase(sd->GetName(), comp)) {
            return sd;
        }
    }
    return nullptr;
}

VirtualDir FindDirectoryRelativeCaseInsensitive(const VirtualDir& root, std::string_view rel_path) {
    if (!root) {
        return nullptr;
    }
    if (auto exact = root->GetDirectoryRelative(rel_path); exact != nullptr) {
        return exact;
    }
    VirtualDir current = root;
    std::size_t pos = 0;
    while (pos < rel_path.size() && current != nullptr) {
        while (pos < rel_path.size() && (rel_path[pos] == '/' || rel_path[pos] == '\\')) {
            ++pos;
        }
        if (pos >= rel_path.size()) {
            break;
        }
        const std::size_t next_sep = rel_path.find_first_of("/\\", pos);
        const std::string_view comp =
            (next_sep == std::string_view::npos) ? rel_path.substr(pos) : rel_path.substr(pos, next_sep - pos);
        current = FindSubdirectoryCaseInsensitive(current, comp);
        if (next_sep == std::string_view::npos) {
            break;
        }
        pos = next_sep + 1;
    }
    return current;
}

VirtualFile FindFileRelativeCaseInsensitive(const VirtualDir& root, std::string_view rel_path) {
    if (!root) {
        return nullptr;
    }
    if (auto exact = root->GetFileRelative(rel_path); exact != nullptr) {
        return exact;
    }
    while (!rel_path.empty() && (rel_path.front() == '/' || rel_path.front() == '\\')) {
        rel_path.remove_prefix(1);
    }
    if (rel_path.empty()) {
        return nullptr;
    }
    const std::size_t last_sep = rel_path.find_last_of("/\\");
    VirtualDir target_dir = root;
    std::string_view file_comp = rel_path;
    if (last_sep != std::string_view::npos) {
        target_dir = FindDirectoryRelativeCaseInsensitive(root, rel_path.substr(0, last_sep));
        file_comp = rel_path.substr(last_sep + 1);
    }
    if (!target_dir) {
        return nullptr;
    }
    if (auto exact = target_dir->GetFile(file_comp); exact != nullptr) {
        return exact;
    }
    for (const auto& f : target_dir->GetFiles()) {
        if (EqualsIgnoreCase(f->GetName(), file_comp)) {
            return f;
        }
    }
    return nullptr;
}

} // Anonymous namespace

LayeredVfsDirectory::LayeredVfsDirectory(std::vector<VirtualDir> dirs_, std::string name_)
    : dirs(std::move(dirs_)), name(std::move(name_)) {}

LayeredVfsDirectory::~LayeredVfsDirectory() = default;

VirtualDir LayeredVfsDirectory::MakeLayeredDirectory(std::vector<VirtualDir> dirs,
                                                     std::string name) {
    if (dirs.empty())
        return nullptr;
    if (dirs.size() == 1 && (name.empty() || dirs[0]->GetName() == name))
        return dirs[0];

    return VirtualDir(new LayeredVfsDirectory(std::move(dirs), std::move(name)));
}

VirtualFile LayeredVfsDirectory::GetFileRelative(std::string_view path) const {
    for (const auto& layer : dirs) {
        const auto file = FindFileRelativeCaseInsensitive(layer, path);
        if (file != nullptr)
            return file;
    }

    return nullptr;
}

VirtualDir LayeredVfsDirectory::GetDirectoryRelative(std::string_view path) const {
    std::vector<VirtualDir> out;
    for (const auto& layer : dirs) {
        auto dir = FindDirectoryRelativeCaseInsensitive(layer, path);
        if (dir != nullptr) {
            out.emplace_back(std::move(dir));
        }
    }
    if (out.empty()) {
        return nullptr;
    }

    // Preserve the base layer's canonical directory name when overriding
    std::string canonical_name = out.back()->GetName();
    return MakeLayeredDirectory(std::move(out), std::move(canonical_name));
}

VirtualFile LayeredVfsDirectory::GetFile(std::string_view file_name) const {
    return GetFileRelative(file_name);
}

VirtualDir LayeredVfsDirectory::GetSubdirectory(std::string_view subdir_name) const {
    return GetDirectoryRelative(subdir_name);
}

std::string LayeredVfsDirectory::GetFullPath() const {
    return dirs[0]->GetFullPath();
}

std::vector<VirtualFile> LayeredVfsDirectory::GetFiles() const {
    // First collect canonical file names from lowest priority (base RomFS) to highest priority
    ankerl::unordered_dense::map<std::string, std::string> canonical_names;
    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
        for (const auto& file : (*it)->GetFiles()) {
            auto fname = file->GetName();
            canonical_names.try_emplace(ToLowerAscii(fname), std::move(fname));
        }
    }

    std::vector<VirtualFile> out;
    ankerl::unordered_dense::set<std::string> out_names;

    for (const auto& layer : dirs) {
        for (auto& file : layer->GetFiles()) {
            const std::string fname = file->GetName();
            const std::string lower_name = ToLowerAscii(fname);
            const auto [it, is_new] = out_names.emplace(lower_name);
            if (is_new) {
                const auto canon_it = canonical_names.find(lower_name);
                if (canon_it != canonical_names.end() && canon_it->second != fname) {
                    out.emplace_back(
                        std::make_shared<CanonicalNamedVfsFile>(std::move(file), canon_it->second));
                } else {
                    out.emplace_back(std::move(file));
                }
            }
        }
    }

    return out;
}

std::vector<VirtualDir> LayeredVfsDirectory::GetSubdirectories() const {
    // Collect canonical subdirectory names from base layer first so case-mismatched mod folders
    // merge cleanly into the base RomFS's canonical directory name.
    ankerl::unordered_dense::map<std::string, std::string> canonical_names;
    std::vector<std::string> ordered_lower_names;

    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
        for (const auto& sd : (*it)->GetSubdirectories()) {
            auto sd_name = sd->GetName();
            auto lower = ToLowerAscii(sd_name);
            if (canonical_names.try_emplace(lower, std::move(sd_name)).second) {
                ordered_lower_names.push_back( std::move(lower));
            }
        }
    }

    std::vector<VirtualDir> out;
    out.reserve(ordered_lower_names.size());
    for (const auto& lower : ordered_lower_names) {
        const auto& canon_name = canonical_names[lower];
        if (auto subdir = GetSubdirectory(canon_name); subdir != nullptr) {
            out.emplace_back(std::move(subdir));
        }
    }

    return out;
}

bool LayeredVfsDirectory::IsWritable() const {
    return false;
}

bool LayeredVfsDirectory::IsReadable() const {
    return true;
}

std::string LayeredVfsDirectory::GetName() const {
    return name.empty() ? dirs.back()->GetName() : name;
}

VirtualDir LayeredVfsDirectory::GetParentDirectory() const {
    return dirs[0]->GetParentDirectory();
}

VirtualDir LayeredVfsDirectory::CreateSubdirectory(std::string_view subdir_name) {
    return nullptr;
}

VirtualFile LayeredVfsDirectory::CreateFile(std::string_view file_name) {
    return nullptr;
}

bool LayeredVfsDirectory::DeleteSubdirectory(std::string_view subdir_name) {
    return false;
}

bool LayeredVfsDirectory::DeleteFile(std::string_view file_name) {
    return false;
}

bool LayeredVfsDirectory::Rename(std::string_view new_name) {
    name = new_name;
    return true;
}

} // namespace FileSys
