

#ifndef _METADOT_IMGUILAYER_H__
#define _METADOT_IMGUILAYER_H__

#include <iostream>
#include <map>

#include "core/core.hpp"
#include "imgui/imgui_impl.hpp"
#include "imgui_element.h"
#include "libs/rapidxml/rapidxml.hpp"

namespace ImGuiCSS {

class Context;
class ComponentContainer;
class ElementFactory;
class Element;
class ScriptState;
class Style;
class FontManager;
struct Layout;

/**
 * Customization point for file system access
 */
class FileSystem {
public:
    enum Mode { TEXT, BINARY };

    typedef void LoadCallback(char* data);

    virtual ~FileSystem() {}

    /**
     * Loads file
     *
     * @param path file path
     * @param size output size to the variable
     * @param mode file read mode
     * @return loaded data
     */
    virtual char* load(const char* path, int* size = NULL, Mode mode = Mode::TEXT) = 0;

    /**
     * Load file async
     *
     * @param path file path
     * @param callback load callback
     */
    virtual void loadAsync(const char* path, LoadCallback* callback) = 0;
};

/**
 * Default filesystem implementation
 */
class SimpleFileSystem : public FileSystem {
public:
    /**
     * Loads file
     *
     * @param path file path
     * @param size output size to the variable
     * @param mode file read mode
     * @return loaded data
     */
    char* load(const char* path, int* size = NULL, Mode mode = Mode::TEXT);

    /**
     * Load file async
     *
     * @param path file path
     * @param callback load callback
     */
    void loadAsync(const char* path, LoadCallback* callback);
};

/**
 * Texture manager can be used by elements to load images
 */
class TextureManager {
public:
    virtual ~TextureManager(){};
    /**
     * Creates texture from raw data
     *
     * @param data image data in rgba format
     * @param width texture width
     * @param height texture height
     * @returns generated texture id
     */
    virtual ImTextureID createTexture(void* data, int width, int height) = 0;

    /**
     * Deletes old texture
     */
    virtual void deleteTexture(ImTextureID id) = 0;
};

/**
 * Font manager
 */
class FontManager {
public:
    struct FontHandle {
        ImFont* font;
        ImFontConfig cfg;
        ImVector<ImWchar> glyphRanges;
        float size;
    };

    FontManager();

    /**
     * Load font from disk
     *
     * @param name Font name
     * @param path Font path
     *
     * @returns ImFont if succeed
     */
    ImFont* loadFontFromFile(const char* name, const char* path, ImVector<ImWchar> glyphRanges);

    /**
     * Activate font using name
     *
     * @param name Font name
     * @param size Font size
     *
     * @returns true if pushed
     */
    bool pushFont(const char* name);

    inline FontHandle& getFont(const char* name) { return mFonts.at(name); }

    FileSystem* fs;

private:
    typedef std::map<const char*, FontHandle> Fonts;
    Fonts mFonts;
};

/**
 * Object that keeps imvue configuration
 */
class Context {
public:
    ~Context();

    ScriptState* script;
    TextureManager* texture;
    ElementFactory* factory;
    FileSystem* fs;
    ComponentContainer* root;
    Context* parent;
    Style* style;
    FontManager* fontManager;
    Layout* layout;
    // additional userdata that will be available from all the components
    void* userdata;

    // adjust styles scale using this variable
    ImVec2 scale;
};

/**
 * Create new context
 */
Context* createContext(ElementFactory* factory, ScriptState* script = 0, TextureManager* texture = 0, FileSystem* fs = 0, void* userdata = 0);

/**
 * Clone existing context
 */
Context* newChildContext(Context* ctx);
}  // namespace ImGuiCSS

namespace ImGuiCSS {

class Component;

struct ComponentProperty {
    ImString attribute;
    // required for type validation
    ImVector<ObjectType> types;
    // default value
    Object def;
    Object validator;
    bool required;

    ComponentProperty() : required(false) {}

    void setAttr(ImString value) {
        attribute = ImString(value.size() + 1);
        attribute[0] = ':';
        ImStrncpy(&attribute.get()[1], value.get(), value.size() + 1);
    }

    const char* id() const { return &attribute.c_str()[1]; }

    bool validate(Object value) {
        if (validator) {
            Object args[] = {value};
            Object rets[1];
            validator(args, rets, 1, 1);
            return rets[0].as<bool>();
        } else if (types.size() != 0) {
            ObjectType t = value.type();
            for (int i = 0; i < types.size(); ++i) {
                if (t == (ObjectType)types[i]) {
                    return true;
                }
            }

            return false;
        }

        return true;
    }
};

typedef std::unordered_map<ImU32, ComponentProperty> ComponentProperties;

inline char* getNodeData(Context* ctx, rapidxml::xml_node<>* node) {
    rapidxml::xml_attribute<>* src = node->first_attribute("src");
    if (src) {
        char* data = ctx->fs->load(MetaEngine::Format("Data/assets/ui/imguicss/{0}", std::string(src->value())).c_str()
        );
        if (!data) {
            IMGUICSS_EXCEPTION(ImGui::ElementError, "failed to load file %s", src->value());
            return NULL;
        }

        return data;
    }

    return NULL;
}

/**
 * Loads component construction info
 * Can create new components of a type
 */
class ComponentFactory {

public:
    ComponentFactory();

    ComponentFactory(Object definition);

    Component* create();

private:
    void defineProperty(Object& key, Object& value);

    void readPropertyTypes(ComponentProperty& prop, Object value);

    ComponentProperties mProperties;
    ImString mTemplate;
    Object mData;
    bool mValid;
};

class ComponentContainer : public ContainerElement {

public:
    ComponentContainer();
    virtual ~ComponentContainer();
    /**
     * Create element using ElementFactory or tries to create Component
     *
     * @param node
     */
    Element* createElement(rapidxml::xml_node<>* node, ScriptState::Context* sctx = 0, Element* parent = 0);

    void renderBody();

protected:
    void destroy();

    virtual bool build();

    /**
     * Initialize components factories
     */
    void configureComponentFactories();
    /**
     * Creates component
     */
    Element* createComponent(rapidxml::xml_node<>* node, Context* ctx, ScriptState::Context* sctx, Element* parent);

    void parseXML(const char* data);

    rapidxml::xml_document<>* mDocument;

    char* mRawData;
    bool mMounted;

    /**
     * Override copy constructor and assignment
     */
    ComponentContainer& operator=(ComponentContainer& other) {
        if (mRefs != other.mRefs) {
            destroy();
        }

        ComponentContainer tmp(other);
        swap(*this, tmp);
        return *this;
    }

    ComponentContainer(ComponentContainer& other) : ContainerElement(other), mDocument(other.mDocument), mRawData(other.mRawData), mMounted(other.mMounted), mRefs(other.mRefs) { (*mRefs)++; }

private:
    friend void swap(ComponentContainer& first, ComponentContainer& second)  // nothrow
    {
        std::swap(first.mRefs, second.mRefs);
        std::swap(first.mDocument, second.mDocument);
        std::swap(first.mRawData, second.mRawData);
        std::swap(first.mMounted, second.mMounted);
    }

    typedef std::unordered_map<ImU32, ComponentFactory> ComponentFactories;
    ComponentFactories mComponents;
    int* mRefs;
};

/**
 * Root document
 */
class Document : public ComponentContainer {

public:
    Document(Context* ctx = 0);
    virtual ~Document();

    /**
     * Parse document
     *
     * @param data xml file, describing the document
     */
    void parse(const char* data);
};

/**
 * Child document
 */
class Component : public ComponentContainer {
public:
    Component(ImString tmpl, ComponentProperties& props, Object data);
    virtual ~Component();

    bool build();

    void configure(rapidxml::xml_node<>* node, Context* ctx, ScriptState::Context* sctx, Element* parent);

    virtual bool initAttribute(const char* id, const char* value, int flags = 0, ScriptState::Fields* fields = 0);

private:
    ComponentProperties& mProperties;
    Object mData;
};

}  // namespace ImGuiCSS
#endif
