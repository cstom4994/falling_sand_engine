
#ifndef ME_NBT_HPP
#define ME_NBT_HPP

#include <cstdint>
#include <limits>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace ME::nbt {

#define ME_NBT_ALL_TYPES                                                                                                                                                           \
    byte, int16_t, int32_t, int64_t, float, double, char, tag_payload_t::ByteArray, tag_payload_t::IntArray, tag_payload_t::LongArray, tag_payload_t::String, tag_payload_t::List, \
            tag_payload_t::Compound

struct byte {
    int8_t b;
    byte() = default;
    byte(int8_t i) : b(i) {}
    byte& operator=(int8_t i) {
        b = i;
        return *this;
    }
    operator int8_t&() { return b; }
    operator int8_t const&() const { return b; }
};

enum class TAG : uint8_t {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    Byte_Array = 7,
    String = 8,
    List = 9,
    Compound = 10,
    Int_Array = 11,
    Long_Array = 12,
    INVALID = 0xCC
};

struct tag_payload_t {
    struct ByteArray {
        int32_t count_;
        size_t poolIndex_;
    };
    struct IntArray {
        int32_t count_;
        size_t poolIndex_;
    };
    struct LongArray {
        int32_t count_;
        size_t poolIndex_;
    };
    struct String {
        uint16_t length_;
        size_t poolIndex_;
    };
    struct List {
        TAG elementType_ = TAG::End;
        int32_t count_ = 0;
        size_t poolIndex_ = std::numeric_limits<size_t>::max();
    };
    struct Compound {
        size_t storageIndex_ = std::numeric_limits<size_t>::max();
    };

    template <typename T>
    T& as() {
        return std::get<T>(data_);
    }

    template <typename T>
    T const& as() const {
        return std::get<T>(data_);
    }

    template <typename T>
    void Set(T val = T{}) {
        data_.emplace<T>(val);
    }

private:
    std::variant<ME_NBT_ALL_TYPES> data_;
};

class data_tag {
public:
    data_tag(TAG type) : type(type) {}
    data_tag() = default;

    tag_payload_t payload;
    TAG type = TAG::INVALID;
};

class named_data_tag {
public:
    std::string_view get_name() const;
    void set_name(std::string_view inName);

private:
    std::string name;

public:
    data_tag dataTag;
};

namespace internal {
bool is_container(TAG t);

struct named_data_tag_index {
    uint64_t idx;
    named_data_tag_index() = default;
    named_data_tag_index(uint64_t i) : idx(i) {}
    operator uint64_t&() { return idx; }
    operator uint64_t const&() const { return idx; }
    bool operator<(named_data_tag_index const& rhs) const { return idx < rhs.idx; }
};

// Pools are the backing storage of lists/arrays

template <typename... Ts>
struct pools {
    std::tuple<std::vector<Ts>...> pools;

    template <typename T>
    std::vector<T>& pool() {
        return std::get<std::vector<T>>(pools);
    }

    void clear() { (std::get<std::vector<Ts>>(pools).clear(), ...); }
};

using all_pools = internal::pools<ME_NBT_ALL_TYPES>;

}  // namespace internal

struct data_store : internal::all_pools {
    // sets of indices into namedTags
    std::vector<std::vector<internal::named_data_tag_index>> compoundStorage;

    std::vector<named_data_tag> namedTags;

    internal::named_data_tag_index add_named_data_tag(TAG type, std::string_view name);

    void clear();
};

#undef ME_NBT_ALL_TYPES

class builder {
public:
    //  开始其他标签的复合。
    //  这意味着将写入调用 EndCompound() 之前的所有写入
    //  进入这个Compound Compound类似于结构并包含命名
    //  任何类型的标签 Compound内的标记具有以下要求他们必须被命名
    bool begin_compound(std::string_view name = "");
    void end_compound();

    bool begin_list(std::string_view name = "");
    void end_list();

    void write_byte(int8_t b, std::string_view name = "");
    void write_short(int16_t s, std::string_view name = "");
    void write_int(int32_t i, std::string_view name = "");
    void write_long(int64_t l, std::string_view name = "");
    void write_float(float f, std::string_view name = "");
    void write_double(double d, std::string_view name = "");
    void write_byte_array(int8_t const* array, int32_t length, std::string_view name = "");
    void write_int_array(int32_t const* array, int32_t count, std::string_view name = "");
    void write_long_array(int64_t const* array, int32_t count, std::string_view name = "");
    void write_string(std::string_view str, std::string_view name = "");

    void end();

protected:
    void begin(std::string_view rootName = "");
    bool is_end() const;

    data_store dataStore;

    struct TemporaryContainer {
        TAG type;
        internal::pools<tag_payload_t::List, tag_payload_t::Compound> data;
    };

    std::stack<TemporaryContainer> temporaryContainers;

    struct ContainerInfo {
        bool named;
        TAG type;
        int32_t currentIndex;
        struct NamedContainer {
            internal::named_data_tag_index tagIndex;
        };
        union AnonContainer {
            tag_payload_t::List list;
            tag_payload_t::Compound compound;
        };
        struct {
            NamedContainer namedContainer;
            AnonContainer anonContainer;
            TemporaryContainer* temporaryContainer = nullptr;
        };

        TAG& Type();
        TAG Type() const;
        TAG& element_type(data_store& ds);
        int32_t count(data_store const& ds) const;
        void increment_count(data_store& ds);
        uint64_t storage(data_store const& ds) const;
        size_t& pool_index(data_store& ds);
    };

    std::stack<ContainerInfo> containers;

    template <typename T, typename Fn>
    bool write_tag(TAG type, std::string_view name, Fn valueGetter);

    template <typename T, std::enable_if_t<!std::is_invocable_v<T>, bool> = true>
    bool write_tag(TAG type, std::string_view name, T value);
};

class reader : builder {
public:
    bool import_file(std::string_view filepath);

    bool import_textfile(std::string_view filepath);
    bool import_binfile(std::string_view filepath);

    bool import_string(char const* data, uint32_t length);
    bool import_bin(uint8_t const* data, uint32_t length);

    bool open_compound(std::string_view name = "");
    void close_compound();

    bool open_list(std::string_view name = "");
    int32_t list_size() const;
    void close_list();

    int8_t read_byte(std::string_view name = "");
    int16_t read_short(std::string_view name = "");
    int32_t read_int(std::string_view name = "");
    int64_t read_long(std::string_view name = "");
    float read_float(std::string_view name = "");
    double read_double(std::string_view name = "");
    std::vector<int8_t> read_byte_array(std::string_view name = "");
    std::vector<int32_t> read_int_array(std::string_view name = "");
    std::vector<int64_t> read_long_array(std::string_view name = "");
    std::string_view read_string(std::string_view name = "");

    std::optional<int8_t> maybe_read_byte(std::string_view name = "");
    std::optional<int16_t> maybe_read_short(std::string_view name = "");
    std::optional<int32_t> maybe_read_int(std::string_view name = "");
    std::optional<int64_t> maybe_read_long(std::string_view name = "");
    std::optional<float> maybe_read_float(std::string_view name = "");
    std::optional<double> maybe_read_double(std::string_view name = "");
    std::optional<std::vector<int8_t>> maybe_read_byte_array(std::string_view name = "");
    std::optional<std::vector<int32_t>> maybe_read_int_array(std::string_view name = "");
    std::optional<std::vector<int64_t>> maybe_read_long_array(std::string_view name = "");
    std::optional<std::string_view> maybe_read_string(std::string_view name = "");

    template <typename T>
    T read(std::string_view name = "");

    template <typename T>
    std::optional<T> maybe_read(std::string_view name = "");

    std::string_view get_file_path() const { return filepath; }

    // Advanced API
public:
    int32_t count() const;

    class CompoundView;

    /*!
     * \brief An iterable view of the names of tags in the currently open compound.
     * Usage:
     *
     *  if (OpenCompound("my_compound"))
     *  {
     *    vector<std::string_view> arr {ListSize()};
     *    for (std::string_view name : Names())
     *    {
     *      arr[i] = name;
     *    }
     *    CloseCompound();
     *  }
     *
     * If no compound is open, yields a view with no elements.
     */
    CompoundView names();

    class CompoundView {
    public:
        class NameProxy {
        public:
            struct End {};
            std::string_view operator++(int) {
                std::string_view view = operator*();
                ++ntiIndex;
                return view;
            }
            NameProxy& operator++() {
                ++ntiIndex;
                return *this;
            }
            NameProxy& operator--() {
                ++ntiIndex;
                return *this;
            }
            std::string_view operator*() const { return compoundView->dataStore->namedTags[(*compoundView->namedTagIndices)[ntiIndex]].get_name(); }
            bool operator!=(End const&) const { return compoundView ? compoundView->namedTagIndices->size() != ntiIndex : true; }
            bool operator!=(NameProxy const& rhs) const { return compoundView == rhs.compoundView && ntiIndex == rhs.ntiIndex; }

        private:
            CompoundView* compoundView;
            int32_t ntiIndex = 0;
            friend CompoundView;
            NameProxy(CompoundView* view) : compoundView(view) {}
        };

        NameProxy begin() { return dataStore ? NameProxy(this) : NameProxy(nullptr); }
        NameProxy::End end() { return {}; }

    private:
        data_store const* dataStore;
        std::vector<internal::named_data_tag_index> const* namedTagIndices;
        CompoundView(data_store const* dataStore, std::vector<internal::named_data_tag_index> const* namedTagIndices) : dataStore(dataStore), namedTagIndices(namedTagIndices) {}

        friend NameProxy;
        friend CompoundView reader::names();
    };

private:
    class MemoryStream {
        size_t position = 0;
        std::vector<uint8_t> data;

    public:
        void set_contents(std::vector<uint8_t>&& inData);

        template <typename T>
        T retrieve();
        template <typename T>
        T const* retrieve_range_view(size_t count);

        void clear();
        bool has_contents() const;

        char current_byte() const;
        char lookahead_byte(int bytes) const;

        template <char... ToMatch>
        bool match_current_byte();
        template <char... ToSkip>
        void skip_bytes();
    } memoryStream;

    struct Token {
        enum class Type {
            COMPOUND_BEGIN,   // {
            COMPOUND_END,     // }
            LIST_BEGIN,       // [
            LIST_END,         // ]
            NAME_DELIM,       // :
            CONTAINER_DELIM,  // ,
            STRING,           // ("(?:[^"]|(?:\\"))+")|('(?:[^']|(?:\\'))+')|([^,\[\]{} \n]+)
            INTEGER,          // [0-9]+
            REAL,             // [0-9]+\.[0-9]+
        } type;
        std::string_view text{};
        char typeIndicator{};  // [bBsSlLfFdDI]
    };

    template <char... ToCheck>
    static bool check_byte(char byte);

    class TextTokenizer {
    public:
        TextTokenizer(MemoryStream& stream);

        void tokenize();

        Token const& get_current() const;

        bool match(Token::Type type);

    private:
        MemoryStream& stream;
        std::vector<Token> tokens;
        size_t current = 0;

        std::optional<Token> parse_token();

        std::optional<Token> try_parse_list_open();
        std::optional<Token> try_parse_number();
        std::optional<Token> try_parse_string();
    };

    TextTokenizer* textTokenizer = nullptr;

    std::string filepath;

    bool inVirtualRootCompound = false;

    void clear();

    bool import_uncompressed_file(std::string_view filepath);

    bool parse_text_stream();
    bool parse_bin_stream();

    TAG parse_bin_named_tag();
    bool parse_bin_payload(TAG type, std::string_view name = "");

    TAG retrieve_bin_tag();
    std::string retrieve_bin_str();
    int32_t retrieve_bin_array_len();

    TAG parse_text_named_tag();
    TAG parse_text_payload(std::string_view name = "");

    bool handle_nesting(std::string_view name, TAG t);

    bool open_container(TAG t, std::string_view name);

    template <typename T>
    T& read_value(TAG t, std::string_view name);

    template <typename T>
    std::optional<T> maybe_read_value(TAG t, std::string_view name);

    template <typename T, void (builder::*WriteArray)(T const*, int32_t, std::string_view), char...>
    friend auto packed_integer_list(reader* reader, TAG tag, std::string_view name) -> TAG;
};

class writer : public builder {
public:
    enum class PrettyPrint {
        Disabled,
        Enabled,
    };

    writer();
    ~writer();

    template <typename T>
    void write(T value, std::string_view name = "");

    bool export_text_file(std::string_view filepath, PrettyPrint prettyPrint = PrettyPrint::Enabled);
    bool export_bin_file(std::string_view filepath);

    bool export_string(std::string& out, PrettyPrint prettyPrint = PrettyPrint::Disabled);
    bool export_bin(std::vector<uint8_t>& out);

private:
    void output_bin_tag(std::vector<uint8_t>& out, named_data_tag const& tag);
    void output_bin_str(std::vector<uint8_t>& out, std::string_view str);
    void output_bin_payload(std::vector<uint8_t>& out, data_tag const& tag);

    void output_text_tag(std::ostream& out, named_data_tag const& tag);
    void output_text_str(std::ostream& out, std::string_view str);
    void output_text_payload(std::ostream& out, data_tag const& tag);

    struct TextOutputState {
        int depth = 0;
        PrettyPrint prettyPrint;
    } textOutputState{};
};

}  // namespace ME::nbt

#endif