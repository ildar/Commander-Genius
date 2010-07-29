#include "CObjectAI.h"
#include "../../../misc.h"
#include "../../../sdl/sound/CSound.h"
#include "../../../graphics/CGfxEngine.h"
#include "../../../graphics/effects/CFlash.h"
#include "CVorticon.h"

// Vorticon (all Episodes, albeit the behavior changes slightly
// depending on levelcontrol.Episode).
CVorticon::CVorticon( CMap *p_map, std::vector<CPlayer> &m_vec_Player,
		Uint32 x, Uint32 y, char hp, object_t objtype) :
CObject(p_map, x, y, objtype),
m_Difficulty(mp_Map->m_Difficulty),
m_Dark(mp_Map->m_Dark),
m_Player(m_vec_Player)
{
	 frame = 0;
	 animtimer = 0;
	 state = VORT_LOOK;
	 timer = 0;
	 dist_traveled = VORT_TRAPPED_DIST+1;
	 HealthPoints = hp;
	 canbezapped = true;

	 short Episode = g_pBehaviorEngine->getEpisode();
	 // copy in animation frame indexes for the current ep
	 if (Episode==1)
	 {
		 WalkLeftFrame = VORT1_WALK_LEFT_FRAME;
		 WalkRightFrame = VORT1_WALK_RIGHT_FRAME;
		 LookFrame = VORT1_LOOK_FRAME;
		 JumpRightFrame = VORT1_JUMP_RIGHT_FRAME;
		 JumpLeftFrame = VORT1_JUMP_LEFT_FRAME;
		 DyingFrame = VORT1_DYING_FRAME;
	 }
	 else if (Episode==2)
	 {
		 WalkLeftFrame = VORT2_WALK_LEFT_FRAME;
		 WalkRightFrame = VORT2_WALK_RIGHT_FRAME;
		 LookFrame = VORT2_LOOK_FRAME;
		 JumpRightFrame = VORT2_JUMP_RIGHT_FRAME;
		 JumpLeftFrame = VORT2_JUMP_LEFT_FRAME;
		 DyingFrame = VORT2_DYING_FRAME;
		 DeadFrame = VORT2_DEAD_FRAME;
	 }
	 else if (Episode==3)
	 {
		 WalkLeftFrame = VORT3_WALK_LEFT_FRAME;
		 WalkRightFrame = VORT3_WALK_RIGHT_FRAME;
		 LookFrame = VORT3_LOOK_FRAME;
		 JumpRightFrame = VORT3_JUMP_RIGHT_FRAME;
		 JumpLeftFrame = VORT3_JUMP_LEFT_FRAME;
		 DyingFrame = VORT3_DYING_FRAME;
		 DeadFrame = VORT3_DEAD_FRAME;
	 }
}

void CVorticon::process()
{
	bool kill=false;
	short Episode = g_pBehaviorEngine->getEpisode();

	if ( HealthPoints <= 0 && state != VORT_DYING && state != VORT2_DYING ) kill = true;
	else if (Episode==2 && mp_Map->at((getXLeftPos())>>CSF, (getYUpPos())>>CSF)==TILE_GLOWCELL)
		kill = true;

	if (kill)
	{
		inhibitfall = false;
		animtimer = 0;
		frame = 0;
		if (Episode == 1)
		{
			// White Fade and back
			g_pGfxEngine->pushEffectPtr(new CFlash(3000, 8, 0xFFFFFF, 200 ));
			state = VORT_DYING;
		}
		else
			state = VORT2_DYING;

		g_pSound->playStereofromCoord(SOUND_VORT_DIE, PLAY_NOW, scrx);
	}

	vort_reprocess: ;
	switch(state)
	{
	case VORT_JUMP:
		if (movedir == RIGHT && !blockedr)
			moveRight(VORT_WALK_SPEED);
		else if (!blockedl)
			moveLeft(VORT_WALK_SPEED);

		if (inertiay>0 && blockedd)
		{  // The Vorticon Has Landed!
			inhibitfall = 0;
			state = VORT_LOOK;
			goto vort_reprocess;
		}

		// check if the vorticon has bonked into a ceiling, if so,
		// immediately terminate the jump
		if (blockedu && inertiay < 0)
			inertiay = 0;

		// apply Y inertia
		moveYDir(inertiay);

		if (timer > VORT_JUMP_FRICTION)
		{ // slowly decrease upgoing rate
			if (inertiay<VORT_MAX_FALL_SPEED)
				inertiay+=1<<5;

			timer = 0;
		}
		else timer++;
		break;
	case VORT_LOOK:
		sprite = LookFrame + frame;

		if (animtimer > VORT_LOOK_ANIM_TIME)
		{
			if (frame>0)
			{
				if (blockedl)
				{ movedir = RIGHT; }
				else if (blockedr)
				{ movedir = LEFT; }
				else
				{ // not blocked on either side, head towards player
					if ( m_Player[0].getXPosition() < getXPosition() )
					{ movedir = LEFT; }
					else
					{ movedir = RIGHT; }
				}
				timer = 0;
				frame = 0;
				state = VORT_WALK;
			} else frame++;
			animtimer = 0;
		} else animtimer++;
		break;
	case VORT_WALK:
		dist_traveled++;
		int odds;

		switch(m_Difficulty)
		{
		case 0: odds = getProbability(VORT_JUMP_PROB_EASY); break;
		case 2: odds = getProbability(VORT_JUMP_PROB_HARD); break;
		default: odds = getProbability(VORT_JUMP_PROB); break;
		}

		if (odds)
		{  // let's jump.
			if (!m_Dark && !blockedu)
			{
				initiateJump();
				goto vort_reprocess;
			}
		}
		if (movedir==LEFT)
		{  // move left
			sprite = WalkLeftFrame + frame;

			if (!blockedl)
			{
				moveLeft(VORT_WALK_SPEED);
			}
			else
			{
				frame = 0;
				animtimer = 0;
				state = VORT_LOOK;

				// if we only traveled a tiny amount before hitting a wall, we've
				// probably fallen into a small narrow area, and we need to try
				// to jump out of it
				if (dist_traveled < VORT_TRAPPED_DIST && !m_Dark && blockedd && !blockedu)
				{
					initiateJump();
					if (rnd()&1)
					{
						inertiay = -VORT_MAX_JUMP_HEIGHT;
					}
					else
					{
						inertiay = -VORT_MIN_JUMP_HEIGHT;
					}
					goto vort_reprocess;
				}
				else dist_traveled = 0;
			}
		}
		else
		{  // move right
			sprite = WalkRightFrame + frame;

			if (!blockedr)
			{
				moveRight(VORT_WALK_SPEED);
			}
			else
			{
				frame = 0;
				animtimer = 0;
				state = VORT_LOOK;

				if (dist_traveled < VORT_TRAPPED_DIST && !m_Dark && blockedd && !blockedu)
				{
					initiateJump();
					if (rnd()&1)
					{
						inertiay = -VORT_MAX_JUMP_HEIGHT;
					}
					else
					{
						inertiay = -VORT_MIN_JUMP_HEIGHT;
					}
					goto vort_reprocess;
				}
				else dist_traveled = 0;
			}
		}
		// walk animation
		if (animtimer > VORT_WALK_ANIM_TIME)
		{
			if (frame>=3) frame=0;
			else frame++;
			animtimer = 0;
		} else animtimer++;
		break;
		case VORT_DYING:
			sprite = DyingFrame + frame;

			if (animtimer > VORT_DIE_ANIM_TIME)
			{
				frame++;
				if (frame>=6)
				{
					frame = 5;
					g_pGfxEngine->Palette.fadeto(0, FADE_SPEED_VERY_SLOW);
					if(!g_pGfxEngine->Palette.in_progress())
						dead = true;
				}

				animtimer = 0;
			} else animtimer++;
			break;
		case VORT2_DYING:
			sprite = DyingFrame;
			if (animtimer > VORT2_DIE_ANIM_TIME)
			{
				sprite = DeadFrame;
				dead = true;
			}
			else
			{
				animtimer++;
			}
			break;
		default: break;
	}
}

void CVorticon::initiateJump()
{
	// must be standing on floor to jump
	if (!blockedd) return;

	frame = 0;
	animtimer = 0;
	inertiay =
			-((rnd()%(VORT_MAX_JUMP_HEIGHT-VORT_MIN_JUMP_HEIGHT))+VORT_MIN_JUMP_HEIGHT);

	if (movedir==RIGHT)
		sprite = JumpRightFrame;
	else
		sprite = JumpLeftFrame;

	inhibitfall = true;
	state = VORT_JUMP;
}

void CVorticon::getTouchedBy(CObject &theObject)
{
	if(theObject.m_type == OBJ_PLAYER)
	{
		if(	state != VORT_DYING and
			!dead and
			state != VORT2_DYING )
		theObject.kill();
	}
}
