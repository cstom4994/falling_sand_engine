// Copyright(c) 2022, KaoruXun All rights reserved.

struct RigidBody;
struct b2Body;
struct R_Target;

struct WorldEntity
{
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    int hw = 14;
    int hh = 26;
    bool ground = false;
    RigidBody *rb = nullptr;
    b2Body *body = nullptr;
    bool is_player = false;

    virtual void render(R_Target *target, int ofsX, int ofsY);
    virtual void renderLQ(R_Target *target, int ofsX, int ofsY);
    WorldEntity(bool isplayer);
    ~WorldEntity();
};

struct EntityComponents
{
};