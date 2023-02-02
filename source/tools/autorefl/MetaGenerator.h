
#ifndef _METADOT_METAGENERATOR_H_
#define _METADOT_METAGENERATOR_H_

#include "Meta.h"

#include <memory>

namespace MetaEngine::StaticRefl {
    class MetaGenerator {
    public:
		MetaGenerator();
		~MetaGenerator();
		MetaGenerator(MetaGenerator&&) noexcept;
        MetaGenerator& operator=(MetaGenerator&&) noexcept;

        std::vector<TypeMeta> Parse(std::string_view cppCode);
    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
    };
}

#endif