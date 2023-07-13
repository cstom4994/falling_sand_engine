#include "nbt.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "engine/core/core.hpp"
#include "engine/core/sdl_wrapper.h"

static inline constexpr uint16_t swap_u16(uint16_t x) { return (x >> 8) | (x << 8); }

static inline constexpr uint32_t swap_u32(uint32_t x) {
    x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
    return (x << 16) | (x >> 16);
}

static inline constexpr uint64_t swap_u64(uint64_t x) {
    x = ((x << 8) & 0xFF00FF00FF00FF00ULL) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
    x = ((x << 16) & 0xFFFF0000FFFF0000ULL) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
    return (x << 32) | (x >> 32);
}

static inline int16_t swap_i16(int16_t x) {
    uint16_t const tmp = swap_u16(reinterpret_cast<uint16_t&>(x));
    return reinterpret_cast<int16_t const&>(tmp);
}

static inline int32_t swap_i32(int32_t x) {
    uint32_t const tmp = swap_u32(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<int32_t const&>(tmp);
}

static inline int64_t swap_i64(int64_t x) {
    uint64_t const tmp = swap_u64(reinterpret_cast<uint64_t&>(x));
    return reinterpret_cast<int64_t const&>(tmp);
}

static inline float swap_f32(float x) {
    uint32_t const tmp = swap_u32(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<float const&>(tmp);
}

static inline double swap_f64(double x) {
    uint64_t const tmp = swap_u64(reinterpret_cast<uint64_t&>(x));
    return reinterpret_cast<double const&>(tmp);
}

namespace ME::nbt {

namespace internal {

bool is_container(TAG t) { return t == TAG::List || t == TAG::Compound; }

}  // namespace internal

std::string_view named_data_tag::get_name() const { return {name.data(), name.size()}; }

void named_data_tag::set_name(std::string_view inName) { name.assign(inName.data(), inName.size()); }

internal::named_data_tag_index data_store::add_named_data_tag(TAG type, std::string_view name) {
    named_data_tag tag;
    tag.dataTag.type = type;
    tag.set_name(name);

    namedTags.push_back(tag);
    return namedTags.size() - 1;
}

void data_store::clear()

{
    compoundStorage.clear();
    namedTags.clear();
    internal::pools<byte, int16_t, int32_t, int64_t, float, double, char, tag_payload_t::ByteArray, tag_payload_t::IntArray, tag_payload_t::LongArray, tag_payload_t::String, tag_payload_t::List,
                    tag_payload_t::Compound>::clear();
}

bool builder::begin_compound(std::string_view name) { return write_tag(TAG::Compound, name, tag_payload_t::Compound{}); }

void builder::end_compound() {
    ContainerInfo& container = containers.top();
    ME_ASSERT(container.Type() == TAG::Compound);
    if (container.temporaryContainer) {
        auto& pool = dataStore.pool<tag_payload_t::Compound>();
        int64_t const currentSize = static_cast<int64_t>(pool.size());
        auto& tempPool = container.temporaryContainer->data.pool<tag_payload_t::Compound>();
        pool.insert(pool.end(), tempPool.begin(), tempPool.end());
        container.pool_index(dataStore) = currentSize;
        ME_ASSERT(container.temporaryContainer == &temporaryContainers.top());
        temporaryContainers.pop();
    }
    tag_payload_t::Compound compound{container.storage(dataStore)};
    containers.pop();
    if (containers.empty()) return;
    ContainerInfo& parentContainer = containers.top();
    if (parentContainer.temporaryContainer) {
        parentContainer.temporaryContainer->data.pool<tag_payload_t::Compound>()[parentContainer.currentIndex++] = compound;
    }
}

bool builder::begin_list(std::string_view name) { return write_tag(TAG::List, name, tag_payload_t::List{}); }

void builder::end_list() {
    ContainerInfo& container = containers.top();
    ME_ASSERT(container.Type() == TAG::List);
    if (container.temporaryContainer) {
        // TODO: update elements in temporary container data pool
        int64_t currentSize = -1;
        if (container.element_type(dataStore) == TAG::List) {
            auto& pool = dataStore.pool<tag_payload_t::List>();
            currentSize = static_cast<int64_t>(pool.size());
            auto& tempPool = container.temporaryContainer->data.pool<tag_payload_t::List>();
            pool.insert(pool.end(), tempPool.begin(), tempPool.end());

        } else if (container.element_type(dataStore) == TAG::Compound) {
            auto& pool = dataStore.pool<tag_payload_t::Compound>();
            currentSize = static_cast<int64_t>(pool.size());
            auto& tempPool = container.temporaryContainer->data.pool<tag_payload_t::Compound>();
            pool.insert(pool.end(), tempPool.begin(), tempPool.end());
        }
        container.pool_index(dataStore) = currentSize;
        ME_ASSERT(container.temporaryContainer == &temporaryContainers.top());
        temporaryContainers.pop();
    }
    tag_payload_t::List list{container.element_type(dataStore), container.count(dataStore), container.pool_index(dataStore)};
    containers.pop();
    ContainerInfo& parentContainer = containers.top();
    if (parentContainer.temporaryContainer) {
        parentContainer.temporaryContainer->data.pool<tag_payload_t::List>()[parentContainer.currentIndex++] = list;
    }
}

void builder::write_byte(int8_t b, std::string_view name) { write_tag(TAG::Byte, name, byte{b}); }

void builder::write_short(int16_t s, std::string_view name) { write_tag(TAG::Short, name, s); }

void builder::write_int(int32_t i, std::string_view name) { write_tag(TAG::Int, name, i); }

void builder::write_long(int64_t l, std::string_view name) { write_tag(TAG::Long, name, l); }

void builder::write_float(float f, std::string_view name) { write_tag(TAG::Float, name, f); }

void builder::write_double(double d, std::string_view name) { write_tag(TAG::Double, name, d); }

void builder::write_byte_array(int8_t const* array, int32_t count, std::string_view name) {
    write_tag<tag_payload_t::ByteArray>(TAG::Byte_Array, name, [this, array, count]() {
        auto& bytePool = dataStore.pool<byte>();
        tag_payload_t::ByteArray byteArrayTag{count, bytePool.size()};
        bytePool.insert(bytePool.end(), array, array + count);
        return byteArrayTag;
    });
}

void builder::write_int_array(int32_t const* array, int32_t count, std::string_view name) {
    write_tag<tag_payload_t::IntArray>(TAG::Int_Array, name, [this, array, count]() {
        auto& intPool = dataStore.pool<int32_t>();
        tag_payload_t::IntArray intArrayTag{count, intPool.size()};
        intPool.insert(intPool.end(), array, array + count);
        return intArrayTag;
    });
}

void builder::write_long_array(int64_t const* array, int32_t count, std::string_view name) {
    write_tag<tag_payload_t::LongArray>(TAG::Long_Array, name, [this, array, count]() {
        auto& longPool = dataStore.pool<int64_t>();
        tag_payload_t::LongArray longArrayTag{count, longPool.size()};
        longPool.insert(longPool.end(), array, array + count);
        return longArrayTag;
    });
}

void builder::write_string(std::string_view str, std::string_view name) {
    write_tag<tag_payload_t::String>(TAG::String, name, [this, str]() {
        auto& stringPool = dataStore.pool<char>();
        tag_payload_t::String stringTag{static_cast<uint16_t>(str.size()), stringPool.size()};
        stringPool.insert(stringPool.end(), str.data(), str.data() + str.size());
        return stringTag;
    });
}

void builder::begin(std::string_view rootName) {
    internal::named_data_tag_index rootTagIndex = dataStore.add_named_data_tag(TAG::Compound, rootName);

    ContainerInfo rootContainer{};
    rootContainer.named = true;
    rootContainer.Type() = TAG::Compound;
    rootContainer.namedContainer.tagIndex = rootTagIndex;

    tag_payload_t::Compound c;
    c.storageIndex_ = dataStore.compoundStorage.size();
    dataStore.namedTags[rootTagIndex].dataTag.payload.Set<tag_payload_t::Compound>(c);
    dataStore.compoundStorage.emplace_back();

    containers.push(rootContainer);
}

void builder::end() {
    while (!is_end()) {
        switch (containers.top().Type()) {
            case TAG::List:
                end_list();
                break;
            case TAG::Compound:
                end_compound();
                break;
            default:
                ME_ASSERT(!"[NBT] Builder : internal Type Error - This should never happen.");
        }
    }
}

bool builder::is_end() const { return containers.empty(); }

TAG& builder::ContainerInfo::Type() { return type; }

TAG builder::ContainerInfo::Type() const { return type; }

TAG& builder::ContainerInfo::element_type(data_store& ds) {
    // only Lists have single element types
    ME_ASSERT(Type() == TAG::List);
    if (named) {
        return ds.namedTags[namedContainer.tagIndex].dataTag.payload.as<tag_payload_t::List>().elementType_;
    }
    return anonContainer.list.elementType_;
}

int32_t builder::ContainerInfo::count(data_store const& ds) const {
    if (named) {
        switch (Type()) {
            case TAG::List:
                return ds.namedTags[namedContainer.tagIndex].dataTag.payload.as<tag_payload_t::List>().count_;
            case TAG::Compound:
                return static_cast<int32_t>(ds.compoundStorage[ds.namedTags[namedContainer.tagIndex].dataTag.payload.as<tag_payload_t::Compound>().storageIndex_].size());
            default:
                ME_ASSERT(!"[NBT] Builder : internal Type Error - This should never happen.");
                return 0;
        }
    }
    switch (Type()) {
        case TAG::List:
            return anonContainer.list.count_;
        case TAG::Compound:
            return static_cast<int32_t>(ds.compoundStorage[anonContainer.compound.storageIndex_].size());
        default:
            ME_ASSERT(!"[NBT] Builder : internal Type Error - This should never happen.");
            return 0;
    }
}

void builder::ContainerInfo::increment_count(data_store& ds) {
    // only lists can have their count incremented
    ME_ASSERT(Type() == TAG::List);
    if (named) {
        ++ds.namedTags[namedContainer.tagIndex].dataTag.payload.as<tag_payload_t::List>().count_;
    } else {
        ++anonContainer.list.count_;
    }
}

uint64_t builder::ContainerInfo::storage(data_store const& ds) const {
    // only compounds can have storage
    ME_ASSERT(Type() == TAG::Compound);
    if (named) {
        return ds.namedTags[namedContainer.tagIndex].dataTag.payload.as<tag_payload_t::Compound>().storageIndex_;
    }
    return anonContainer.compound.storageIndex_;
}

size_t& builder::ContainerInfo::pool_index(data_store& ds) {
    // only compounds can have pool indices
    ME_ASSERT(Type() == TAG::List);
    if (named) {
        return ds.namedTags[namedContainer.tagIndex].dataTag.payload.as<tag_payload_t::List>().poolIndex_;
    }
    return anonContainer.list.poolIndex_;
}

template <typename T, typename Fn>
bool builder::write_tag(TAG type, std::string_view name, Fn valueGetter) {
    if (containers.size() >= 512) {
        ME_ASSERT(!"[NBT] Builder : Depth Error - Compound and List tags may not be nested beyond a depth of 512");
        return false;
    }
    if (containers.empty()) {
        ME_ASSERT(!"[NBT] Builder : Write After Finalized - Attempted to write tags after structure was finalized");
        return false;
    }
    ContainerInfo& container = containers.top();
    if (container.Type() == TAG::Compound) {
        internal::named_data_tag_index newTagIndex = dataStore.add_named_data_tag(type, name);
        dataStore.namedTags[newTagIndex].dataTag.payload.Set<T>(valueGetter());
        dataStore.compoundStorage[container.storage(dataStore)].push_back(newTagIndex);
        if (internal::is_container(type)) {
            ContainerInfo newContainer{};
            newContainer.named = true;
            newContainer.Type() = type;
            newContainer.namedContainer.tagIndex = newTagIndex;
            if (type == TAG::Compound) {
                dataStore.namedTags[newTagIndex].dataTag.payload.as<tag_payload_t::Compound>().storageIndex_ = dataStore.compoundStorage.size();
                dataStore.compoundStorage.emplace_back();
            }
            containers.push(newContainer);
        }
    } else if (container.Type() == TAG::List) {
        if (!name.empty()) {
            ME_ASSERT(!"[NBT] Builder : Name Error - Attempted to add a named tag to a List. Lists cannot contain named tags.\n");
            return false;
        }
        // The list is not exclusively the same type as this tag
        if (container.element_type(dataStore) != type) {
            // If the list is currently empty, make it into a list of tags of this
            // type
            if (container.count(dataStore) == 0) {
                container.element_type(dataStore) = type;
            } else {
                ME_ASSERT(!"[NBT] Builder : Type Error - Attempted to add a tag to a list with tags of different type. All tags in a list must be of the same type.\n");
                return false;
            }
        }
        if (internal::is_container(type)) {
            if (container.count(dataStore) == 0) {
                TemporaryContainer temporaryContainer{};
                temporaryContainer.type = type;
                temporaryContainers.push(temporaryContainer);
                container.temporaryContainer = &temporaryContainers.top();
            }
            if constexpr (std::is_same_v<T, tag_payload_t::List> || std::is_same_v<T, tag_payload_t::Compound>) {
                container.temporaryContainer->data.pool<T>().push_back(valueGetter());
            }

            ContainerInfo newContainer{};
            newContainer.named = false;
            newContainer.Type() = type;
            if (type == TAG::Compound) {
                newContainer.anonContainer.compound.storageIndex_ = dataStore.compoundStorage.size();
                dataStore.compoundStorage.emplace_back();
            }

            containers.push(newContainer);
        } else  // ordinary data type
        {
            uint64_t poolIndex = dataStore.pool<T>().size();
            dataStore.pool<T>().push_back(valueGetter());
            if (container.count(dataStore) == 0) {
                container.pool_index(dataStore) = poolIndex;
            }
        }
        container.increment_count(dataStore);
    }
    return true;
}

template <typename T, std::enable_if_t<!std::is_invocable_v<T>, bool>>
bool builder::write_tag(TAG type, std::string_view name, T value) {
    return write_tag<T>(type, name, [&value]() -> T { return value; });
}

bool reader::import_file(std::string_view filepath) {
    if (import_binfile(filepath)) return true;
    if (import_textfile(filepath)) return true;
    return false;
}

bool reader::import_textfile(std::string_view filepath) {
    if (!import_uncompressed_file(filepath)) return false;

    clear();

    return parse_text_stream();
}

bool reader::import_binfile(std::string_view filepath) {
    if (!import_uncompressed_file(filepath)) return false;

    clear();

    return parse_bin_stream();
}

bool reader::import_string(char const* data, uint32_t length) {
    std::vector<uint8_t> mem(data, data + length);
    memoryStream.set_contents(std::move(mem));
    return parse_text_stream();
}

bool reader::import_bin(uint8_t const* data, uint32_t length) {
    std::vector mem(data, data + length);
    memoryStream.set_contents(std::move(mem));
    return parse_bin_stream();
}

bool reader::open_compound(std::string_view name) {
    if (!handle_nesting(name, TAG::Compound)) {
        return false;
    }
    return open_container(TAG::Compound, name);
}

void reader::close_compound() {
    ContainerInfo& container = containers.top();
    if (container.type == TAG::Compound) {
        if (!inVirtualRootCompound) {
            containers.pop();
        }
        return;
    }
    ME_ASSERT(!"[NBT] Reader : Compound Close Mismatch - Attempted to close a compound when a compound was not open.\n");
}

bool reader::open_list(std::string_view name) {
    if (!handle_nesting(name, TAG::List)) {
        return false;
    }
    return open_container(TAG::List, name);
}

int32_t reader::list_size() const {
    ContainerInfo const& container = containers.top();
    if (container.type == TAG::List) {
        return container.count(dataStore);
    }
    ME_ASSERT(!"[NBT] Reader : Invalid List Size Read - Attempted to read a list's size when a list was not open.\n");
    return 0;
}

void reader::close_list() {
    ContainerInfo& container = containers.top();
    if (container.type == TAG::List) {
        containers.pop();
        return;
    }
    ME_ASSERT(!"[NBT] Reader : List Close Mismatch - Attempted to close a list when a list was not open.\n");
}

int8_t reader::read_byte(std::string_view name) {
    handle_nesting(name, TAG::Byte);
    return read_value<byte>(TAG::Byte, name);
}

int16_t reader::read_short(std::string_view name) {
    handle_nesting(name, TAG::Short);
    return read_value<int16_t>(TAG::Short, name);
}

int32_t reader::read_int(std::string_view name) {
    handle_nesting(name, TAG::Int);
    return read_value<int32_t>(TAG::Int, name);
}

int64_t reader::read_long(std::string_view name) {
    handle_nesting(name, TAG::Long);
    return read_value<int64_t>(TAG::Long, name);
}

float reader::read_float(std::string_view name) {
    handle_nesting(name, TAG::Float);
    return read_value<float>(TAG::Float, name);
}

double reader::read_double(std::string_view name) {
    handle_nesting(name, TAG::Double);
    return read_value<double>(TAG::Double, name);
}

std::vector<int8_t> reader::read_byte_array(std::string_view name) {
    handle_nesting(name, TAG::Byte_Array);
    tag_payload_t::ByteArray byteArray = read_value<tag_payload_t::ByteArray>(TAG::Byte_Array, name);
    auto* bytePool = dataStore.pool<byte>().data() + byteArray.poolIndex_;
    std::vector<int8_t> ret{bytePool, bytePool + byteArray.count_};
    return ret;
}

std::vector<int32_t> reader::read_int_array(std::string_view name) {
    handle_nesting(name, TAG::Int_Array);
    tag_payload_t::IntArray intArray = read_value<tag_payload_t::IntArray>(TAG::Int_Array, name);
    auto* intPool = dataStore.pool<int32_t>().data() + intArray.poolIndex_;
    std::vector<int32_t> ret{intPool, intPool + intArray.count_};
    std::transform(ret.begin(), ret.end(), ret.begin(), swap_i32);
    return ret;
}

std::vector<int64_t> reader::read_long_array(std::string_view name) {
    handle_nesting(name, TAG::Long_Array);
    tag_payload_t::LongArray longArray = read_value<tag_payload_t::LongArray>(TAG::Long_Array, name);
    auto* longPool = dataStore.pool<int64_t>().data() + longArray.poolIndex_;
    std::vector<int64_t> ret{longPool, longPool + longArray.count_};
    std::transform(ret.begin(), ret.end(), ret.begin(), swap_i64);
    return ret;
}

std::string_view reader::read_string(std::string_view name) {
    handle_nesting(name, TAG::String);
    tag_payload_t::String string = read_value<tag_payload_t::String>(TAG::String, name);
    return std::string_view{dataStore.pool<char>().data() + string.poolIndex_, static_cast<size_t>(string.length_)};
}

std::optional<int8_t> reader::maybe_read_byte(std::string_view name) {
    if (!handle_nesting(name, TAG::Byte)) return std::nullopt;
    if (auto b = maybe_read_value<byte>(TAG::Byte, name)) return *b;
    return {};
}

std::optional<int16_t> reader::maybe_read_short(std::string_view name) {
    if (!handle_nesting(name, TAG::Short)) return std::nullopt;
    return maybe_read_value<int16_t>(TAG::Short, name);
}

std::optional<int32_t> reader::maybe_read_int(std::string_view name) {
    if (!handle_nesting(name, TAG::Int)) return std::nullopt;
    return maybe_read_value<int32_t>(TAG::Int, name);
}

std::optional<int64_t> reader::maybe_read_long(std::string_view name) {
    if (!handle_nesting(name, TAG::Long)) return std::nullopt;
    return maybe_read_value<int64_t>(TAG::Long, name);
}

std::optional<float> reader::maybe_read_float(std::string_view name) {
    if (!handle_nesting(name, TAG::Float)) return std::nullopt;
    return maybe_read_value<float>(TAG::Float, name);
}

std::optional<double> reader::maybe_read_double(std::string_view name) {
    if (!handle_nesting(name, TAG::Double)) return std::nullopt;
    return maybe_read_value<double>(TAG::Double, name);
}

std::optional<std::vector<int8_t>> reader::maybe_read_byte_array(std::string_view name) {
    if (!handle_nesting(name, TAG::Byte_Array)) return std::nullopt;
    std::optional<tag_payload_t::ByteArray> byteArray = maybe_read_value<tag_payload_t::ByteArray>(TAG::Byte_Array, name);
    if (!byteArray) return std::nullopt;
    auto* bytePool = dataStore.pool<byte>().data() + byteArray->poolIndex_;
    std::vector<int8_t> ret{bytePool, bytePool + byteArray->count_};
    return std::make_optional(ret);
}

std::optional<std::vector<int32_t>> reader::maybe_read_int_array(std::string_view name) {
    if (!handle_nesting(name, TAG::Int_Array)) return std::nullopt;
    std::optional<tag_payload_t::IntArray> intArray = maybe_read_value<tag_payload_t::IntArray>(TAG::Int_Array, name);
    if (!intArray) return std::nullopt;
    auto* intPool = dataStore.pool<int32_t>().data() + intArray->poolIndex_;
    std::vector<int32_t> ret{intPool, intPool + intArray->count_};
    std::transform(ret.begin(), ret.end(), ret.begin(), swap_i32);
    return std::make_optional(ret);
}

std::optional<std::vector<int64_t>> reader::maybe_read_long_array(std::string_view name) {
    if (!handle_nesting(name, TAG::Long_Array)) return std::nullopt;
    std::optional<tag_payload_t::LongArray> longArray = maybe_read_value<tag_payload_t::LongArray>(TAG::Long_Array, name);
    if (!longArray) return std::nullopt;
    auto* longPool = dataStore.pool<int64_t>().data() + longArray->poolIndex_;
    std::vector<int64_t> ret{longPool, longPool + longArray->count_};
    std::transform(ret.begin(), ret.end(), ret.begin(), swap_i64);
    return std::make_optional(ret);
}

std::optional<std::string_view> reader::maybe_read_string(std::string_view name) {
    if (!handle_nesting(name, TAG::String)) return std::nullopt;
    std::optional<tag_payload_t::String> string = maybe_read_value<tag_payload_t::String>(TAG::String, name);
    if (!string) return std::nullopt;
    return std::string_view{dataStore.pool<char>().data() + string->poolIndex_, static_cast<size_t>(string->length_)};
}

int32_t reader::count() const {
    ContainerInfo const& container = containers.top();
    return container.count(dataStore);
}

reader::CompoundView reader::names() {
    ContainerInfo const& container = containers.top();
    if (container.Type() != TAG::Compound) return CompoundView{nullptr, nullptr};

    return CompoundView{&dataStore, &dataStore.compoundStorage[container.storage(dataStore)]};
}

void reader::MemoryStream::set_contents(std::vector<uint8_t>&& inData) {
    clear();
    data = std::move(inData);
}

void reader::MemoryStream::clear() {
    position = 0;
    data.clear();
}

bool reader::MemoryStream::has_contents() const { return position < data.size(); }

char reader::MemoryStream::current_byte() const {
    ME_ASSERT(has_contents());
    return data[position];
}

char reader::MemoryStream::lookahead_byte(int bytes) const {
    ME_ASSERT(position + bytes < data.size());
    return data[position + bytes];
}

reader::TextTokenizer::TextTokenizer(MemoryStream& stream) : stream(stream) {}

void reader::TextTokenizer::tokenize() {
    while (stream.has_contents()) {
        if (auto token = parse_token()) {
            tokens.emplace_back(*token);
        }
    }
}

reader::Token const& reader::TextTokenizer::get_current() const { return tokens[current]; }

bool reader::TextTokenizer::match(Token::Type type) {
    if (tokens[current].type == type) {
        ++current;
        return true;
    }
    return false;
}

std::optional<reader::Token> reader::TextTokenizer::parse_token() {
    stream.skip_bytes<' ', '\r', '\n', '\t'>();
    char const c = stream.current_byte();
    switch (c) {
        case '{':
            stream.retrieve<char>();
            return Token{Token::Type::COMPOUND_BEGIN};
        case '}':
            stream.retrieve<char>();
            return Token{Token::Type::COMPOUND_END};
        case '[':
            return try_parse_list_open();
        case ']':
            stream.retrieve<char>();
            return Token{Token::Type::LIST_END};
        case ':':
            stream.retrieve<char>();
            return Token{Token::Type::NAME_DELIM};
        case ',':
            stream.retrieve<char>();
            return Token{Token::Type::CONTAINER_DELIM};
        default:
            break;
    }
    if (auto const number = try_parse_number()) {
        return number;
    }
    if (auto const string = try_parse_string()) {
        return string;
    }
    return {};
}

std::optional<reader::Token> reader::TextTokenizer::try_parse_list_open() {
    if (stream.match_current_byte<'['>()) {
        Token t{Token::Type::LIST_BEGIN};
        if (stream.lookahead_byte(1) == ';') {
            if (char type = stream.current_byte(); stream.match_current_byte<'B', 'I', 'L'>()) {
                t.typeIndicator = type;
            }
            // eat the `;`
            (void)stream.retrieve<char>();
        }
        return t;
    }
    return {};
}

std::optional<reader::Token> reader::TextTokenizer::try_parse_number() {
    Token::Type type = Token::Type::INTEGER;
    int offset = 0;
    int digitCount = 0;

    if (stream.lookahead_byte(offset) == '-' || stream.lookahead_byte(offset) == '+') {
        ++offset;
    }
    while (isdigit(stream.lookahead_byte(offset))) {
        ++offset;
        ++digitCount;
    }
    if (stream.lookahead_byte(offset) == '.') {
        ++offset;
        type = Token::Type::REAL;
        while (isdigit(stream.lookahead_byte(offset))) {
            ++offset;
            ++digitCount;
        }
    }
    if (digitCount) {
        Token t;
        t.type = type;
        t.text = std::string_view(stream.retrieve_range_view<char>(offset), offset);
        if (char const suffix = stream.current_byte(); stream.match_current_byte<'l', 'L', 'b', 'B', 's', 'S', 'f', 'F', 'd', 'D'>()) {
            t.typeIndicator = suffix;
        }
        return t;
    }
    return {};
}

std::optional<reader::Token> reader::TextTokenizer::try_parse_string() {
    int offset = 0;
    char quote = '\0';
    if (stream.lookahead_byte(offset) == '\'') {
        quote = '\'';
        ++offset;
    } else if (stream.lookahead_byte(offset) == '\"') {
        quote = '\"';
        ++offset;
    }

    if (quote) {
        while (!(stream.lookahead_byte(offset) == quote && stream.lookahead_byte(offset - 1) != '\\')) {
            ++offset;
        }
        if (offset >= 2) {
            (void)stream.retrieve<char>();
            Token t{Token::Type::STRING};
            t.text = std::string_view(stream.retrieve_range_view<char>(offset), offset - 1);
            return t;
        }
    } else {
        while (!check_byte<',', '[', ']', '{', '}', ' '>(stream.lookahead_byte(offset))) {
            ++offset;
        }
        if (offset >= 1) {
            Token t{Token::Type::STRING};
            t.text = std::string_view(stream.retrieve_range_view<char>(offset), offset);
            return t;
        }
    }
    return {};
}

void reader::clear() {
    dataStore.clear();
    decltype(containers)().swap(containers);
}

bool reader::import_uncompressed_file(std::string_view filepath) {
    FILE* infile = fopen(filepath.data(), "rb");
    if (!infile) return false;

    bool ret = true;

    // get file size
    if (fseek(infile, 0, SEEK_END)) {
        ret = false;
        goto cleanup;
    }
    {
        size_t const fileSize = ftell(infile);
        rewind(infile);

        std::vector<uint8_t> fileData(fileSize, {});
        size_t const bytesRead = fread(fileData.data(), sizeof(uint8_t), fileSize, infile);
        if (bytesRead != fileSize) {
            ret = false;
            goto cleanup;
        }

        memoryStream.set_contents(std::move(fileData));
    }
    this->filepath = filepath;

cleanup:
    fclose(infile);
    return ret;
}

bool reader::parse_text_stream() {
    TextTokenizer tokenizer(memoryStream);
    tokenizer.tokenize();

    textTokenizer = &tokenizer;

    if (!textTokenizer->match(Token::Type::COMPOUND_BEGIN)) return false;

    begin();

    do {
        if (parse_text_named_tag() == TAG::End) return false;
    } while (textTokenizer->match(Token::Type::CONTAINER_DELIM));

    if (!textTokenizer->match(Token::Type::COMPOUND_END)) return false;

    textTokenizer = nullptr;

    return true;
}

bool reader::parse_bin_stream() {
    // parse root tag
    TAG const type = retrieve_bin_tag();
    if (type != TAG::Compound) return false;
    begin(retrieve_bin_str());

    do {
    } while (parse_bin_named_tag() != TAG::End);

    return true;
}

TAG reader::parse_bin_named_tag() {
    TAG const type = retrieve_bin_tag();
    if (type == TAG::End) return type;
    auto const name = retrieve_bin_str();
    parse_bin_payload(type, name);
    return type;
}

bool reader::parse_bin_payload(TAG type, std::string_view name) {
    switch (type) {
        case TAG::Byte:
            write_byte(memoryStream.retrieve<byte>(), name);
            break;
        case TAG::Short:
            write_short(swap_i16(memoryStream.retrieve<int16_t>()), name);
            break;
        case TAG::Int:
            write_int(swap_i32(memoryStream.retrieve<int32_t>()), name);
            break;
        case TAG::Long:
            write_long(swap_i64(memoryStream.retrieve<int64_t>()), name);
            break;
        case TAG::Float:
            write_float(swap_f32(memoryStream.retrieve<float>()), name);
            break;
        case TAG::Double:
            write_double(swap_f64(memoryStream.retrieve<double>()), name);
            break;
        case TAG::String:
            write_string(retrieve_bin_str(), name);
            break;
        case TAG::Byte_Array: {
            auto const count = retrieve_bin_array_len();
            write_byte_array(memoryStream.retrieve_range_view<int8_t>(count), count, name);
        } break;
        case TAG::Int_Array: {
            auto const count = retrieve_bin_array_len();
            write_int_array(memoryStream.retrieve_range_view<int32_t>(count), count, name);
        } break;
        case TAG::Long_Array: {
            auto const count = retrieve_bin_array_len();
            write_long_array(memoryStream.retrieve_range_view<int64_t>(count), count, name);
        } break;
        case TAG::List: {
            if (begin_list(name)) {
                auto const elementType = retrieve_bin_tag();
                auto const count = retrieve_bin_array_len();
                for (int i = 0; i < count; ++i) {
                    parse_bin_payload(elementType);
                }
                end_list();
            } else
                return false;
        } break;
        case TAG::Compound: {
            if (begin_compound(name)) {
                do {
                } while (parse_bin_named_tag() != TAG::End);
                end_compound();
            } else
                return false;
        }
    }
    return true;
}

TAG reader::retrieve_bin_tag() { return memoryStream.retrieve<TAG>(); }

int32_t reader::retrieve_bin_array_len() { return swap_i32(memoryStream.retrieve<int32_t>()); }

template <typename T>
static auto ByteSwap(T x) -> T {
    if constexpr (sizeof(T) == 2) {
        uint16_t const tmp = swap_u16(reinterpret_cast<uint16_t&>(x));
        return reinterpret_cast<T const&>(tmp);
    }
    if constexpr (sizeof(T) == 4) {
        uint32_t const tmp = swap_u32(reinterpret_cast<uint32_t&>(x));
        return reinterpret_cast<T const&>(tmp);
    }
    if constexpr (sizeof(T) == 8) {
        uint64_t const tmp = swap_u64(reinterpret_cast<uint64_t&>(x));
        return reinterpret_cast<T const&>(tmp);
    }
    return x;
}

template <typename T>
static auto parse_number(std::string_view str) -> T {
    T number;
    std::from_chars(str.data(), str.data() + str.size(), number);
    return number;
}

template <typename T, void (builder::*WriteArray)(T const*, int32_t, std::string_view), char... Suffix>
static auto packed_integer_list(reader* reader, TAG tag, std::string_view name) -> TAG {
    using Token = reader::Token;
    reader::TextTokenizer* textTokenizer = reader->textTokenizer;
    std::vector<T> integers;
    do {
        Token const& token = textTokenizer->get_current();
        if (!textTokenizer->match(Token::Type::INTEGER)) return TAG::End;
        if constexpr (sizeof...(Suffix) > 0)
            if (!reader::check_byte<Suffix...>(token.typeIndicator)) return TAG::End;
        integers.push_back(parse_number<T>(token.text));
    } while (textTokenizer->match(Token::Type::CONTAINER_DELIM));

    if (!textTokenizer->match(Token::Type::LIST_END)) return TAG::End;
    std::transform(integers.begin(), integers.end(), integers.begin(), ByteSwap<T>);
    (reader->*WriteArray)(integers.data(), static_cast<int32_t>(integers.size()), name);
    return tag;
}

TAG reader::parse_text_named_tag() {
    Token const& name = textTokenizer->get_current();
    if (!textTokenizer->match(Token::Type::STRING)) return TAG::End;
    if (!textTokenizer->match(Token::Type::NAME_DELIM)) return TAG::End;
    return parse_text_payload(name.text);
}

TAG reader::parse_text_payload(std::string_view name) {
    if (textTokenizer->match(Token::Type::COMPOUND_BEGIN) && begin_compound(name)) {
        while (parse_text_named_tag() != TAG::End) {
            if (!textTokenizer->match(Token::Type::CONTAINER_DELIM)) break;
        }

        if (textTokenizer->match(Token::Type::COMPOUND_END)) {
            end_compound();
            return TAG::Compound;
        }

        return TAG::End;
    }
    if (Token const& listOpen = textTokenizer->get_current(); textTokenizer->match(Token::Type::LIST_BEGIN)) {
        switch (listOpen.typeIndicator) {
            case 'B':
                return packed_integer_list<int8_t, &builder::write_byte_array, 'b', 'B'>(this, TAG::Byte_Array, name);
            case 'I':
                return packed_integer_list<int32_t, &builder::write_int_array>(this, TAG::Int_Array, name);
            case 'L':
                return packed_integer_list<int64_t, &builder::write_long_array, 'l', 'L'>(this, TAG::Long_Array, name);
            case '\0':
                break;
            default:
                return TAG::End;
        }
        if (begin_list(name)) {
            // empty list
            if (textTokenizer->match(Token::Type::LIST_END)) {
                end_list();
                return TAG::List;
            }
            do {
                if (parse_text_payload() == TAG::End) return TAG::End;
            } while (textTokenizer->match(Token::Type::CONTAINER_DELIM));

            if (textTokenizer->match(Token::Type::LIST_END)) {
                end_list();
                return TAG::List;
            }
        }
        return TAG::End;
    }
    if (Token const& string = textTokenizer->get_current(); textTokenizer->match(Token::Type::STRING)) {
        if (string.text == "true") {
            write_byte(1, name);
            return TAG::Byte;
        }
        if (string.text == "false") {
            write_byte(0, name);
            return TAG::Byte;
        }
        write_string(string.text, name);
        return TAG::String;
    }
    if (Token const& integer = textTokenizer->get_current(); textTokenizer->match(Token::Type::INTEGER)) {
        switch (integer.typeIndicator) {
            case 'b':
            case 'B': {
                write_byte(parse_number<int8_t>(integer.text), name);
                return TAG::Byte;
            }
            case 's':
            case 'S': {
                write_short(parse_number<int16_t>(integer.text), name);
                return TAG::Short;
            }
            case 'l':
            case 'L': {
                write_long(parse_number<int64_t>(integer.text), name);
                return TAG::Long;
            }
            // ints do not use a suffix
            case '\0': {
                write_int(parse_number<int32_t>(integer.text), name);
                return TAG::Int;
            }
            default:
                return TAG::End;
        }
    }
    if (Token const& real = textTokenizer->get_current(); textTokenizer->match(Token::Type::REAL)) {
        switch (real.typeIndicator) {
            case 'f':
            case 'F': {
                write_float(parse_number<float>(real.text), name);
                return TAG::Float;
            }
            case 'd':
            case 'D':
            case '\0': {
                write_double(parse_number<double>(real.text), name);
                return TAG::Float;
            }
            default:
                return TAG::End;
        }
    }

    return TAG::End;
}

std::string reader::retrieve_bin_str() {
    auto const len = swap_i16(memoryStream.retrieve<int16_t>());
    std::string str(memoryStream.retrieve_range_view<char>(len), len);
    return str;
}

bool reader::handle_nesting(std::string_view name, TAG t) {
    auto& container = containers.top();
    // Lists have strict requirements
    if (container.Type() == TAG::List) {
        if (!name.empty()) {
            // problems, lists cannot have named tags
            ME_ASSERT(!"[NBT] Reader : List Named Read - Attempted to read named tag from a list.");
            return false;
        }
        if (container.element_type(dataStore) != t) {
            if (container.count(dataStore) != 0) {
                ME_ASSERT(!"[NBT] Reader : List Type Mismatch - Attempted to read the wrong type from a list.");
                return false;
            }
        } else {
            if (container.currentIndex >= container.count(dataStore)) {
                ME_ASSERT(!"[NBT] Reader : List Overread - Attempted to read too many items from a list.");
                return false;
            }
        }
        ++container.currentIndex;
    } else if (container.Type() == TAG::Compound) {
        if (name.empty()) {
            // allow unnamed compound reads at file level for simplicity in Read implementations
            if ((t == TAG::Compound) && (containers.size() == 1) && !inVirtualRootCompound) {
                return true;
            }
            // bad, other compound tags must have names
            ME_ASSERT(!"[NBT] Reader : Compound Unnamed Read - Attempted to read unnamed tag from a compound.");
            return false;
        }
    }
    return true;
}

bool reader::open_container(TAG t, std::string_view name) {
    auto& container = containers.top();
    if (container.Type() == TAG::List) {
        ContainerInfo newContainer{};
        newContainer.named = false;
        newContainer.type = t;
        if (t == TAG::List) {
            newContainer.anonContainer.list.poolIndex_ = dataStore.pool<tag_payload_t::List>()[(container.currentIndex - 1) + container.pool_index(dataStore)].poolIndex_;
        }
        if (t == TAG::Compound) {
            newContainer.anonContainer.compound.storageIndex_ = dataStore.pool<tag_payload_t::Compound>()[(container.currentIndex - 1) + container.pool_index(dataStore)].storageIndex_;
        }
        containers.push(newContainer);

        return true;
    }
    if (container.Type() == TAG::Compound) {
        if (name.empty() && (t == TAG::Compound) && (containers.size() == 1)) {
            inVirtualRootCompound = true;
            return true;
        }
        for (internal::named_data_tag_index tagIndex : dataStore.compoundStorage[container.storage(dataStore)]) {
            named_data_tag const& tag = dataStore.namedTags[tagIndex];
            // TODO: if this ever becomes a performance issue, look at changing the vector to a set
            if (tag.get_name() == name && tag.dataTag.type == t) {
                ContainerInfo newContainer{};
                newContainer.named = true;
                newContainer.type = t;
                newContainer.namedContainer.tagIndex = tagIndex;
                containers.push(newContainer);

                return true;
            }
        }
    }
    return false;
}

template <typename T>
T reader::MemoryStream::retrieve() {
    ME_ASSERT(position + sizeof(T) <= data.size());
    T const* valueAddress = reinterpret_cast<T*>(data.data() + position);
    position += sizeof(T);
    return *valueAddress;
}

template <typename T>
T const* reader::MemoryStream::retrieve_range_view(size_t count) {
    ME_ASSERT(position + sizeof(T) * count <= data.size());
    T const* valueAddress = reinterpret_cast<T*>(data.data() + position);
    position += sizeof(T) * count;
    return valueAddress;
}

template <char... ToMatch>
bool reader::MemoryStream::match_current_byte() {
    if (reader::check_byte<ToMatch...>(current_byte())) {
        ++position;
        return true;
    }
    return false;
}

template <char... ToSkip>
void reader::MemoryStream::skip_bytes() {
    while (reader::check_byte<ToSkip...>(data[position])) {
        ++position;
    }
}

template <char... ToCheck>
bool reader::check_byte(char byte) {
    return ((byte == ToCheck) || ...);
}

template <typename T>
T& reader::read_value(TAG t, std::string_view name) {
    ContainerInfo& container = containers.top();
    if (container.type == TAG::List) {
        return (dataStore.pool<T>().data() + container.pool_index(dataStore))[container.currentIndex - 1];
    }
    if (container.type == TAG::Compound) {
        // TODO：如果这成为性能问题，请考虑将向量更改为集合
        for (auto tagIndex : dataStore.compoundStorage[container.storage(dataStore)]) {
            auto& tag = dataStore.namedTags[tagIndex];
            if (tag.get_name() == name) {
                ME_ASSERT(tag.dataTag.type == t);
                return tag.dataTag.payload.as<T>();
            }
        }
    }

    ME_ASSERT(!"[NBT] Reader Error: Value with given name not present");
    // 未定义的真正的故意不良行为
    return *static_cast<T*>(nullptr);
}

template <typename T>
std::optional<T> reader::maybe_read_value(TAG t, std::string_view name) {
    ContainerInfo& container = containers.top();
    if (container.type == TAG::List) {
        return (dataStore.pool<T>().data() + container.pool_index(dataStore))[container.currentIndex - 1];
    }
    if (container.type == TAG::Compound) {
        // TODO：如果这成为性能问题，请考虑将向量更改为集合
        for (internal::named_data_tag_index tagIndex : dataStore.compoundStorage[container.storage(dataStore)]) {
            auto& tag = dataStore.namedTags[tagIndex];
            if (tag.get_name() == name) {
                if (tag.dataTag.type != t) return {};
                return tag.dataTag.payload.as<T>();
            }
        }
    }
    return std::nullopt;
}

template <>
int8_t reader::read(std::string_view name) {
    return read_byte(name);
}
template <>
int16_t reader::read(std::string_view name) {
    return read_short(name);
}
template <>
int32_t reader::read(std::string_view name) {
    return read_int(name);
}
template <>
int64_t reader::read(std::string_view name) {
    return read_long(name);
}
template <>
float reader::read(std::string_view name) {
    return read_float(name);
}
template <>
double reader::read(std::string_view name) {
    return read_double(name);
}
template <>
std::vector<int8_t> reader::read(std::string_view name) {
    return read_byte_array(name);
}
template <>
std::vector<int32_t> reader::read(std::string_view name) {
    return read_int_array(name);
}
template <>
std::vector<int64_t> reader::read(std::string_view name) {
    return read_long_array(name);
}
template <>
std::string_view reader::read(std::string_view name) {
    return read_string(name);
}

template <>
std::optional<int8_t> reader::maybe_read(std::string_view name) {
    return maybe_read_byte(name);
}
template <>
std::optional<int16_t> reader::maybe_read(std::string_view name) {
    return maybe_read_short(name);
}
template <>
std::optional<int32_t> reader::maybe_read(std::string_view name) {
    return maybe_read_int(name);
}
template <>
std::optional<int64_t> reader::maybe_read(std::string_view name) {
    return maybe_read_long(name);
}
template <>
std::optional<float> reader::maybe_read(std::string_view name) {
    return maybe_read_float(name);
}
template <>
std::optional<double> reader::maybe_read(std::string_view name) {
    return maybe_read_double(name);
}
template <>
std::optional<std::vector<int8_t>> reader::maybe_read(std::string_view name) {
    return maybe_read_byte_array(name);
}
template <>
std::optional<std::vector<int32_t>> reader::maybe_read(std::string_view name) {
    return maybe_read_int_array(name);
}
template <>
std::optional<std::vector<int64_t>> reader::maybe_read(std::string_view name) {
    return maybe_read_long_array(name);
}
template <>
std::optional<std::string_view> reader::maybe_read(std::string_view name) {
    return maybe_read_string(name);
}

template <typename T, std::enable_if_t<(sizeof(T) <= sizeof(T*)), bool> = true>
void store(std::vector<uint8_t>& v, T data) {
    auto const size = v.size();
    v.resize(size + sizeof(T));
    std::memcpy(v.data() + size, &data, sizeof(T));
}

template <typename T, std::enable_if_t<(sizeof(T) > sizeof(T*)), bool> = true>
void store(std::vector<uint8_t>& v, T const& data) {
    auto const size = v.size();
    v.resize(size + sizeof(T));
    std::memcpy(v.data() + size, &data, sizeof(T));
}

template <typename T>
void store_range(std::vector<uint8_t>& v, T* data, size_t count) {
    auto const size = v.size();
    v.resize(size + sizeof(T) * count);
    std::memcpy(v.data() + size, data, sizeof(T) * count);
}

std::string escape_quotes(std::string_view inStr) {
    std::string result;
    size_t const length = inStr.length();
    result.reserve(length + 10);  // assume up to 10 quotes
    for (size_t i = 0; i < length; ++i) {
        if (inStr[i] == '"') {
            result += R"(\")";
        } else {
            result += inStr[i];
        }
    }
    return result;
}

class NewlineOp {
    writer::PrettyPrint prettyPrint;

public:
    NewlineOp(writer::PrettyPrint prettyPrint) : prettyPrint(prettyPrint) {}
    static std::ostream& newline_fn(std::ostream& out) {
        out << '\n';
        return out;
    }
    friend std::ostream& operator<<(std::ostream& out, NewlineOp const& op) {
        if (op.prettyPrint == writer::PrettyPrint::Enabled) return newline_fn(out);
        return out;
    }
};

#define Newline NewlineOp(textOutputState.prettyPrint)

// private implementations

writer::writer() { begin(); }

writer::~writer() { end(); }

bool writer::export_text_file(std::string_view filepath, PrettyPrint prettyPrint) {
    if (!is_end()) return false;

    FILE* file = fopen(filepath.data(), "wb");
    if (!file) {
        return false;
    }
    std::string text;
    if (!export_string(text, prettyPrint)) {
        return false;
    }
    auto const written = fwrite(text.data(), sizeof(uint8_t), text.size(), file);
    fclose(file);
    return written == text.size();
}

bool writer::export_bin_file(std::string_view filepath) {
    if (!is_end()) return false;

    FILE* file = fopen(filepath.data(), "wb");
    if (!file) {
        return false;
    }
    std::vector<uint8_t> data;
    if (!export_bin(data)) {
        fclose(file);
        return false;
    }
    auto const written = fwrite(data.data(), sizeof(uint8_t), data.size(), file);
    fclose(file);
    return written == data.size();
}

bool writer::export_string(std::string& out, PrettyPrint prettyPrint) {
    if (!is_end()) return false;
    textOutputState = {};
    textOutputState.prettyPrint = prettyPrint;
    auto const& root = dataStore.namedTags[0];
    std::stringstream outStream(out);
    output_text_tag(outStream, root);
    out = outStream.str();
    return true;
}

bool writer::export_bin(std::vector<uint8_t>& out) {
    if (!is_end()) return false;
    auto const& root = dataStore.namedTags[0];
    output_bin_tag(out, root);
    return true;
}

void writer::output_bin_tag(std::vector<uint8_t>& out, named_data_tag const& tag) {
    store(out, tag.dataTag.type);
    output_bin_str(out, tag.get_name());
    output_bin_payload(out, tag.dataTag);
}

void writer::output_bin_str(std::vector<uint8_t>& out, std::string_view str) {
    uint16_t const lenBigEndian = swap_u16(static_cast<int16_t>(str.length()));
    store(out, lenBigEndian);
    store_range(out, str.data(), str.length());
}

void writer::output_bin_payload(std::vector<uint8_t>& out, data_tag const& tag) {
    switch (tag.type) {
        case TAG::Byte: {
            store(out, tag.payload.as<byte>());
        } break;
        case TAG::Short: {
            store(out, swap_i16(tag.payload.as<int16_t>()));
        } break;
        case TAG::Int: {
            store(out, swap_i32(tag.payload.as<int32_t>()));
        } break;
        case TAG::Long: {
            store(out, swap_i64(tag.payload.as<int64_t>()));
        } break;
        case TAG::Float: {
            store(out, swap_f32(tag.payload.as<float>()));
        } break;
        case TAG::Double: {
            store(out, swap_f64(tag.payload.as<double>()));
        } break;
        case TAG::Byte_Array: {
            auto& byteArray = tag.payload.as<tag_payload_t::ByteArray>();
            store(out, swap_i32(byteArray.count_));
            store_range(out, dataStore.pool<byte>().data() + byteArray.poolIndex_, byteArray.count_);
        } break;
        case TAG::Int_Array: {
            auto& intArray = tag.payload.as<tag_payload_t::IntArray>();
            auto intPool = dataStore.pool<int32_t>().data() + intArray.poolIndex_;
            store(out, swap_i32(intArray.count_));
            for (int i = 0; i < intArray.count_; ++i) {
                store(out, swap_i32(intPool[i]));
            }
        } break;
        case TAG::Long_Array: {
            auto& longArray = tag.payload.as<tag_payload_t::LongArray>();
            auto longPool = dataStore.pool<int64_t>().data() + longArray.poolIndex_;
            store(out, swap_i32(longArray.count_));
            for (int i = 0; i < longArray.count_; ++i) {
                store(out, swap_i64(longPool[i]));
            }
        } break;
        case TAG::String: {
            auto& string = tag.payload.as<tag_payload_t::String>();
            store(out, swap_u16(string.length_));
            store_range(out, dataStore.pool<char>().data() + string.poolIndex_, string.length_);
        } break;
        case TAG::List: {
            auto& list = tag.payload.as<tag_payload_t::List>();
            if (list.count_ == 0) {
                store(out, TAG::End);
                store(out, list.count_);
                return;
            }
            store(out, list.elementType_);
            store(out, swap_u32(list.count_));
            switch (list.elementType_) {
                case TAG::Byte: {
                    store_range(out, dataStore.pool<byte>().data() + list.poolIndex_, list.count_);
                } break;
                case TAG::Short: {
                    auto const* shortPool = dataStore.pool<int16_t>().data() + list.poolIndex_;
                    out.reserve(out.size() + sizeof(int16_t) * list.count_);
                    for (int i = 0; i < list.count_; ++i) {
                        store(out, swap_i16(shortPool[i]));
                    }
                } break;
                case TAG::Int: {
                    auto const* intPool = dataStore.pool<int32_t>().data() + list.poolIndex_;
                    out.reserve(out.size() + sizeof(int32_t) * list.count_);
                    for (int i = 0; i < list.count_; ++i) {
                        store(out, swap_i32(intPool[i]));
                    }
                } break;
                case TAG::Long: {
                    auto const* longPool = dataStore.pool<int64_t>().data() + list.poolIndex_;
                    out.reserve(out.size() + sizeof(int64_t) * list.count_);
                    for (int i = 0; i < list.count_; ++i) {
                        store(out, swap_i64(longPool[i]));
                    }
                } break;
                case TAG::Float: {
                    auto const* floatPool = dataStore.pool<float>().data() + list.poolIndex_;
                    out.reserve(out.size() + sizeof(float) * list.count_);
                    for (int i = 0; i < list.count_; ++i) {
                        store(out, swap_f32(floatPool[i]));
                    }
                } break;
                case TAG::Double: {
                    auto const* doublePool = dataStore.pool<double>().data() + list.poolIndex_;
                    out.reserve(out.size() + sizeof(double) * list.count_);
                    for (int i = 0; i < list.count_; ++i) {
                        store(out, swap_f64(doublePool[i]));
                    }
                } break;
                case TAG::Byte_Array: {
                    auto const* byteArrayPool = dataStore.pool<tag_payload_t::ByteArray>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_; ++i) {
                        auto const& byteArray = byteArrayPool[i];
                        store(out, swap_i32(byteArray.count_));
                        store_range(out, dataStore.pool<byte>().data() + byteArray.poolIndex_, byteArray.count_);
                    }
                } break;
                case TAG::Int_Array: {
                    auto const* intArrayPool = dataStore.pool<tag_payload_t::IntArray>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_; ++i) {
                        auto const& intArray = intArrayPool[i];
                        auto const* intPool = dataStore.pool<int32_t>().data() + intArray.poolIndex_;
                        store(out, swap_i32(intArray.count_));
                        for (int j = 0; j < intArray.count_; ++j) {
                            store(out, swap_i32(intPool[j]));
                        }
                    }
                } break;
                case TAG::Long_Array: {
                    auto const* longArrayPool = dataStore.pool<tag_payload_t::LongArray>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_; ++i) {
                        auto const& longArray = longArrayPool[i];
                        auto const* longPool = dataStore.pool<int64_t>().data() + longArray.poolIndex_;
                        store(out, swap_i32(longArray.count_));
                        for (int j = 0; j < longArray.count_; ++j) {
                            store(out, swap_i64(longPool[j]));
                        }
                    }
                } break;
                case TAG::String: {
                    auto const* stringPool = dataStore.pool<tag_payload_t::String>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_; ++i) {
                        auto& string = stringPool[i];
                        store(out, swap_u16(string.length_));
                        store_range(out, dataStore.pool<char>().data() + string.poolIndex_, string.length_);
                    }
                } break;
                case TAG::List: {
                    auto const* listPool = dataStore.pool<tag_payload_t::List>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_; ++i) {
                        data_tag sublistTag;
                        sublistTag.type = TAG::List;
                        sublistTag.payload.Set(listPool[i]);
                        output_bin_payload(out, sublistTag);
                    }
                } break;
                case TAG::Compound: {
                    auto const* compoundPool = dataStore.pool<tag_payload_t::Compound>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_; ++i) {
                        data_tag subcompoundTag;
                        subcompoundTag.type = TAG::Compound;
                        subcompoundTag.payload.Set(compoundPool[i]);
                        output_bin_payload(out, subcompoundTag);
                    }
                } break;
            }
        } break;
        case TAG::Compound: {
            auto& compound = tag.payload.as<tag_payload_t::Compound>();
            for (auto namedTagIndex : dataStore.compoundStorage[compound.storageIndex_]) {
                output_bin_tag(out, dataStore.namedTags[namedTagIndex]);
            }
            store(out, TAG::End);
        } break;
    }
}

void writer::output_text_tag(std::ostream& out, named_data_tag const& tag) {
    // root tag likely nameless
    if (!tag.get_name().empty()) {
        output_text_str(out, tag.get_name());
        out << ':';
    }
    output_text_payload(out, tag.dataTag);
}

void writer::output_text_str(std::ostream& out, std::string_view str) { out << '"' << escape_quotes(str) << '"'; }

void writer::output_text_payload(std::ostream& out, data_tag const& tag) {
    switch (tag.type) {
        case TAG::Byte: {
            out << std::to_string(tag.payload.as<byte>()) << 'b';
        } break;
        case TAG::Short: {
            out << tag.payload.as<int16_t>() << 's';
        } break;
        case TAG::Int: {
            out << tag.payload.as<int32_t>();
        } break;
        case TAG::Long: {
            out << tag.payload.as<int64_t>() << 'l';
        } break;
        case TAG::Float: {
            out << std::setprecision(7);
            out << tag.payload.as<float>() << 'f';
        } break;
        case TAG::Double: {
            out << std::setprecision(15);
            out << tag.payload.as<double>();
        } break;
        case TAG::Byte_Array: {
            out << "[B;";
            auto& byteArray = tag.payload.as<tag_payload_t::ByteArray>();
            auto bytePool = dataStore.pool<byte>().data() + byteArray.poolIndex_;
            for (int i = 0; i < byteArray.count_ - 1; ++i) {
                out << std::to_string(bytePool[i]) << "b,";
            }
            if (byteArray.count_) out << std::to_string(bytePool[byteArray.count_ - 1]) << "b]";
        } break;
        case TAG::Int_Array: {
            out << "[I;";
            auto& intArray = tag.payload.as<tag_payload_t::IntArray>();
            auto intPool = dataStore.pool<int32_t>().data() + intArray.poolIndex_;
            for (int i = 0; i < intArray.count_ - 1; ++i) {
                out << intPool[i] << ',';
            }
            if (intArray.count_) out << intPool[intArray.count_ - 1] << ']';
        } break;
        case TAG::Long_Array: {
            out << "[L;";
            auto& longArray = tag.payload.as<tag_payload_t::LongArray>();
            auto longPool = dataStore.pool<int64_t>().data() + longArray.poolIndex_;
            for (int i = 0; i < longArray.count_ - 1; ++i) {
                out << longPool[i] << "l,";
            }
            if (longArray.count_) out << longPool[longArray.count_ - 1] << "l]";
        } break;
        case TAG::String: {
            auto& string = tag.payload.as<tag_payload_t::String>();
            output_text_str(out, {dataStore.pool<char>().data() + string.poolIndex_, string.length_});
        } break;
        case TAG::List: {
            out << '[';
            auto& list = tag.payload.as<tag_payload_t::List>();
            if (list.count_ == 0) {
                out << ']';
                return;
            }
            switch (list.elementType_) {
                case TAG::Byte: {
                    auto const* bytePool = dataStore.pool<byte>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_ - 1; ++i) {
                        out << std::to_string(bytePool[i]) << "b,";
                    }
                    out << std::to_string(bytePool[list.count_ - 1]) << 'b';
                } break;
                case TAG::Short: {
                    auto const* shortPool = dataStore.pool<int16_t>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_ - 1; ++i) {
                        out << shortPool[i] << "s,";
                    }
                    out << shortPool[list.count_ - 1] << 's';
                } break;
                case TAG::Int: {
                    auto const* intPool = dataStore.pool<int32_t>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_ - 1; ++i) {
                        out << intPool[i] << ',';
                    }
                    out << intPool[list.count_ - 1];
                } break;
                case TAG::Long: {
                    auto const* longPool = dataStore.pool<int64_t>().data() + list.poolIndex_;
                    for (int i = 0; i < list.count_ - 1; ++i) {
                        out << longPool[i] << "l,";
                    }
                    out << longPool[list.count_ - 1] << 'l';
                } break;
                case TAG::Float: {
                    auto const* floatPool = dataStore.pool<float>().data() + list.poolIndex_;
                    out << std::setprecision(7);
                    for (int i = 0; i < list.count_ - 1; ++i) {
                        out << floatPool[i] << "f,";
                    }
                    out << floatPool[list.count_ - 1] << 'f';
                } break;
                case TAG::Double: {
                    auto const* doublePool = dataStore.pool<double>().data() + list.poolIndex_;
                    out << std::setprecision(15);
                    for (int i = 0; i < list.count_ - 1; ++i) {
                        out << doublePool[i] << ',';
                    }
                    out << doublePool[list.count_ - 1];
                } break;
                case TAG::Byte_Array: {
                    auto const* byteArrayPool = dataStore.pool<tag_payload_t::ByteArray>().data() + list.poolIndex_;
                    ++textOutputState.depth;
                    for (int i = 0; i < list.count_; ++i) {
                        data_tag newTag;
                        newTag.type = list.elementType_;
                        newTag.payload.Set(byteArrayPool[i]);
                        output_text_payload(out, newTag);
                        if (i != list.count_ - 1) out << ',';
                    }
                    --textOutputState.depth;
                } break;
                case TAG::Int_Array: {
                    auto const* intArrayPool = dataStore.pool<tag_payload_t::IntArray>().data() + list.poolIndex_;
                    ++textOutputState.depth;
                    for (int i = 0; i < list.count_; ++i) {
                        data_tag newTag;
                        newTag.type = list.elementType_;
                        newTag.payload.Set(intArrayPool[i]);
                        output_text_payload(out, newTag);
                        if (i != list.count_ - 1) out << ',';
                    }
                    --textOutputState.depth;
                } break;
                case TAG::Long_Array: {
                    auto const* longArrayPool = dataStore.pool<tag_payload_t::LongArray>().data() + list.poolIndex_;
                    ++textOutputState.depth;
                    for (int i = 0; i < list.count_; ++i) {
                        data_tag newTag;
                        newTag.type = list.elementType_;
                        newTag.payload.Set(longArrayPool[i]);
                        output_text_payload(out, newTag);
                        if (i != list.count_ - 1) out << ',';
                    }
                    --textOutputState.depth;
                } break;
                case TAG::List: {
                    auto const* listPool = dataStore.pool<tag_payload_t::List>().data() + list.poolIndex_;
                    ++textOutputState.depth;
                    for (int i = 0; i < list.count_; ++i) {
                        data_tag newTag;
                        newTag.type = list.elementType_;
                        newTag.payload.Set(listPool[i]);
                        output_text_payload(out, newTag);
                        if (i != list.count_ - 1) out << ',';
                    }
                    --textOutputState.depth;
                } break;
                case TAG::Compound: {
                    auto const* compoundPool = dataStore.pool<tag_payload_t::Compound>().data() + list.poolIndex_;
                    ++textOutputState.depth;
                    for (int i = 0; i < list.count_; ++i) {
                        out << Newline;
                        data_tag newTag;
                        newTag.type = list.elementType_;
                        newTag.payload.Set(compoundPool[i]);
                        output_text_payload(out, newTag);
                        if (i != list.count_ - 1) out << ',';
                    }
                    --textOutputState.depth;
                } break;
            }
            out << ']';
        } break;
        case TAG::Compound: {
            auto& compound = tag.payload.as<tag_payload_t::Compound>();
            out << '{' << Newline;
            ++textOutputState.depth;
            for (auto namedTagIndex : dataStore.compoundStorage[compound.storageIndex_]) {
                output_text_tag(out, dataStore.namedTags[namedTagIndex]);
                if (namedTagIndex != dataStore.compoundStorage[compound.storageIndex_].back())
                    out << ',' << Newline;
                else
                    out << Newline;
            }
            --textOutputState.depth;
            out << '}';
        } break;
    }
}

}  // namespace ME::nbt
