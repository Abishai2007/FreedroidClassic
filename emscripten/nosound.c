/* nosound.c — no-op sound stubs for the Emscripten / WASM build.
 *
 * Replaces sound.c, eliminating the SDL3_mixer dependency.
 * Init_Audio sets sound_on = FALSE so the rest of the game skips
 * all audio code paths at runtime.
 */

#define _sound_c  /* satisfies the EXTERN toggling in proto.h */

#include "system.h"
#include "defs.h"
#include "struct.h"
#include "global.h"
#include "proto.h"

void Init_Audio(void)                           { sound_on = FALSE; }
void Set_BG_Music_Volume(float v)               { (void)v; }
void Set_Sound_FX_Volume(float v)               { (void)v; }
void Switch_Background_Music_To(const char *f)  { (void)f; }
void Stop_Background_Music(void)                { }
void Play_Sound(int s)                          { (void)s; }
void GotHitSound(void)                          { }
void GotIntoBlastSound(void)                    { }
void CountdownSound(void)                       { }
void EndCountdownSound(void)                    { }
void CrySound(void)                             { }
void TransferSound(void)                        { }
void RefreshSound(void)                         { }
void MoveLiftSound(void)                        { }
void MenuItemSelectedSound(void)                { }
void MoveMenuPositionSound(void)                { }
void EnterLiftSound(void)                       { }
void LeaveLiftSound(void)                       { }
void Fire_Bullet_Sound(int s)                   { (void)s; }
void BounceSound(void)                          { }
void CollisionGotDamagedSound(void)             { }
void CollisionDamagedEnemySound(void)           { }
void DruidBlastSound(void)                      { }
void ThouArtDefeatedSound(void)                 { }
void Takeover_Set_Capsule_Sound(void)           { }
void Takeover_Game_Won_Sound(void)              { }
void Takeover_Game_Deadlock_Sound(void)         { }
void Takeover_Game_Lost_Sound(void)             { }
void FreeSounds(void)                           { }
