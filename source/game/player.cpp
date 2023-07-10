
#include "game/player.hpp"

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/game.hpp"
#include "engine/game_basic.hpp"
#include "engine/game_utils/cells.h"
#include "engine/world.hpp"

RigidBody::RigidBody(b2Body *body, std::string name) {
    this->body = body;
    this->name = std::string(name);
}

RigidBody::~RigidBody() {
    // if (item) delete item;
}

bool RigidBody::set_surface(C_Surface *sur) {
    this->surface = sur;
    return true;
}

C_Surface *RigidBody::get_surface() {
    // ME_ASSERT_E(this->surface);
    return this->surface;
}

bool RigidBody::set_texture(R_Image *tex) {
    this->texture = tex;
    return true;
}

R_Image *RigidBody::get_texture() {
    ME_ASSERT_E(this->texture);
    return this->texture;
}

void Player::render(WorldEntity *we, R_Target *target, int ofsX, int ofsY) {
    if (heldItem != NULL) {
        int scaleEnt = global.game->GameIsolate_.globaldef.hd_objects ? global.game->GameIsolate_.globaldef.hd_objects_size : 1;

        ME_rect *ir = new ME_rect{(f32)(int)(ofsX + we->x + we->hw / 2.0 - heldItem->surface->w), (f32)(int)(ofsY + we->y + we->hh / 2.0 - heldItem->surface->h / 2), (f32)heldItem->surface->w,
                                  (f32)heldItem->surface->h};
        f32 fx = (f32)(int)(-ir->x + ofsX + we->x + we->hw / 2.0);
        f32 fy = (f32)(int)(-ir->y + ofsY + we->y + we->hh / 2.0);
        fx -= heldItem->pivotX;
        ir->x += heldItem->pivotX;
        fy -= heldItem->pivotY;
        ir->y += heldItem->pivotY;
        R_SetShapeBlendMode(R_BlendPresetEnum::R_BLEND_ADD);
        // R_BlitTransformX(heldItem->texture, NULL, target, ir->x, ir->y, fp->x, fp->y, holdAngle, 1, 1);
        // SDL_RenderCopyExF(renderer, heldItem->texture, NULL, ir, holdAngle, fp, abs(holdAngle) > 90 ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
        ir->x *= scaleEnt;
        ir->y *= scaleEnt;
        ir->w *= scaleEnt;
        ir->h *= scaleEnt;
        R_BlitRectX(heldItem->texture, NULL, target, ir, holdAngle, fx, fy, abs(holdAngle) > 90 ? R_FLIP_VERTICAL : R_FLIP_NONE);
        delete ir;
    }
}

void Player::renderLQ(WorldEntity *we, R_Target *target, int ofsX, int ofsY) {
    R_Rectangle(target, we->x + ofsX, we->y + ofsY, we->x + ofsX + we->hw, we->y + ofsY + we->hh, {0xff, 0xff, 0xff, 0xff});
}

MEvec2 rotate_point2(f32 cx, f32 cy, f32 angle, MEvec2 p);

void Player::setItemInHand(WorldEntity *we, Item *item, World *world) {
    RigidBody *r;
    if (heldItem != NULL) {
        b2PolygonShape ps;
        ps.SetAsBox(1, 1);

        f32 angle = holdAngle;

        MEvec2 pt = rotate_point2(0, 0, angle * 3.1415 / 180.0, {(f32)(heldItem->surface->w / 2.0), (f32)(heldItem->surface->h / 2.0)});

        r = world->makeRigidBody(b2_dynamicBody, we->x + we->hw / 2 + world->loadZone.x - pt.x + 16 * cos((holdAngle + 180) * 3.1415f / 180.0f),
                                 we->y + we->hh / 2 + world->loadZone.y - pt.y + 16 * sin((holdAngle + 180) * 3.1415f / 180.0f), angle, ps, 1, 0.3, heldItem->surface);

        //  0 -> -w/2 -h/2
        // 90 ->  w/2 -h/2
        // 180 ->  w/2  h/2
        // 270 -> -w/2  h/2

        f32 strength = 10;
        int time = ME_gettime() - startThrow;

        if (time > 1000) time = 1000;

        strength += time / 1000.0 * 30;

        r->body->SetLinearVelocity({(f32)(strength * (f32)cos((holdAngle + 180) * 3.1415f / 180.0f)), (f32)(strength * (f32)sin((holdAngle + 180) * 3.1415f / 180.0f)) - 10});

        b2Filter bf = {};
        bf.categoryBits = 0x0001;
        // bf.maskBits = 0x0000;
        r->body->GetFixtureList()[0].SetFilterData(bf);

        r->item = heldItem;
        world->rigidBodies.push_back(r);
        world->updateRigidBodyHitbox(r);
        // SDL_DestroyTexture(heldItem->texture);
    }
    heldItem = item;
}

Player::Player() {}

Player::~Player() {
    if (heldItem) Item::deleteItem(heldItem);
}

MEvec2 rotate_point2(f32 cx, f32 cy, f32 angle, MEvec2 p) {
    f32 s = sin(angle);
    f32 c = cos(angle);

    // translate to origin
    p.x -= cx;
    p.y -= cy;

    // rotate
    f32 xn = p.x * c - p.y * s;
    f32 yn = p.x * s + p.y * c;

    // translate back
    return MEvec2(xn + cx, yn + cy);
}

void ControableSystem::process(MetaEngine::ECS::registry &world, const move_player_event &evt) {
    world.for_joined_components<WorldEntity, Player>(
            [&evt](MetaEngine::ECS::entity, WorldEntity &we, Player &pl) {
                pl.renderLQ(&we, evt.g->TexturePack_.textureEntitiesLQ->target, evt.g->GameIsolate_.world->loadZone.x + (int)(we.vx * evt.thruTick),
                            evt.g->GameIsolate_.world->loadZone.y + (int)(we.vy * evt.thruTick));
                pl.render(&we, evt.g->TexturePack_.textureEntities->target, evt.g->GameIsolate_.world->loadZone.x + (int)(we.vx * evt.thruTick),
                          evt.g->GameIsolate_.world->loadZone.y + (int)(we.vy * evt.thruTick));
            },
            MetaEngine::ECS::exists<Player>{} && MetaEngine::ECS::exists<Controlable>{});
}

void WorldEntitySystem::process(MetaEngine::ECS::registry &world, const entity_update_event &evt) {
    world.for_joined_components<WorldEntity>(
            [&evt](MetaEngine::ECS::entity, WorldEntity &pl) {
                // entity fluid displacement & make solid

                for (int tx = 0; tx < pl.hw; tx++) {
                    for (int ty = 0; ty < pl.hh; ty++) {

                        int wx = (int)(tx + pl.x + evt.g->GameIsolate_.world->loadZone.x);
                        int wy = (int)(ty + pl.y + evt.g->GameIsolate_.world->loadZone.y);
                        if (wx < 0 || wy < 0 || wx >= evt.g->GameIsolate_.world->width || wy >= evt.g->GameIsolate_.world->height) continue;
                        if (evt.g->GameIsolate_.world->tiles[wx + wy * evt.g->GameIsolate_.world->width].mat->physicsType == PhysicsType::AIR) {
                            evt.g->GameIsolate_.world->tiles[wx + wy * evt.g->GameIsolate_.world->width] = Tiles_OBJECT;
                            evt.g->objectDelete[wx + wy * evt.g->GameIsolate_.world->width] = true;
                        } else if (evt.g->GameIsolate_.world->tiles[wx + wy * evt.g->GameIsolate_.world->width].mat->physicsType == PhysicsType::SAND ||
                                   evt.g->GameIsolate_.world->tiles[wx + wy * evt.g->GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                            evt.g->GameIsolate_.world->addCell(new CellData(evt.g->GameIsolate_.world->tiles[wx + wy * evt.g->GameIsolate_.world->width], (f32)(wx + rand() % 3 - 1 - pl.vx),
                                                                            (f32)(wy - abs(pl.vy)), (f32)(-pl.vx / 4 + (rand() % 10 - 5) / 5.0f), (f32)(-pl.vy / 4 + -(rand() % 5 + 5) / 5.0f), 0,
                                                                            (f32)0.1));
                            evt.g->GameIsolate_.world->tiles[wx + wy * evt.g->GameIsolate_.world->width] = Tiles_OBJECT;
                            evt.g->objectDelete[wx + wy * evt.g->GameIsolate_.world->width] = true;
                            evt.g->GameIsolate_.world->dirty[wx + wy * evt.g->GameIsolate_.world->width] = true;
                        }
                    }
                }
            },
            MetaEngine::ECS::exists<WorldEntity>{});
}