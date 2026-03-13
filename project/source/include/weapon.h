#ifndef WEAPON_H
#define WEAPON_H

#include "raylib.h"

#define MAX_BULLETS 100

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float speed;
    float radius;
    bool active;
    float lifeTimer;
} Bullet;

typedef struct {
    Bullet bullets[MAX_BULLETS];
    float shootCooldown;
    float cooldownTimer;
} Weapon;

void InitWeapon(Weapon* weapon);
void ShootWeapon(Weapon* weapon, Vector2 startPos, Vector2 targetPos);
void UpdateWeapon(Weapon* weapon, float deltaTime, Rectangle* colliders, int colliderCount);
void DrawWeapon(Weapon* weapon);

#endif
