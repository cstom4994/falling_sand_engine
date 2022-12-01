
#ifndef _METADOT_ARCHIVER_HPP_
#define _METADOT_ARCHIVER_HPP_

#include "Properties.hpp"

namespace Meta::properties {

    class properties;

    /**
	 * Built-in archiver for (de)serialization to/from JSON.
	 *
	 * @details This implementation uses `nlohmann::json`.
	 */
    class archiver_json : public archiver {
    public:
        [[nodiscard]] std::string save(const properties &p) const override;

        std::pair<bool, std::string> load(properties &p, const std::string &str) const override;

    private:
        static void write_recursively();
        static void read_recursively();
    };

}// namespace Meta::properties

#endif