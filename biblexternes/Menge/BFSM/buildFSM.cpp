/*

License

Menge
All rights reserved.

Permission to use, copy, modify, and distribute this software and its documentation 
for educational, research, and non-profit purposes, without fee, and without a 
written agreement is hereby granted, provided that the above copyright notice, 
this paragraph, and the following four paragraphs appear in all copies.

This software program and documentation are copyrighted by the University of North 
Carolina at Chapel Hill. The software program and documentation are supplied "as is," 
without any accompanying services from the University of North Carolina at Chapel 
Hill or the authors. The University of North Carolina at Chapel Hill and the 
authors do not warrant that the operation of the program will be uninterrupted 
or error-free. The end-user understands that the program was developed for research 
purposes and is advised not to rely exclusively on the program for any reason.

IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS 
BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS 
DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY 
DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY STATUTORY WARRANTY 
OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND 
THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS 
TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

Any questions or comments should be sent to the authors {menge,geom}@cs.unc.edu

*/

/*!
 *	@file	buildFSM.cpp
 *	@brief	Functionality to realize Behavior FSM from configuration.
 */

#include "BFSM/FSMDescrip.h"

#include "Core.h"
#include "Agents/BaseAgent.h"
#include "Agents/Elevations/Elevation.h"
#include "Agents/Events/Event.h"
#include "Agents/Events/EventException.h"
#include "Agents/Events/EventSystem.h"
#include "Agents/SimulatorInterface.h"
#include "Agents/SimulatorState.h"
#include "Agents/SpatialQueries/SpatialQuery.h"
#include "BFSM/FSM.h"
#include "BFSM/fsmCommon.h"
#include "BFSM/State.h"
#include "BFSM/GoalSelectors/GoalSelector.h"
#include "BFSM/GoalSelectors/GoalSelectorIdentity.h"
#include "BFSM/GoalSelectors/GoalSelectorShared.h"
#include "BFSM/Transitions/Transition.h"
#include "BFSM/VelocityComponents/VelCompConst.h"
#include "resources/NavMesh.h"

namespace Menge {

	namespace BFSM {
		/////////////////////////////////////////////////////////////////////
		//                   Implementation of buildFSM
		/////////////////////////////////////////////////////////////////////

		FSM * buildFSM( FSMDescrip & fsmDescrip, Agents::SimulatorInterface * sim, bool VERBOSE ) {
			// Acquire the spatial query instance
			SPATIAL_QUERY = sim->getSpatialQuery();

			//Global Simulator interface
			SIMULATOR = sim;

			//bool valid = true;
			const size_t AGT_COUNT = sim->getNumAgents();
			FSM * fsm = new FSM( sim );

			// Build the fsm
			
			//we'll need this iterator later
			std::vector< VelModifier * >::iterator vItr;


			// Map of state names to state IDs.
			std::map< std::string, size_t > stateNameMap;
			
			// Copy the GoalSets
			fsm->_goalSets.clear();
			fsm->_goalSets.insert( fsmDescrip._goalSets.begin(), fsmDescrip._goalSets.end() );
			fsmDescrip._goalSets.clear();

			//	1. Create states
			//		a. Add velocity components and actions
			//		b. add to fsm

			std::list< StateDescrip * >::const_iterator sItr = fsmDescrip._states.begin();
			for ( ; sItr != fsmDescrip._states.end(); ++sItr ) {
				StateDescrip * sData = *sItr;
				State * s = fsmDescrip.addState( sData ); 

				if ( s == nullptr ) {
					logger << Logger::ERR_MSG << "Error creating state!";
					delete fsm;
					return nullptr;
				}
				if ( VERBOSE ) {
					logger << Logger::INFO_MSG << "\tAdding state: " << s->getName();
					logger << "(" << s->getID() << ")\n";
				}
				

				// State's goal selector
				GoalSelector * gs = sData->_goalSelector;
				if ( gs == nullptr ) {
					logger << Logger::WARN_MSG << "The state " << sData->_name;
					logger << " doesn't specify a goal selector.  "
						"The identity goal selector will be used.";
					gs = new IdentityGoalSelector();
				}

				try {
					gs->setGoalSet( fsm->getGoalSets() );	// possibly throws GoalSelectorException
					s->setGoalSelector( gs );				// possibly throws GoalSelectorException
				} catch ( GoalSelectorException const &) {
					logger << Logger::ERR_MSG << "Problem initializing the goal selector for "
						"the state " << s->getName() << ".";
					delete fsm;
					return nullptr;
				}
				sData->_goalSelector = nullptr;

				// construct each velocity component
				if ( sData->_velComponent == nullptr ) {
					logger << Logger::WARN_MSG << "The state " << sData->_name << " doesn't "
						"specify a velocity component.  The zero velocity component will be used.";
					s->setVelComponent( new ZeroVelComponent() );
				} else {
					s->setVelComponent( sData->_velComponent );
					sData->_velComponent = nullptr;
				}
				
				// transfer each action
				std::list< Action * >::iterator aItr = sData->_actions.begin();
				for ( ; aItr != sData->_actions.end(); ++aItr ) {
					s->addAction( *aItr );
				}
				sData->_actions.clear();

				//transfer velocity modifiers from the state description
				vItr = sData->_velModifiers.begin();
				for ( ; vItr != sData->_velModifiers.end(); ++vItr ) {
					s->addVelModifier( *vItr );
				}
				sData->_velModifiers.clear();


				// Set start node
				size_t stateID = fsm->addNode( s );
				stateNameMap[ sData->_name ] = stateID;
			}

			// Connect all shared goal selectors
			std::map< std::string, size_t >::iterator stateItr = stateNameMap.begin();
			for ( ; stateItr != stateNameMap.end(); ++stateItr ) {
				std::string stateName = stateItr->first;
				size_t stateID = stateItr->second;
				State * state = fsm->getNode( stateID );
				SharedGoalSelector * gs =
					dynamic_cast< SharedGoalSelector * >( state->getGoalSelector() );
				if ( gs != nullptr ) {
					if ( stateNameMap.count( gs->_stateName ) == 0 ) {
						logger << Logger::ERR_MSG << "Found shared goal selector defined on line ";
						logger << gs->_lineNo << ", but unable to locate state with the provided "
							"name: \"" << gs->_stateName << "\".";
						delete fsm;
						return nullptr;
					}
					State * src = fsm->getNode( stateNameMap[ gs->_stateName ] );
					if ( dynamic_cast< SharedGoalSelector * >( src->getGoalSelector() ) ) {
						logger << Logger::ERR_MSG << "Shared goal selector defined on line ";
						logger << gs->_lineNo << " references a state with a shared goal.  The "
							"source state must have a full goal selector definition.";
						delete fsm;
						return nullptr;
					}
					state->clearGoalSelector();
					GoalSelector * srcGS = src->getGoalSelector();
					srcGS->setPersistence( true );
					state->setGoalSelector( srcGS );
				}
			}

			if ( VERBOSE ) {
				logger << Logger::INFO_MSG << "There are " << fsmDescrip._transitions.size();
				logger << " transitions\n";
			}

			//	2. Create transitions
			std::map< std::string, std::list< Transition * > >::iterator stItr =
				fsmDescrip._transitions.begin();
			for ( ; stItr != fsmDescrip._transitions.end(); ++stItr ) {
				const std::string fromName = stItr->first;
				std::list< Transition * > & tList = stItr->second;

				// Determine if the origin state is valid
				if ( fsmDescrip._stateNameMap.find( fromName ) ==
					 fsmDescrip._stateNameMap.end() ) {
					logger << Logger::ERR_MSG << "Transition with invalid from node name: ";
					logger << fromName << ".";
					delete fsm;
					return nullptr;
				}

				// Try to connect the transitions to the destination(s)
				std::list< Transition * >::iterator tItr = tList.begin();
				for ( ; tItr != tList.end(); ++tItr ) {
					Transition * t = *tItr;
					if ( ! t->connectStates( fsmDescrip._stateNameMap ) ) {
						delete fsm;
						return nullptr;
					}
					fsm->addTransition( stateNameMap[ fromName ], t );
				}
				tList.clear();
			}
			fsmDescrip._transitions.clear();

			//////////////

			// copy over the velocity modifiers
			vItr = fsmDescrip._velModifiers.begin();
			//TODO: replace global vel mod initalizer
			for ( ; vItr != fsmDescrip._velModifiers.end(); ++vItr ) {   
				fsm->addVelModifier( *vItr );  
			}
			fsmDescrip._velModifiers.clear();

			// 3. Query simulator and fsm for possible reasons to have a task
			fsm->collectTasks();
			for ( std::list< Task * >::iterator itr = fsmDescrip._tasks.begin();
				itr != fsmDescrip._tasks.end();
				++itr ) {
				fsm->addTask( (*itr) );
			}
			fsmDescrip._tasks.clear();

			// spatial query and elevation tasks
			fsm->addTask( SPATIAL_QUERY->getTask() );
			if ( sim->getElevationInstance() ) {
				// NOTE: The elevation instance is null because none were specified.
				//	Eventually, the default elevation will be set.
				// HOWEVER, if the default ever changes such that it requires a task,
				//	this won't catch it!!!  So, make sure the default never requires
				//	a task.
				fsm->addTask( sim->getElevationInstance()->getTask() );
			}

			logger << Logger::INFO_MSG << "There are " << fsm->getTaskCount();
			logger << " registered tasks.\n";
			fsm->doTasks();

			

			//	5. Initialize all agents
			if ( VERBOSE ) logger << Logger::INFO_MSG << "Initializing agents:\n";
			Agents::SimulatorState * initState = sim->getInitialState();

			for ( size_t a = 0; a < AGT_COUNT; ++a ) {
				Agents::BaseAgent * agt = sim->getAgent( a );
				// update current state to class-appropriate value
				const std::string stateName = initState->getAgentState( agt->_id );

				std::map< std::string, size_t >::iterator stateIDItr =
					stateNameMap.find( stateName );
				if ( stateIDItr == stateNameMap.end() ) {
					logger << Logger::ERR_MSG << "Agent " << agt->_id;
					logger << " requested to start in an unknown state: " << stateName << ".";
					delete fsm;
					return nullptr;
				}
				size_t stateID = stateIDItr->second;

				// initialize velocity to preferred velocity
				State * cState = fsm->getNode( stateID );
				if ( VERBOSE ) {
					logger << Logger::INFO_MSG << "Agent " << agt->_id << " starts in ";
					logger << cState->getName() << ".";
				}
				fsm->setCurrentState( agt, stateID );
				cState->enter( agt );
				// TODO: Restore support for defining inital velocity state: zero or preferred
				agt->_vel.set( Vector2( 0.f, 0.f ) );

				//register the agent for all vel modifiers
				vItr = fsm->_velModifiers.begin();
				//TODO: replace global vel mod initalizer
				for ( ; vItr != fsm->_velModifiers.end(); ++vItr ) {
					( *vItr )->registerAgent(agt);  
				}
			}

			ACTIVE_FSM = fsm;

			return fsm;
		}
	}	// namespace BFSM
}	// namespace Menge
