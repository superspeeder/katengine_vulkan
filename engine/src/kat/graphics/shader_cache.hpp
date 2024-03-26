#pragma once

#include "kat/graphics/context.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace kat {
    struct ShaderId {
        // For now, we are just loading shaders from spirv files. In the future this might be upgraded to a shader database, which would output blobs given some other identifier.
        // In preparation, we will use this as an inbetween for caching ids, to reduce necessary refactoring.
        std::string path;

        inline ShaderId(const std::string &path_) : path(path_) {};
        inline ShaderId(const char* path_) : path(path_) {};

        inline ShaderId(const std::filesystem::path& path_) : path(path_.string()) {};

        std::vector<uint32_t> load_code() const;

        inline friend bool operator==(const ShaderId &a, const ShaderId &b) { return a.path == b.path; };
    };
} // namespace kat

template <>
struct std::hash<kat::ShaderId> {
    std::size_t operator()(const kat::ShaderId &id) const { return std::hash<std::string>()(id.path); }
};

namespace kat {

    // In the future, instead of an unordered_map, we should use an lfu cache.
    class ShaderCache {
      public:
        explicit ShaderCache(const std::shared_ptr<kat::Context> &context);

        ~ShaderCache();

        vk::ShaderModule get(const ShaderId &id);

        std::optional<vk::ShaderModule> get_if_present(const ShaderId &id);

        void reset();

        bool is_loaded(const ShaderId &id);

      private:
        std::shared_ptr<kat::Context> m_context;

        std::unordered_map<ShaderId, vk::ShaderModule> m_cache;
    };
} // namespace kat
