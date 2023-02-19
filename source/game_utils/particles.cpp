#include "particles.h"

#include <algorithm>
#include <string>

#include "core/core.hpp"
#include "core/math/mathlib.hpp"
#include "game_resources.hpp"

inline float Deg2Rad(float a) { return a * 0.01745329252f; }

inline float Rad2Deg(float a) { return a * 57.29577951f; }

inline float clampf(float value, float min_inclusive, float max_inclusive) {
    if (min_inclusive > max_inclusive) {
        std::swap(min_inclusive, max_inclusive);
    }
    return value < min_inclusive ? min_inclusive : value < max_inclusive ? value : max_inclusive;
}

inline void normalize_point(float x, float y, metadot_vec2* out) {
    float n = x * x + y * y;
    // Already normalized.
    if (n == 1.0f) {
        return;
    }

    n = sqrt(n);
    // Too close to zero.
    if (n < 1e-5) {
        return;
    }

    n = 1.0f / n;
    out->X = x * n;
    out->Y = y * n;
}

/**
A more effect random number getter function, get from ejoy2d.
*/
inline static float RANDOM_M11(unsigned int* seed) {
    *seed = *seed * 134775813 + 1;
    union {
        uint32_t d;
        float f;
    } u;
    u.d = (((uint32_t)(*seed) & 0x7fff) << 8) | 0x40000000;
    return u.f - 3.0f;
}

ParticleSystem::ParticleSystem() {}

// implementation ParticleSystem

bool ParticleSystem::initWithTotalParticles(int numberOfParticles) {
    _totalParticles = numberOfParticles;
    _isActive = true;
    _emitterMode = Mode::GRAVITY;
    _isAutoRemoveOnFinish = false;
    _transformSystemDirty = false;

    resetTotalParticles(numberOfParticles);

    return true;
}

void ParticleSystem::resetTotalParticles(int numberOfParticles) {
    if (particle_data_.size() < numberOfParticles) {
        // particle_data_.resize(numberOfParticles);
    }
}

ParticleSystem::~ParticleSystem() {}

void ParticleSystem::addParticles(int count) {
    if (_paused) {
        return;
    }
    uint32_t RANDSEED = rand();

    int start = _particleCount;
    _particleCount += count;

    // life
    for (int i = start; i < _particleCount; ++i) {
        float theLife = _life + _lifeVar * RANDOM_M11(&RANDSEED);
        particle_data_[i].timeToLive = (std::max)(0.0f, theLife);
    }

    // position
    for (int i = start; i < _particleCount; ++i) {
        particle_data_[i].posx = _sourcePosition.X + _posVar.X * RANDOM_M11(&RANDSEED);
    }

    for (int i = start; i < _particleCount; ++i) {
        particle_data_[i].posy = _sourcePosition.Y + _posVar.Y * RANDOM_M11(&RANDSEED);
    }

    // color
#define SET_COLOR(c, b, v)                                                 \
    for (int i = start; i < _particleCount; ++i) {                         \
        particle_data_[i].c = clampf(b + v * RANDOM_M11(&RANDSEED), 0, 1); \
    }

    SET_COLOR(colorR, _startColor.r, _startColorVar.r);
    SET_COLOR(colorG, _startColor.g, _startColorVar.g);
    SET_COLOR(colorB, _startColor.b, _startColorVar.b);
    SET_COLOR(colorA, _startColor.a, _startColorVar.a);

    SET_COLOR(deltaColorR, _endColor.r, _endColorVar.r);
    SET_COLOR(deltaColorG, _endColor.g, _endColorVar.g);
    SET_COLOR(deltaColorB, _endColor.b, _endColorVar.b);
    SET_COLOR(deltaColorA, _endColor.a, _endColorVar.a);

#define SET_DELTA_COLOR(c, dc)                                                                              \
    for (int i = start; i < _particleCount; ++i) {                                                          \
        particle_data_[i].dc = (particle_data_[i].dc - particle_data_[i].c) / particle_data_[i].timeToLive; \
    }

    SET_DELTA_COLOR(colorR, deltaColorR);
    SET_DELTA_COLOR(colorG, deltaColorG);
    SET_DELTA_COLOR(colorB, deltaColorB);
    SET_DELTA_COLOR(colorA, deltaColorA);

    // size
    for (int i = start; i < _particleCount; ++i) {
        particle_data_[i].size = _startSize + _startSizeVar * RANDOM_M11(&RANDSEED);
        particle_data_[i].size = (std::max)(0.0f, particle_data_[i].size);
    }

    if (_endSize != START_SIZE_EQUAL_TO_END_SIZE) {
        for (int i = start; i < _particleCount; ++i) {
            float endSize = _endSize + _endSizeVar * RANDOM_M11(&RANDSEED);
            endSize = (std::max)(0.0f, endSize);
            particle_data_[i].deltaSize = (endSize - particle_data_[i].size) / particle_data_[i].timeToLive;
        }
    } else {
        for (int i = start; i < _particleCount; ++i) {
            particle_data_[i].deltaSize = 0.0f;
        }
    }

    // rotation
    for (int i = start; i < _particleCount; ++i) {
        particle_data_[i].rotation = _startSpin + _startSpinVar * RANDOM_M11(&RANDSEED);
    }
    for (int i = start; i < _particleCount; ++i) {
        float endA = _endSpin + _endSpinVar * RANDOM_M11(&RANDSEED);
        particle_data_[i].deltaRotation = (endA - particle_data_[i].rotation) / particle_data_[i].timeToLive;
    }

    // position
    metadot_vec2 pos;
    pos.X = x_;
    pos.Y = y_;

    for (int i = start; i < _particleCount; ++i) {
        particle_data_[i].startPosX = pos.X;
    }
    for (int i = start; i < _particleCount; ++i) {
        particle_data_[i].startPosY = pos.Y;
    }

    // Mode Gravity: A
    if (_emitterMode == Mode::GRAVITY) {

        // radial accel
        for (int i = start; i < _particleCount; ++i) {
            particle_data_[i].modeA.radialAccel = modeA.radialAccel + modeA.radialAccelVar * RANDOM_M11(&RANDSEED);
        }

        // tangential accel
        for (int i = start; i < _particleCount; ++i) {
            particle_data_[i].modeA.tangentialAccel = modeA.tangentialAccel + modeA.tangentialAccelVar * RANDOM_M11(&RANDSEED);
        }

        // rotation is dir
        if (modeA.rotationIsDir) {
            for (int i = start; i < _particleCount; ++i) {
                float a = Deg2Rad(_angle + _angleVar * RANDOM_M11(&RANDSEED));
                metadot_vec2 v{cosf(a), sinf(a)};
                float s = modeA.speed + modeA.speedVar * RANDOM_M11(&RANDSEED);
                metadot_vec2 dir = v * s;
                particle_data_[i].modeA.dirX = dir.X;  // v * s ;
                particle_data_[i].modeA.dirY = dir.Y;
                particle_data_[i].rotation = -Rad2Deg(NewMaths::vec22angle(dir));
            }
        } else {
            for (int i = start; i < _particleCount; ++i) {
                float a = Deg2Rad(_angle + _angleVar * RANDOM_M11(&RANDSEED));
                metadot_vec2 v{cosf(a), sinf(a)};
                float s = modeA.speed + modeA.speedVar * RANDOM_M11(&RANDSEED);
                metadot_vec2 dir = v * s;
                particle_data_[i].modeA.dirX = dir.X;  // v * s ;
                particle_data_[i].modeA.dirY = dir.Y;
            }
        }
    }

    // Mode Radius: B
    else {
        for (int i = start; i < _particleCount; ++i) {
            particle_data_[i].modeB.radius = modeB.startRadius + modeB.startRadiusVar * RANDOM_M11(&RANDSEED);
        }

        for (int i = start; i < _particleCount; ++i) {
            particle_data_[i].modeB.angle = Deg2Rad(_angle + _angleVar * RANDOM_M11(&RANDSEED));
        }

        for (int i = start; i < _particleCount; ++i) {
            particle_data_[i].modeB.degreesPerSecond = Deg2Rad(modeB.rotatePerSecond + modeB.rotatePerSecondVar * RANDOM_M11(&RANDSEED));
        }

        if (modeB.endRadius == START_RADIUS_EQUAL_TO_END_RADIUS) {
            for (int i = start; i < _particleCount; ++i) {
                particle_data_[i].modeB.deltaRadius = 0.0f;
            }
        } else {
            for (int i = start; i < _particleCount; ++i) {
                float endRadius = modeB.endRadius + modeB.endRadiusVar * RANDOM_M11(&RANDSEED);
                particle_data_[i].modeB.deltaRadius = (endRadius - particle_data_[i].modeB.radius) / particle_data_[i].timeToLive;
            }
        }
    }
}

void ParticleSystem::stopSystem() {
    _isActive = false;
    _elapsed = _duration;
    _emitCounter = 0;
}

void ParticleSystem::resetSystem() {
    _isActive = true;
    _elapsed = 0;
    for (int i = 0; i < _particleCount; ++i) {
        // particle_data_[i].timeToLive = 0.0f;
    }
}

bool ParticleSystem::isFull() { return (_particleCount == _totalParticles); }

// ParticleSystem - MainLoop
void ParticleSystem::update() {
    float dt = 1.0 / 25;
    if (_isActive && _emissionRate) {
        float rate = 1.0f / _emissionRate;
        int totalParticles = _totalParticles;

        // issue #1201, prevent bursts of particles, due to too high emitCounter
        if (_particleCount < totalParticles) {
            _emitCounter += dt;
            if (_emitCounter < 0.f) {
                _emitCounter = 0.f;
            }
        }

        int emitCount = (std::min)(1.0f * (totalParticles - _particleCount), _emitCounter / rate);
        addParticles(emitCount);
        _emitCounter -= rate * emitCount;

        _elapsed += dt;
        if (_elapsed < 0.f) {
            _elapsed = 0.f;
        }
        if (_duration != DURATION_INFINITY && _duration < _elapsed) {
            this->stopSystem();
        }
    }

    for (int i = 0; i < _particleCount; ++i) {
        particle_data_[i].timeToLive -= dt;
    }

    // rebirth
    for (int i = 0; i < _particleCount; ++i) {
        if (particle_data_[i].timeToLive <= 0.0f) {
            int j = _particleCount - 1;
            // while (j > 0 && particle_data_[i].timeToLive <= 0)
            //{
            //     _particleCount--;
            //     j--;
            // }
            particle_data_[i] = particle_data_[_particleCount - 1];
            --_particleCount;
        }
    }

    if (_emitterMode == Mode::GRAVITY) {
        for (int i = 0; i < _particleCount; ++i) {
            metadot_vec2 tmp, radial = {0.0f, 0.0f}, tangential;

            // radial acceleration
            if (particle_data_[i].posx || particle_data_[i].posy) {
                normalize_point(particle_data_[i].posx, particle_data_[i].posy, &radial);
            }
            tangential = radial;
            radial.X *= particle_data_[i].modeA.radialAccel;
            radial.Y *= particle_data_[i].modeA.radialAccel;

            // tangential acceleration
            std::swap(tangential.X, tangential.Y);
            tangential.X *= -particle_data_[i].modeA.tangentialAccel;
            tangential.Y *= particle_data_[i].modeA.tangentialAccel;

            // (gravity + radial + tangential) * dt
            tmp.X = radial.X + tangential.X + modeA.gravity.X;
            tmp.Y = radial.Y + tangential.Y + modeA.gravity.Y;
            tmp.X *= dt;
            tmp.Y *= dt;

            particle_data_[i].modeA.dirX += tmp.X;
            particle_data_[i].modeA.dirY += tmp.Y;

            // this is cocos2d-x v3.0
            // if (_configName.length()>0 && _yCoordFlipped != -1)

            // this is cocos2d-x v3.0
            tmp.X = particle_data_[i].modeA.dirX * dt * _yCoordFlipped;
            tmp.Y = particle_data_[i].modeA.dirY * dt * _yCoordFlipped;
            particle_data_[i].posx += tmp.X;
            particle_data_[i].posy += tmp.Y;
        }
    } else {
        for (int i = 0; i < _particleCount; ++i) {
            particle_data_[i].modeB.angle += particle_data_[i].modeB.degreesPerSecond * dt;
            particle_data_[i].modeB.radius += particle_data_[i].modeB.deltaRadius * dt;
            particle_data_[i].posx = -cosf(particle_data_[i].modeB.angle) * particle_data_[i].modeB.radius;
            particle_data_[i].posy = -sinf(particle_data_[i].modeB.angle) * particle_data_[i].modeB.radius * _yCoordFlipped;
        }
    }

    // color, size, rotation
    for (int i = 0; i < _particleCount; ++i) {
        particle_data_[i].colorR += particle_data_[i].deltaColorR * dt;
        particle_data_[i].colorG += particle_data_[i].deltaColorG * dt;
        particle_data_[i].colorB += particle_data_[i].deltaColorB * dt;
        particle_data_[i].colorA += particle_data_[i].deltaColorA * dt;
        particle_data_[i].size += (particle_data_[i].deltaSize * dt);
        particle_data_[i].size = (std::max)(0.0f, particle_data_[i].size);
        particle_data_[i].rotation += particle_data_[i].deltaRotation * dt;
    }
}

// ParticleSystem - Texture protocol
void ParticleSystem::setTexture(Texture* var) {
    if (_texture != var) {
        _texture = var;
    }
}

void ParticleSystem::draw() {
    if (_texture == nullptr) {
        return;
    }
    for (int i = 0; i < _particleCount; i++) {
        auto& p = particle_data_[i];
        if (p.size <= 0 || p.colorA <= 0) {
            continue;
        }
        SDL_Rect r = {int(p.posx + p.startPosX - p.size / 2), int(p.posy + p.startPosY - p.size / 2), int(p.size), int(p.size)};
        SDL_Color c = {Uint8(p.colorR * 255), Uint8(p.colorG * 255), Uint8(p.colorB * 255), Uint8(p.colorA * 255)};
        // SDL_SetTextureColorMod(_texture, c.r, c.g, c.b);
        // SDL_SetTextureAlphaMod(_texture, c.a);
        // SDL_SetTextureBlendMode(_texture, SDL_BLENDMODE_BLEND);
        // SDL_RenderCopyEx(_renderer, _texture, nullptr, &r, p.rotation, nullptr, SDL_FLIP_NONE);
    }
    update();
}

Texture* ParticleSystem::getTexture() { return _texture; }

////don't use a transform matrix, this is faster
// void ParticleSystem::setScale(float s)
//{
//     _transformSystemDirty = true;
//     Node::setScale(s);
// }
//
// void ParticleSystem::setRotation(float newRotation)
//{
//     _transformSystemDirty = true;
//     Node::setRotation(newRotation);
// }
//
// void ParticleSystem::setScaleX(float newScaleX)
//{
//     _transformSystemDirty = true;
//     Node::setScaleX(newScaleX);
// }
//
// void ParticleSystem::setScaleY(float newScaleY)
//{
//     _transformSystemDirty = true;
//     Node::setScaleY(newScaleY);
// }

bool ParticleSystem::isPaused() const { return _paused; }

void ParticleSystem::pauseEmissions() { _paused = true; }

void ParticleSystem::resumeEmissions() { _paused = false; }