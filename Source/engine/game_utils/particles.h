
#pragma once

#include <string>
#include <vector>

#include "core/core.hpp"
#include "engine/mathlib.hpp"
#include "engine/renderer/gpu.hpp"
#include "game/game_datastruct.hpp"
#include "game/game_resources.hpp"

struct ParticleData {
    float posx = 0;
    float posy = 0;
    float startPosX = 0;
    float startPosY = 0;

    float colorR = 0;
    float colorG = 0;
    float colorB = 0;
    float colorA = 0;

    float deltaColorR = 0;
    float deltaColorG = 0;
    float deltaColorB = 0;
    float deltaColorA = 0;

    float size = 0;
    float deltaSize = 0;
    float rotation = 0;
    float deltaRotation = 0;
    float timeToLive = 0;
    unsigned int atlasIndex = 0;

    struct {
        float dirX = 0;
        float dirY = 0;
        float radialAccel = 0;
        float tangentialAccel = 0;
    } modeA;

    struct {
        float angle = 0;
        float degreesPerSecond = 0;
        float radius = 0;
        float deltaRadius = 0;
    } modeB;

    MaterialInstance tile{};
    F32 x = 0;
    F32 y = 0;
    F32 vx = 0;
    F32 vy = 0;
    F32 ax = 0;
    F32 ay = 0;
    F32 targetX = 0;
    F32 targetY = 0;
    F32 targetForce = 0;
    bool phase = false;
    bool temporary = false;
    int lifetime = 0;
    int fadeTime = 60;
    U8 inObjectState = 0;
    Meta::AnyFunction killCallback = []() {};

    explicit ParticleData(MaterialInstance tile, F32 x, F32 y, F32 vx, F32 vy, F32 ax, F32 ay) : tile(std::move(tile)), x(x), y(y), vx(vx), vy(vy), ax(ax), ay(ay) {}
    ParticleData(const ParticleData& part) : tile(part.tile), x(part.x), y(part.y), vx(part.vx), vy(part.vy), ax(part.ax), ay(part.ay) {}
};

// typedef void (*CC_UPDATE_PARTICLE_IMP)(id, SEL, tParticle*, Vec2);

// ParticleSystem
// Brief粒子系统基类。
// 粒子系统的属性：
// -颗粒物的排放速率。
// -重力模式(模式A)
// -重力。
// -方向。
// -速度+-方差。
// -切向加速度+-方差。
// -径向加速度+-方差。
// -半径模式(模式B)
// -起始半径+-方差。
// -端面半径+-方差。
// -旋转+-方差。
// -所有模式共有的属性：
// -生活+-生活差异。
// -起始自旋+-方差。
// -末端自旋+-方差。
// -起始大小+-方差。
// -末端尺寸+-方差。
// -开始颜色+-变化。
// -结束颜色+-变化。
// -寿命+-方差。
// -混合函数。
// -纹理。

// Emitter.RadialAccel=15
// Emitter.startSpin=0

class ParticleSystem {
public:
    enum class Mode {
        GRAVITY,
        RADIUS,
    };

    enum {
        DURATION_INFINITY = -1,
        START_SIZE_EQUAL_TO_END_SIZE = -1,
        START_RADIUS_EQUAL_TO_END_RADIUS = -1,
    };

public:
    void addParticles(int count);
    void stopSystem();
    void resetSystem();
    bool isFull();

    void setOpacityModifyRGB(bool opacityModifyRGB) { _opacityModifyRGB = opacityModifyRGB; }
    bool isOpacityModifyRGB() const { return _opacityModifyRGB; }

    Texture* getTexture();
    void setTexture(Texture* texture);
    void draw();
    void update();

    ParticleSystem();
    virtual ~ParticleSystem();

    virtual bool initWithTotalParticles(int numberOfParticles);
    virtual void resetTotalParticles(int numberOfParticles);
    virtual bool isPaused() const;
    virtual void pauseEmissions();
    virtual void resumeEmissions();

public:
    bool _isBlendAdditive = true;
    bool _isAutoRemoveOnFinish = false;
    std::string _plistFile;
    float _elapsed = 0;

    struct {
        metadot_vec2 gravity;
        float speed = 0;
        float speedVar = 0;
        float tangentialAccel = 0;
        float tangentialAccelVar = 0;
        float radialAccel = 0;
        float radialAccelVar = 0;
        bool rotationIsDir = 0;
    } modeA;

    struct {
        float startRadius = 0;
        float startRadiusVar = 0;
        float endRadius = 0;
        float endRadiusVar = 0;
        float rotatePerSecond = 0;
        float rotatePerSecondVar = 0;
    } modeB;

    std::vector<ParticleData> particle_data_;
    std::string _configName;

    float _emitCounter = 0;
    int _atlasIndex = 0;
    bool _transformSystemDirty = false;
    int _allocatedParticles = 0;
    bool _isActive = true;
    int _particleCount = 0;
    float _duration = 0;
    metadot_vec2 _sourcePosition;
    metadot_vec2 _posVar;
    float _life = 0;
    float _lifeVar = 0;
    float _angle = 0;
    float _angleVar = 0;
    Mode _emitterMode = Mode::GRAVITY;
    float _startSize = 0;
    float _startSizeVar = 0;
    float _endSize = 0;
    float _endSizeVar = 0;
    METAENGINE_Color _startColor;
    METAENGINE_Color _startColorVar;
    METAENGINE_Color _endColor;
    METAENGINE_Color _endColorVar;
    float _startSpin = 0;
    float _startSpinVar = 0;
    float _endSpin = 0;
    float _endSpinVar = 0;
    float _emissionRate = 0;
    int _totalParticles = 0;
    Texture* _texture = nullptr;
    bool _opacityModifyRGB = false;
    int _yCoordFlipped = 1;
    bool _paused = false;
    bool _sourcePositionCompatible = false;
    int x_ = 0, y_ = 0;

public:
    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
    }
};
