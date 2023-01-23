#pragma once
#include "cJSONPP.h"

namespace cjsonpp {

class Validator {
public:
    Validator() {}
    ~Validator() {}

    bool validate(const Json& data, const Json& schema);

    const std::string& error() { return error_; }

private:
    bool verifyValue(const Json& value, const char* opt);
    std::string error_;
};

}  // namespace cjsonpp
