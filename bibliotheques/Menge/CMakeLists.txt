# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2019 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE bib_menge)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(${NOM_CIBLE} STATIC
    Agents/AgentGenerators/AgentGenerator.cpp
    Agents/AgentGenerators/AgentGenerator.h
    Agents/AgentGenerators/AgentGeneratorDatabase.cpp
    Agents/AgentGenerators/AgentGeneratorDatabase.h
    Agents/AgentGenerators/AgentGeneratorFactory.cpp
    Agents/AgentGenerators/AgentGeneratorFactory.h
    Agents/AgentGenerators/ExplicitAgentGenerator.cpp
    Agents/AgentGenerators/ExplicitAgentGenerator.h
    Agents/AgentGenerators/HexLatticeGenerator.cpp
    Agents/AgentGenerators/HexLatticeGenerator.h
    Agents/AgentGenerators/NavMeshAgentGenerator.cpp
    Agents/AgentGenerators/NavMeshAgentGenerator.h
    Agents/AgentGenerators/RectGridGenerator.cpp
    Agents/AgentGenerators/RectGridGenerator.h

    Agents/Elevations/Elevation.h
    Agents/Elevations/ElevationDatabase.cpp
    Agents/Elevations/ElevationDatabase.h
    Agents/Elevations/ElevationFactory.h
    Agents/Elevations/ElevationFlat.cpp
    Agents/Elevations/ElevationFlat.h
    Agents/Elevations/ElevationNavMesh.cpp
    Agents/Elevations/ElevationNavMesh.h

    Agents/Events/AgentEventEffect.cpp
    Agents/Events/AgentEventEffect.h
    Agents/Events/AgentEventTarget.h
    Agents/Events/AgentPropertyEffect.h
    Agents/Events/change_state_effect.cpp
    Agents/Events/change_state_effect.h
    Agents/Events/Event.cpp
    Agents/Events/Event.h
    Agents/Events/EventEffect.h
    Agents/Events/EventEffectAgentState.cpp
    Agents/Events/EventEffectAgentState.h
    Agents/Events/EventEffectDB.cpp
    Agents/Events/EventEffectDB.h
    Agents/Events/EventEffectFactory.h
    Agents/Events/EventException.h
    Agents/Events/EventResponse.cpp
    Agents/Events/EventResponse.h
    Agents/Events/EventSystem.cpp
    Agents/Events/EventSystem.h
    Agents/Events/EventTarget.cpp
    Agents/Events/EventTarget.h
    Agents/Events/EventTargetDB.cpp
    Agents/Events/EventTargetDB.h
    Agents/Events/EventTargetFactory.cpp
    Agents/Events/EventTargetFactory.h
    Agents/Events/EventTrigger.cpp
    Agents/Events/EventTrigger.h
    Agents/Events/EventTriggerDB.cpp
    Agents/Events/EventTriggerDB.h
    Agents/Events/EventTriggerExternal.cpp
    Agents/Events/EventTriggerExternal.h
    Agents/Events/EventTriggerFactory.cpp
    Agents/Events/EventTriggerFactory.h
    Agents/Events/StateEvtTrigger.cpp
    Agents/Events/StateEvtTrigger.h
    Agents/Events/StateMemberTarget.cpp
    Agents/Events/StateMemberTarget.h
    Agents/Events/state_population_trigger.cpp
    Agents/Events/state_population_trigger.h
    Agents/Events/TargetAgentById.cpp
    Agents/Events/TargetAgentById.h

    Agents/ObstacleSets/ExplicitObstacleSet.cpp
    Agents/ObstacleSets/ExplicitObstacleSet.h
    Agents/ObstacleSets/ListObstacleSet.cpp
    Agents/ObstacleSets/ListObstacleSet.h
    Agents/ObstacleSets/NavMeshObstacleSet.cpp
    Agents/ObstacleSets/NavMeshObstacleSet.h
    Agents/ObstacleSets/ObstacleSet.cpp
    Agents/ObstacleSets/ObstacleSet.h
    Agents/ObstacleSets/ObstacleSetDatabase.cpp
    Agents/ObstacleSets/ObstacleSetDatabase.h
    Agents/ObstacleSets/ObstacleSetFactory.cpp
    Agents/ObstacleSets/ObstacleSetFactory.h
    Agents/ObstacleSets/ObstacleVertexList.h

    Agents/ProfileSelectors/ConstProfileSelector.cpp
    Agents/ProfileSelectors/ConstProfileSelector.h
    Agents/ProfileSelectors/ProfileSelector.cpp
    Agents/ProfileSelectors/ProfileSelector.h
    Agents/ProfileSelectors/ProfileSelectorDatabase.cpp
    Agents/ProfileSelectors/ProfileSelectorDatabase.h
    Agents/ProfileSelectors/ProfileSelectorFactory.h
    Agents/ProfileSelectors/profile_selector_random.cpp
    Agents/ProfileSelectors/profile_selector_random.h
    Agents/ProfileSelectors/profile_selector_weighted.cpp
    Agents/ProfileSelectors/profile_selector_weighted.h

    Agents/SpatialQueries/AgentKDTree.cpp
    Agents/SpatialQueries/AgentKDTree.h
    Agents/SpatialQueries/KNearestQuery.cpp
    Agents/SpatialQueries/KNearestQuery.h
    Agents/SpatialQueries/ObstacleKDTree.cpp
    Agents/SpatialQueries/ObstacleKDTree.h
    Agents/SpatialQueries/ProximityQuery.h
    Agents/SpatialQueries/SpatialQuery.cpp
    Agents/SpatialQueries/SpatialQuery.h
    Agents/SpatialQueries/SpatialQueryDatabase.cpp
    Agents/SpatialQueries/SpatialQueryDatabase.h
    Agents/SpatialQueries/SpatialQueryFactory.cpp
    Agents/SpatialQueries/SpatialQueryFactory.h
    Agents/SpatialQueries/SpatialQueryKDTree.h
    Agents/SpatialQueries/SpatialQueryNavMesh.cpp
    Agents/SpatialQueries/SpatialQueryNavMesh.h
    Agents/SpatialQueries/SpatialQueryStructs.h

    Agents/StateSelectors/ConstStateSelector.cpp
    Agents/StateSelectors/ConstStateSelector.h
    Agents/StateSelectors/StateSelector.cpp
    Agents/StateSelectors/StateSelector.h
    Agents/StateSelectors/StateSelectorDatabase.cpp
    Agents/StateSelectors/StateSelectorDatabase.h
    Agents/StateSelectors/StateSelectorFactory.h

    Agents/AgentInitializer.cpp
    Agents/AgentInitializer.h
    Agents/AgentPropertyManipulator.cpp
    Agents/AgentPropertyManipulator.h
    Agents/BaseAgent.cpp
    Agents/BaseAgent.h
    Agents/Obstacle.cpp
    Agents/Obstacle.h
    Agents/PrefVelocity.cpp
    Agents/PrefVelocity.h
    Agents/SCBWriter.cpp
    Agents/SCBWriter.h
    Agents/SimulatorBase.h
    Agents/SimulatorInterface.cpp
    Agents/SimulatorInterface.h
    Agents/SimulatorState.cpp
    Agents/SimulatorState.h
    Agents/SimXMLLoader.cpp
    Agents/SimXMLLoader.h
    Agents/XMLSimulatorBase.cpp
    Agents/XMLSimulatorBase.h

    BFSM/Actions/Action.cpp
    BFSM/Actions/Action.h
    BFSM/Actions/ActionDatabase.cpp
    BFSM/Actions/ActionDatabase.h
    BFSM/Actions/ActionFactory.cpp
    BFSM/Actions/ActionFactory.h
    BFSM/Actions/ObstacleAction.cpp
    BFSM/Actions/ObstacleAction.h
    BFSM/Actions/PropertyAction.h
    BFSM/Actions/TeleportAction.cpp
    BFSM/Actions/TeleportAction.h

    BFSM/Goals/Goal.cpp
    BFSM/Goals/Goal.h
    BFSM/Goals/GoalAABB.cpp
    BFSM/Goals/GoalAABB.h
    BFSM/Goals/GoalCircle.cpp
    BFSM/Goals/GoalCircle.h
    BFSM/Goals/GoalDatabase.cpp
    BFSM/Goals/GoalDatabase.h
    BFSM/Goals/GoalFactory.cpp
    BFSM/Goals/GoalFactory.h
    BFSM/Goals/GoalOBB.cpp
    BFSM/Goals/GoalOBB.h
    BFSM/Goals/GoalPoint.cpp
    BFSM/Goals/GoalPoint.h

    BFSM/GoalSelectors/GoalSelector.cpp
    BFSM/GoalSelectors/GoalSelector.h
    BFSM/GoalSelectors/GoalSelectorDatabase.cpp
    BFSM/GoalSelectors/GoalSelectorDatabase.h
    BFSM/GoalSelectors/GoalSelectorExplicit.cpp
    BFSM/GoalSelectors/GoalSelectorExplicit.h
    BFSM/GoalSelectors/GoalSelectorFactory.cpp
    BFSM/GoalSelectors/GoalSelectorFactory.h
    BFSM/GoalSelectors/GoalSelectorFarthest.cpp
    BFSM/GoalSelectors/GoalSelectorFarthest.h
    BFSM/GoalSelectors/GoalSelectorFarthestNM.cpp
    BFSM/GoalSelectors/GoalSelectorFarthestNM.h
    BFSM/GoalSelectors/GoalSelectorIdentity.cpp
    BFSM/GoalSelectors/GoalSelectorIdentity.h
    BFSM/GoalSelectors/GoalSelectorMirror.cpp
    BFSM/GoalSelectors/GoalSelectorMirror.h
    BFSM/GoalSelectors/GoalSelectorNearest.cpp
    BFSM/GoalSelectors/GoalSelectorNearest.h
    BFSM/GoalSelectors/GoalSelectorNearestNM.cpp
    BFSM/GoalSelectors/GoalSelectorNearestNM.h
    BFSM/GoalSelectors/GoalSelectorOffset.cpp
    BFSM/GoalSelectors/GoalSelectorOffset.h
    BFSM/GoalSelectors/GoalSelectorRandom.cpp
    BFSM/GoalSelectors/GoalSelectorRandom.h
    BFSM/GoalSelectors/GoalSelectorSet.cpp
    BFSM/GoalSelectors/GoalSelectorSet.h
    BFSM/GoalSelectors/GoalSelectorShared.cpp
    BFSM/GoalSelectors/GoalSelectorShared.h
    BFSM/GoalSelectors/GoalSelectorWeighted.cpp
    BFSM/GoalSelectors/GoalSelectorWeighted.h

    BFSM/Tasks/NavMeshLocalizerTask.cpp
    BFSM/Tasks/NavMeshLocalizerTask.h
    BFSM/Tasks/Task.cpp
    BFSM/Tasks/Task.h
    BFSM/Tasks/TaskDatabase.cpp
    BFSM/Tasks/TaskDatabase.h
    BFSM/Tasks/TaskFactory.h

    BFSM/Transitions/CondAuto.h
    BFSM/Transitions/CondBoolean.cpp
    BFSM/Transitions/CondBoolean.h
    BFSM/Transitions/CondGoal.cpp
    BFSM/Transitions/CondGoal.h
    BFSM/Transitions/Condition.h
    BFSM/Transitions/ConditionDatabase.cpp
    BFSM/Transitions/ConditionDatabase.h
    BFSM/Transitions/ConditionFactory.h
    BFSM/Transitions/CondSpace.cpp
    BFSM/Transitions/CondSpace.h
    BFSM/Transitions/CondTimer.cpp
    BFSM/Transitions/CondTimer.h
    BFSM/Transitions/Target.cpp
    BFSM/Transitions/Target.h
    BFSM/Transitions/TargetDatabase.cpp
    BFSM/Transitions/TargetDatabase.h
    BFSM/Transitions/TargetFactory.h
    BFSM/Transitions/TargetProb.cpp
    BFSM/Transitions/TargetProb.h
    BFSM/Transitions/TargetReturn.cpp
    BFSM/Transitions/TargetReturn.h
    BFSM/Transitions/Transition.cpp
    BFSM/Transitions/Transition.h

    BFSM/VelocityComponents/VelCompConst.cpp
    BFSM/VelocityComponents/VelCompConst.h
    BFSM/VelocityComponents/VelCompGoal.cpp
    BFSM/VelocityComponents/VelCompGoal.h
    BFSM/VelocityComponents/VelCompNavMesh.cpp
    BFSM/VelocityComponents/VelCompNavMesh.h
    BFSM/VelocityComponents/VelComponent.cpp
    BFSM/VelocityComponents/VelComponent.h
    BFSM/VelocityComponents/VelComponentDatabase.cpp
    BFSM/VelocityComponents/VelComponentDatabase.h
    BFSM/VelocityComponents/VelComponentFactory.h
    BFSM/VelocityComponents/VelCompRoadMap.cpp
    BFSM/VelocityComponents/VelCompRoadMap.h
    BFSM/VelocityComponents/VelCompVF.cpp
    BFSM/VelocityComponents/VelCompVF.h

    BFSM/VelocityModifiers/VelModContext.cpp
    BFSM/VelocityModifiers/VelModifier.cpp
    BFSM/VelocityModifiers/VelModifier.h
    BFSM/VelocityModifiers/VelModifierContext.h
    BFSM/VelocityModifiers/VelModifierDatabase.cpp
    BFSM/VelocityModifiers/VelModifierDatabase.h
    BFSM/VelocityModifiers/VelModifierFactory.h
    BFSM/VelocityModifiers/VelModifierScale.cpp
    BFSM/VelocityModifiers/VelModifierScale.h

    BFSM/buildFSM.cpp
    BFSM/FSM.cpp
    BFSM/FSM.h
    BFSM/fsmCommon.cpp
    BFSM/fsmCommon.h
    BFSM/FSMDescrip.cpp
    BFSM/FSMDescrip.h
    BFSM/FSMEnumeration.cpp
    BFSM/FSMEnumeration.h
    BFSM/GoalSet.cpp
    BFSM/GoalSet.h
    BFSM/State.cpp
    BFSM/State.h
    BFSM/StateDescrip.cpp
    BFSM/StateDescrip.h

    Math/consts.h
    Math/Geometry2D.cpp
    Math/Geometry2D.h
    Math/geomQuery.cpp
    Math/geomQuery.h
    Math/Line.cpp
    Math/Line.h
    Math/Matrix.cpp
    Math/Matrix.h
    Math/RandGenerator.cpp
    Math/RandGenerator.h
    Math/SimRandom.cpp
    Math/SimRandom.h
    Math/vector.h
    Math/Vector2.h
    Math/Vector3.h

    Orca/ORCA.h
    Orca/ORCAAgent.cpp
    Orca/ORCAAgent.h
    Orca/ORCADBEntry.cpp
    Orca/ORCADBEntry.h
    Orca/ORCAInitializer.cpp
    Orca/ORCAInitializer.h
    Orca/ORCASimulator.cpp
    Orca/ORCASimulator.h

    PedVO/PedVO.h
    PedVO/PedVOAgent.cpp
    PedVO/PedVOAgent.h
    PedVO/PedVODBEntry.cpp
    PedVO/PedVODBEntry.h
    PedVO/PedVOInitializer.cpp
    PedVO/PedVOInitializer.h
    PedVO/PedVOSimulator.cpp
    PedVO/PedVOSimulator.h

    PluginEngine/Attribute.cpp
    PluginEngine/Attribute.h
    PluginEngine/AttributeSet.cpp
    PluginEngine/AttributeSet.h
    PluginEngine/BasePluginEngine.h
    PluginEngine/CorePluginEngine.cpp
    PluginEngine/CorePluginEngine.h
    PluginEngine/Element.h
    PluginEngine/ElementDatabase.h
    PluginEngine/ElementFactory.h
    PluginEngine/Plugin.cpp
    PluginEngine/Plugin.h
    PluginEngine/SharedLibrary.h

    resources/Funnel.cpp
    resources/Funnel.h
    resources/Graph.cpp
    resources/Graph.h
    resources/GraphEdge.cpp
    resources/GraphEdge.h
    resources/GraphVertex.cpp
    resources/GraphVertex.h
    resources/MinHeap.cpp
    resources/MinHeap.h
    resources/NavMesh.cpp
    resources/NavMesh.h
    resources/NavMeshEdge.cpp
    resources/NavMeshEdge.h
    resources/NavMeshLocalizer.cpp
    resources/NavMeshLocalizer.h
    resources/NavMeshNode.cpp
    resources/NavMeshNode.h
    resources/NavMeshObstacle.cpp
    resources/NavMeshObstacle.h
    resources/NavMeshPoly.cpp
    resources/NavMeshPoly.h
    resources/PathPlanner.cpp
    resources/PathPlanner.h
    resources/Portal.cpp
    resources/Portal.h
    resources/PortalPath.cpp
    resources/PortalPath.h
    resources/Resource.cpp
    resources/Resource.h
    resources/ResourceManager.cpp
    resources/ResourceManager.h
    resources/RoadMapPath.cpp
    resources/RoadMapPath.h
    resources/Route.cpp
    resources/Route.h
    resources/VectorField.cpp
    resources/VectorField.h
    resources/WayPortal.cpp
    resources/WayPortal.h

    Runtime/Logger.cpp
    Runtime/Logger.h
    Runtime/os.cpp
    Runtime/os.h
    Runtime/ReadersWriterLock.cpp
    Runtime/ReadersWriterLock.h
    Runtime/SimpleLock.cpp
    Runtime/SimpleLock.h
    Runtime/SimulatorDB.cpp
    Runtime/SimulatorDB.h
    Runtime/SimulatorDBEntry.cpp
    Runtime/SimulatorDBEntry.h
    Runtime/Utils.cpp
    Runtime/Utils.h

    Core.cpp
    Core.h
    CoreConfig.h
    data_set_selector.h
	macros.h
    menge_c_api.cpp
    menge_c_api.h
    mengeCommon.h
    MengeException.h
    ProjectSpec.cpp
    ProjectSpec.h
)

target_include_directories(${NOM_CIBLE} PUBLIC ${INCLUSIONS})

get_target_property(EXTLIB_COMPILE_FLAGS ${NOM_CIBLE} COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wconversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wuseless-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Weffc++)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wsign-conversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wold-style-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wshadow)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wnon-virtual-dtor)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Woverloaded-virtual)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wpedantic)
set_target_properties(${NOM_CIBLE} PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")
