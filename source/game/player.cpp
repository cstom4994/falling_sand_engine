
#include "game/player.hpp"

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/game.hpp"
#include "engine/game_basic.hpp"
#include "engine/game_utils/cells.h"
#include "engine/world.hpp"

namespace ME {

RigidBody::RigidBody(phy::Body *body, std::string name) {
    this->name = std::string(name);
    this->body = body;
}

RigidBody::~RigidBody() {
    // if (item) delete item;
}

// bool RigidBody::set_surface(C_Surface *sur) {
//     this->tex = sur;
//     return true;
// }

C_Surface *RigidBody::get_surface() const { return texture()->surface(); }

void RigidBody::setTexture(TextureRef tex) {
    if (NULL == tex) METADOT_WARN(std::format("RigidBody {0} be set to NULL texture", name).c_str());
    this->m_texture = tex;
}

TextureRef RigidBody::texture() const { return this->m_texture; }

R_Image *RigidBody::image() const {
    // TODO: 需不需要判断image为空?
    return this->m_image;
}

void RigidBody::updateImage(std::optional<C_Surface *> image) {

    // 先释放原有的Image
    if (this->m_image) R_FreeImage(this->m_image);

    if (image.has_value()) {
        this->m_image = R_CopyImageFromSurface(image.value());
    } else {
        this->m_image = R_CopyImageFromSurface(this->get_surface());
    }

    R_SetImageFilter(this->m_image, R_FILTER_NEAREST);
}

void RigidBody::clean() {

    if (NULL != body->shape()) delete body->shape();

    if (NULL != this->tiles) delete[] this->tiles;
    // if (NULL != this->texture) R_FreeImage(this->get_texture());
    // if (NULL != this->surface) SDL_FreeSurface(this->get_surface());
}

void Player::render(WorldEntity *we, R_Target *target, int ofsX, int ofsY) {
    if (heldItem != NULL) {
        int scaleEnt = global.game->Iso.globaldef.hd_objects ? global.game->Iso.globaldef.hd_objects_size : 1;

        MErect *ir = new MErect{(f32)(int)(ofsX + we->x + we->hw / 2.0 - heldItem->texture->surface()->w), (f32)(int)(ofsY + we->y + we->hh / 2.0 - heldItem->texture->surface()->h / 2),
                                (f32)heldItem->texture->surface()->w, (f32)heldItem->texture->surface()->h};
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
        R_BlitRectX(heldItem->image, NULL, target, ir, holdAngle, fx, fy, abs(holdAngle) > 90 ? R_FLIP_VERTICAL : R_FLIP_NONE);
        delete ir;
    }
}

void Player::renderLQ(WorldEntity *we, R_Target *target, int ofsX, int ofsY) {
    R_Rectangle(target, we->x + ofsX, we->y + ofsY, we->x + ofsX + we->hw, we->y + ofsY + we->hh, {0xff, 0xff, 0xff, 0xff});
}

MEvec2 rotate_point2(f32 cx, f32 cy, f32 angle, MEvec2 p);

void Player::setItemInHand(WorldEntity *we, Item *item, world *world) {
    RigidBody *r;
    if (heldItem != NULL) {
        phy::Rectangle *ps = new phy::Rectangle;
        // ps.SetAsBox(1, 1);
        ps->set(1.0f, 1.0f);

        f32 angle = holdAngle;

        MEvec2 pt = rotate_point2(0, 0, angle * 3.1415 / 180.0, {(f32)(heldItem->texture->surface()->w / 2.0), (f32)(heldItem->texture->surface()->h / 2.0)});

        r = world->makeRigidBody(phy::Body::BodyType::Dynamic, we->x + we->hw / 2 + world->loadZone.x - pt.x + 16 * cos((holdAngle + 180) * 3.1415f / 180.0f),
                                 we->y + we->hh / 2 + world->loadZone.y - pt.y + 16 * sin((holdAngle + 180) * 3.1415f / 180.0f), angle, ps, 1, 0.3, heldItem->texture);

        //  0 -> -w/2 -h/2
        // 90 ->  w/2 -h/2
        // 180 ->  w/2  h/2
        // 270 -> -w/2  h/2

        f32 strength = 10;
        int time = ME_gettime() - startThrow;

        if (time > 1000) time = 1000;

        strength += time / 1000.0 * 30;

        // r->body->SetLinearVelocity({(f32)(strength * (f32)cos((holdAngle + 180) * 3.1415f / 180.0f)), (f32)(strength * (f32)sin((holdAngle + 180) * 3.1415f / 180.0f)) - 10});

        r->body->velocity() = {(f32)(strength * (f32)cos((holdAngle + 180) * 3.1415f / 180.0f)), (f32)(strength * (f32)sin((holdAngle + 180) * 3.1415f / 180.0f)) - 10};

        // TODO:  23/7/17 物理相关性实现

        // b2Filter bf = {};
        // bf.categoryBits = 0x0001;
        //// bf.maskBits = 0x0000;
        // r->body->GetFixtureList()[0].SetFilterData(bf);

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

void ControableSystem::process(ecs::registry &world, const move_player_event &evt) {
    world.for_joined_components<WorldEntity, Player>(
            [&evt](ecs::entity, WorldEntity &we, Player &pl) {
                pl.renderLQ(&we, evt.g->TexturePack_.textureEntitiesLQ->target, evt.g->Iso.world->loadZone.x + (int)(we.vx * evt.thruTick), evt.g->Iso.world->loadZone.y + (int)(we.vy * evt.thruTick));
                pl.render(&we, evt.g->TexturePack_.textureEntities->target, evt.g->Iso.world->loadZone.x + (int)(we.vx * evt.thruTick), evt.g->Iso.world->loadZone.y + (int)(we.vy * evt.thruTick));
            },
            ecs::exists<Player>{} && ecs::exists<Controlable>{});
}

void WorldEntitySystem::process(ecs::registry &world, const entity_update_event &evt) {
    world.for_joined_components<WorldEntity>(
            [&evt](ecs::entity, WorldEntity &pl) {
                // entity fluid displacement & make solid

                for (int tx = 0; tx < pl.hw; tx++) {
                    for (int ty = 0; ty < pl.hh; ty++) {

                        int wx = (int)(tx + pl.x + evt.g->Iso.world->loadZone.x);
                        int wy = (int)(ty + pl.y + evt.g->Iso.world->loadZone.y);
                        if (wx < 0 || wy < 0 || wx >= evt.g->Iso.world->width || wy >= evt.g->Iso.world->height) continue;
                        if (evt.g->Iso.world->tiles[wx + wy * evt.g->Iso.world->width].mat->physicsType == PhysicsType::AIR) {
                            evt.g->Iso.world->tiles[wx + wy * evt.g->Iso.world->width] = Tiles_OBJECT;
                            evt.g->objectDelete[wx + wy * evt.g->Iso.world->width] = true;
                        } else if (evt.g->Iso.world->tiles[wx + wy * evt.g->Iso.world->width].mat->physicsType == PhysicsType::SAND ||
                                   evt.g->Iso.world->tiles[wx + wy * evt.g->Iso.world->width].mat->physicsType == PhysicsType::SOUP) {
                            evt.g->Iso.world->addCell(new CellData(evt.g->Iso.world->tiles[wx + wy * evt.g->Iso.world->width], (f32)(wx + rand() % 3 - 1 - pl.vx), (f32)(wy - abs(pl.vy)),
                                                                   (f32)(-pl.vx / 4 + (rand() % 10 - 5) / 5.0f), (f32)(-pl.vy / 4 + -(rand() % 5 + 5) / 5.0f), 0, (f32)0.1));
                            evt.g->Iso.world->tiles[wx + wy * evt.g->Iso.world->width] = Tiles_OBJECT;
                            evt.g->objectDelete[wx + wy * evt.g->Iso.world->width] = true;
                            evt.g->Iso.world->dirty[wx + wy * evt.g->Iso.world->width] = true;
                        }
                    }
                }
            },
            ecs::exists<WorldEntity>{});
}

}  // namespace ME