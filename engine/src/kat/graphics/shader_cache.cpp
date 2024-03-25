#include "kat/graphics/shader_cache.hpp"

#include <fstream>
#include <iostream>

namespace kat {
    std::vector<uint32_t> ShaderId::load_code() const {
        std::ifstream f(path, std::ios::ate | std::ios::in | std::ios::binary);

        if (!f.good()) {
            std::cerr << "File not found: " << path << std::endl;
            throw kat::fatal_exc{};
        }

        auto end = f.tellg();
        f.seekg(0);

        std::vector<uint32_t> code;
        code.resize(end / sizeof(uint32_t));

        f.read(reinterpret_cast<char *>(code.data()), code.size() * sizeof(uint32_t));

        f.close();

        return code;
    }

    ShaderCache::ShaderCache(const std::shared_ptr<kat::Context> &context) : m_context(context) {}

    ShaderCache::~ShaderCache() {
        for (const auto &sm : m_cache) {
            m_context->device().destroy(sm.second);
        }
    }

    vk::ShaderModule ShaderCache::get(const ShaderId &id) {
        auto it = m_cache.find(id);
        if (it != m_cache.end()) {
            return it->second;
        }

        // Load shader

        std::vector<uint32_t> code = id.load_code();

        vk::ShaderModuleCreateInfo ci{};
        ci.setCode(code);

        vk::ShaderModule sm = m_context->device().createShaderModule(ci);

        m_cache.insert({id, sm});

        return sm;
    }

    std::optional<vk::ShaderModule> ShaderCache::get_if_present(const ShaderId &id) {
        auto it = m_cache.find(id);
        if (it != m_cache.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    void ShaderCache::reset() {
        for (const auto &sm : m_cache) {
            m_context->device().destroy(sm.second);
        }

        m_cache.clear();
    }

    bool ShaderCache::is_loaded(const ShaderId &id) {
        return m_cache.contains(id);
    }
} // namespace kat
