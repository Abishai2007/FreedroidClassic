/*
 *
 *   Copyright (c) 1994, 2002, 2003  Johannes Prix
 *   Copyright (c) 1994, 2002, 2003  Reinhard Prix
 *
 *
 *  This file is part of Freedroid
 *
 *  Freedroid is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Freedroid is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Freedroid; see the file COPYING. If not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 */

/*----------------------------------------------------------------------
 *
 * Desc: the main program
 *
 *----------------------------------------------------------------------*/

#define _main_c

#include <SDL3/SDL_main.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "system.h"

#include "defs.h"
#include "struct.h"
#include "global.h"
#include "proto.h"
#include "text.h"
#include "vars.h"
#include "map.h"

int ThisMessageTime;
float LastGotIntoBlastSound = 2;
float LastRefreshSound = 2;

extern bool show_cursor;
void UpdateCountersForThisFrame (void);

/* -----------------------------------------------------------------------
 * Main-loop state machine
 *
 * Emscripten cannot block inside main() — the browser owns the event
 * loop.  We therefore flatten the original two nested while-loops into
 * a single callback (main_loop_iter) that is driven either by
 * emscripten_set_main_loop() or by a plain while-loop on native builds.
 * ----------------------------------------------------------------------- */
typedef enum {
  GS_INIT_MISSION,    /* (re)start: call InitNewMission, scale rects   */
  GS_INTRO_WAIT_KEYS, /* drain any keys still held from previous play   */
  GS_INTRO_PORTRAIT,  /* animate droid portrait for SHOW_WAIT ms        */
  GS_PLAYING,         /* normal per-frame game tick                     */
} GameLoopState;

static GameLoopState loop_state   = GS_INIT_MISSION;
static Uint32        intro_start_ticks = 0;

static void
main_loop_iter (void)
{
  /* QuitProgram is set by ReactToSpecialKeys / the SDL_EVENT_QUIT
   * handler.  On Emscripten we must terminate from inside the
   * callback; on native the while-loop condition catches it too. */
  if (QuitProgram)
    {
      Terminate (0);
      return;
    }

  switch (loop_state)
    {
    case GS_INIT_MISSION:
      {
        float scale;
        InitNewMission (STANDARD_MISSION);

        if ((scale = GameConfig.scale) != 1.0)
          {
            for (int levelnum = 0; levelnum < curShip.num_levels; levelnum++)
              for (int i = 0; i < curShip.num_level_rects[levelnum]; i++)
                ScaleRect (curShip.Level_Rects[levelnum][i], scale);
            for (int i = 0; i < curShip.num_lift_rows; i++)
              ScaleRect (curShip.LiftRow_Rect[i], scale);
          }
        loop_state = GS_INTRO_WAIT_KEYS;
        break;
      }

    case GS_INTRO_WAIT_KEYS:
      /* any_key_is_pressedR pumps the SDL event queue and soft-releases
       * one held key per call, so we stay here until the slate is clear. */
      if (any_key_is_pressedR ())
        break;
      ResetMouseWheel ();
      show_droid_info (Me.type, -3, 0);
      show_droid_portrait (Cons_Droid_Rect, Me.type, DROID_ROTATION_TIME, RESET);
      intro_start_ticks = SDL_GetTicks ();
      loop_state = GS_INTRO_PORTRAIT;
      break;

    case GS_INTRO_PORTRAIT:
      update_input ();   /* pump events so FirePressedR reads fresh state */
      show_droid_portrait (Cons_Droid_Rect, Me.type, DROID_ROTATION_TIME, 0);
      if ((SDL_GetTicks () - intro_start_ticks >= SHOW_WAIT) || FirePressedR ())
        {
          ClearGraphMem ();
          DisplayBanner (NULL, NULL, BANNER_FORCE_UPDATE | BANNER_NO_SDL_UPDATE);
          SDL_UpdateWindowSurface (FD_GetWindow ());
          GameOver = FALSE;
          SDL_SetCursor (crosshair_cursor);
          SDL_ShowCursor ();
          loop_state = GS_PLAYING;
        }
      break;

    case GS_PLAYING:
      if (GameOver)
        {
          loop_state = GS_INIT_MISSION;
          break;
        }

      StartTakingTimeForFPSCalculation ();
      UpdateCountersForThisFrame ();
      ReactToSpecialKeys ();

      if (show_cursor) SDL_ShowCursor ();
      else SDL_HideCursor ();

      MoveLevelDoors ();
      AnimateRefresh ();
      ExplodeBlasts ();
      AlertLevelWarning ();
      DisplayBanner (NULL, NULL, 0);
      MoveBullets ();
      Assemble_Combat_Picture (DO_SCREEN_UPDATE);

      for (int i = 0; i < MAXBULLETS; i++)
        CheckBulletCollisions (i);

      MoveInfluence ();
      MoveEnemys ();
      CheckInfluenceWallCollisions ();
      CheckInfluenceEnemyCollision ();

      if (!CurLevel->empty)
        {
          set_time_factor (1.0);
        }
      else
        {
          if (CurLevel->color == PD_DARK)
            {
              set_time_factor (GameConfig.emptyLevelSpeedup);
            }
          else if (CurLevel->timer <= 0)
            {
              CurLevel->color = PD_DARK;
              Switch_Background_Music_To (BYCOLOR);
            }
        }

      CheckIfMissionIsComplete ();

      if (!GameConfig.HogCPU)
        SDL_Delay (1);

      ComputeFPSForThisFrame ();
      break;
    } /* switch loop_state */
}

/*-----------------------------------------------------------------
 * @Desc: the heart of the Game
 *
 * @Ret: void
 *
 *-----------------------------------------------------------------*/
int
main (int argc, char * argv[])
{
  GameOver = FALSE;
  QuitProgram = FALSE;

  debug_level = 0;       /* 0=no debug 1=first debug level (at the moment=all) */

  joy_sensitivity = 1;
  sound_on = TRUE;	 /* default value, can be overridden by command-line */

  init_keystr();

  InitFreedroid (argc, argv);   // Initialisation of global variables and arrays

  SDL_HideCursor ();

#ifdef SDL_PLATFORM_WIN32
  // spread the word :)
  Win32Disclaimer ();
#endif

#ifdef __EMSCRIPTEN__
  /* Hand control to the browser; main_loop_iter is called each frame.
   * simulate_infinite_loop=1 means this call never returns on the JS
   * side, matching the original blocking-loop behaviour. */
  emscripten_set_main_loop (main_loop_iter, 0, 1);
#else
  while (!QuitProgram)
    main_loop_iter ();
  Terminate (0);
#endif
  return (0);
}				// void main(void)

/*-----------------------------------------------------------------
@Desc: This function updates counters and is called ONCE every frame.
The counters include timers, but framerate-independence of game speed
is preserved because everything is weighted with the Frame_Time()
function.

@Ret: none
 *-----------------------------------------------------------------*/
void
UpdateCountersForThisFrame (void)
{
  // Here are some things, that were previously done by some periodic */
  // interrupt function
  ThisMessageTime++;

  LastGotIntoBlastSound += Frame_Time ();
  LastRefreshSound += Frame_Time ();
  Me.LastCrysoundTime += Frame_Time ();
  Me.timer += Frame_Time();
  if (CurLevel->timer >= 0.0) CurLevel->timer -= Frame_Time ();

  Me.LastTransferSoundTime += Frame_Time();
  Me.TextVisibleTime += Frame_Time();
  LevelDoorsNotMovedTime += Frame_Time();
  if (SkipAFewFrames) SkipAFewFrames = FALSE;

  if ( Me.firewait > 0 )
    {
      Me.firewait-=Frame_Time();
      if (Me.firewait < 0) Me.firewait=0;
    }
  if (ShipEmptyCounter > 1)
    ShipEmptyCounter--;
  if (CurLevel->empty > 2)
    CurLevel->empty--;
  if (RealScore > ShowScore)
    ShowScore++;
  if (RealScore < ShowScore)
    ShowScore--;

  // drain Death-count, responsible for Alert-state
  if (DeathCount > 0)
    DeathCount -= DeathCountDrainSpeed * Frame_Time();
  if (DeathCount < 0) DeathCount = 0;
  // and switch Alert-level according to DeathCount
  AlertLevel = (int)(DeathCount / AlertThreshold);
  if (AlertLevel > AL_RED) AlertLevel = AL_RED;
  // player gets a bonus/second in AlertLevel
  RealScore += AlertLevel * AlertBonusPerSec * Frame_Time();


  for (int i = 0; i < MAX_ENEMYS_ON_SHIP ; i++)
    {

      if (AllEnemys[i].status == OUT ) continue;

      if (AllEnemys[i].warten > 0)
	{
	  AllEnemys[i].warten -= Frame_Time() ;
	  if (AllEnemys[i].warten < 0) AllEnemys[i].warten = 0;
	}

      if (AllEnemys[i].firewait > 0)
	{
	  AllEnemys[i].firewait -= Frame_Time() ;
	  if (AllEnemys[i].firewait <= 0) AllEnemys[i].firewait=0;
	}

      AllEnemys[i].TextVisibleTime += Frame_Time();
    } // for (i=0;...

} /* UpdateCountersForThisFrame() */


#undef _main_c
