/*
 * CGamePassiveMode.cpp
 *
 *  Created on: 26.03.2011
 *      Author: gerstrong
 */

#include "CGamePassiveMode.h"
#include "sdl/sound/CSound.h"
#include "engine/vorticon/CPassiveVort.h"
#include "engine/galaxy/CPassive.h"
#include "engine/infoscenes/CHighScores.h"
#include "common/Menu/CMenuController.h"
#include "core/CGameLauncherMenu.h"


CGamePassiveMode::CGamePassiveMode(const std::string& DataDirectory, const int& Episode) :
m_DataDirectory(DataDirectory),
m_Episode(Episode),
m_Endgame(false),
m_Difficulty(0)
{}

void CGamePassiveMode::init()
{
	// Create mp_PassiveMode object used for the screens while Player is not playing
	if(m_Episode >= 4)
		mpPassive = new galaxy::CPassiveGalaxy();
	else
		mpPassive = new vorticon::CPassiveVort();

	if( m_Endgame == true )
	{
		m_Endgame = false;
		// TODO: Overload this function for galaxy
		if( mpPassive->init(mpPassive->TITLE) ) return;
	}
	else
	{
		if( mpPassive->init() ) return;
	}

	CEventContainer& EventContainer = g_pBehaviorEngine->m_EventList;
	EventContainer.add( new GMSwitchToGameLauncher(-1, -1) );

}

void CGamePassiveMode::switchToGamePlayMode()
{
	const int Episode = mpPassive->getEpisode();
	//const int Numplayers = mp_Passive->getNumPlayers();
	//const int Difficulty = mp_Passive->getDifficulty();
	const int Numplayers = 1;
	const int Difficulty = 1;
	std::string DataDirectory = mpPassive->getGamePath();
	g_pBehaviorEngine->m_EventList.add( new GMSwitchToPlayGameMode( Episode, Numplayers, Difficulty, DataDirectory ) );
}

void CGamePassiveMode::process()
{

	mpPassive->process();

	// Process Events

	CEventContainer& EventContainer = g_pBehaviorEngine->m_EventList;

	if(!EventContainer.empty())
	{
		if( StartGameplayEvent* pLauncher = EventContainer.occurredEvent<StartGameplayEvent>() )
		{
			EventContainer.pop_Event();
			switchToGamePlayMode();
			return;
		}

		if( EventContainer.occurredEvent<LoadGameEvent>() )
		{
			// In this case let's pop this event and add the same one, the way the loading is finished within the playgame object
			EventContainer.pop_Event();
			switchToGamePlayMode();

			// The same caught event is pushed again but this time it will be polled by the GamePlay object which in the next cycles will be running!
			EventContainer.add( new LoadGameEvent() );
		}
	}

	// check here what the player chose from the menu over the passive mode.
	// NOTE: Demo is not part of playgame anymore!!
	if(mpPassive->getchooseGame())
	{
		// TODO: Some of game resources are still not cleaned up here!
		g_pSound->unloadSoundData();
		EventContainer.add( new GMSwitchToGameLauncher(-1, -1) );
		return;
	}

	// User wants to exit. Called from the PassiveMode
	if(mpPassive->getExitEvent())
	{
		EventContainer.add( new GMQuit() );
	}


}
