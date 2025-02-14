/* -*-c++-*-
 * testAAR - testaarmessageprocessor (.h & .cpp) - Using 'The MIT License'
 * Copyright (C) 2006-2008, Alion Science and Technology Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This software was developed by Alion Science and Technology Corporation under
 * circumstances in which the U. S. Government may have rights in the software.
 *
 * William E. Johnson II
 */

#include "testaarmessageprocessor.h"
#include "testaarhud.h"
#include "testaarmessagetypes.h"
#include "testaarexceptionenum.h"
#include "testaarinput.h"
#include "testaargameevent.h"

#include <dtABC/application.h>
#include <dtCore/globals.h>
#include <dtCore/keyboard.h>
#include <dtCore/system.h>
#include <dtCore/object.h>
#include <dtCore/scene.h>
#include <dtCore/transform.h>
#include <dtGame/logcontroller.h>
#include <dtGame/actorupdatemessage.h>
#include <dtGame/logkeyframe.h>
#include <dtGame/logstatus.h>
#include <dtGame/logtag.h>
#include <dtGame/serverloggercomponent.h>
#include <dtGame/basemessages.h>
#include <dtDAL/gameeventmanager.h>
#include <dtDAL/gameevent.h>
#include <dtDAL/actortype.h>
#include <dtDAL/enginepropertytypes.h>
#include <dtActors/taskactor.h>
#include <dtActors/taskactorgameevent.h>
#include <dtActors/taskactorordered.h>
#include <dtActors/taskactorrollup.h>
#include <dtLMS/lmscomponent.h>
#include <dtUtil/mathdefines.h>
#include <dtUtil/exception.h>

#include <iostream>

TestAARMessageProcessor::TestAARMessageProcessor(dtLMS::LmsComponent& lmsComp,
                                                 dtGame::LogController& logCtrl,
                                                 dtGame::ServerLoggerComponent& srvrCtrl)
   : mLogController(&logCtrl)
   , mLmsComponent(&lmsComp)
   , mLastAutoRequestStatus(0.0)
   , mServerLogger(&srvrCtrl)
   , mPlayer(NULL)
{
}

TestAARMessageProcessor::~TestAARMessageProcessor()
{
}

void TestAARMessageProcessor::ProcessMessage(const dtGame::Message& msg)
{
   const dtGame::MessageType& type = msg.GetMessageType();

   if (type == dtGame::MessageType::TICK_LOCAL)
   {
      const dtGame::TickMessage& tick = static_cast<const dtGame::TickMessage&>(msg);
      PreFrame(tick.GetDeltaSimTime());
   }
   else if (type == dtGame::MessageType::INFO_ACTOR_DELETED)
   {
      if (mPlayer != NULL && msg.GetAboutActorId() == mPlayer->GetId())
      {
         mPlayer = NULL;
      }
   }
   else if (type == TestAARMessageType::PLACE_ACTOR)
   {
      PlaceActor();
   }
   else if (type == TestAARMessageType::PLACE_IGNORED_ACTOR)
   {
      PlaceActor(true);
   }
   else if (type == TestAARMessageType::RESET)
   {
      GetGameManager()->ChangeMap("testAAR");
   }
   else if (type == TestAARMessageType::REQUEST_ALL_CONTROLLER_UPDATES)
   {
      RequestAllControllerUpdates();
   }
   else if (type == TestAARMessageType::PRINT_TASKS)
   {
      PrintTasks();
   }
   else if (type == TestAARMessageType::UPDATE_TASK_CAMERA)
   {
      UpdateTaskCamera();
   }
   else if (type == dtGame::MessageType::INFO_ACTOR_UPDATED)
   {
      if (mPlayer != NULL && msg.GetAboutActorId() == mPlayer->GetId())
      {
         UpdatePlayerActor(static_cast<const dtGame::ActorUpdateMessage&>(msg));
      }
   }
   else if (type == dtGame::MessageType::INFO_MAP_CHANGED)
   {
      Reset();
   }

   dtGame::DefaultMessageProcessor::ProcessMessage(msg);
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::OnAddedToGM()
{
   mLogController->SignalReceivedStatus().connect_slot(this, &TestAARMessageProcessor::OnReceivedStatus);
   mLogController->SignalReceivedRejection().connect_slot(this, &TestAARMessageProcessor::OnReceivedRejection);
   mLogController->SignalReceivedTags().connect_slot(this, &TestAARMessageProcessor::OnReceivedTags);
   mLogController->SignalReceivedKeyframes().connect_slot(this, &TestAARMessageProcessor::OnReceivedKeyframes);
}

//////////////////////////////////////////////////////////////////////////
dtCore::RefPtr<dtGame::GameActorProxy>
TestAARMessageProcessor::CreateNewMovingActor(const std::string& meshName,
                                              float velocity,
                                              float turnRate,
                                              bool bSetLocation,
                                              bool ignoreRecording)
{
   if (mLogController->GetLastKnownStatus().GetStateEnum() ==
      dtGame::LogStateEnumeration::LOGGER_STATE_PLAYBACK)
   {
      return NULL;
   }

   float xScale = 0.0f, yScale = 0.0f, zScale = 0.0f;
   float xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;
   dtCore::RefPtr<dtGame::GameActorProxy> object;
   dtCore::Transform position;

   dtCore::RefPtr<const dtDAL::ActorType> playerType = GetGameManager()->FindActorType("ExampleActors", "TestPlayer");
   object = dynamic_cast<dtGame::GameActorProxy *>(GetGameManager()->CreateActor(*playerType).get());

   if (bSetLocation)
   {
      object->SetTranslation(mPlayer->GetTranslation());

      // rescale our object to make it neat.
      zScale = dtUtil::RandFloat(0.70f, 1.3f);
      xScale = dtUtil::RandFloat(0.70f, 1.3f) * zScale;
      yScale = dtUtil::RandFloat(0.70f, 1.3f) * zScale;
      //object->SetScale(osg::Vec3(xScale, yScale, zScale));

      // set initial random rotation (X = pitch, Y = roll, Z = yaw) for non rotating objects
      // don't change rotating objects cause the movement will follow the rotation, which may
      // look weird.
      if (turnRate == 0.0f)
      {
         xRot = dtUtil::RandFloat(-5.0f, 5.0f);
         yRot = dtUtil::RandFloat(-5.0f, 5.0f);
         zRot = dtUtil::RandFloat(0.0f, 360.0f);
         object->SetRotation(osg::Vec3(xRot, yRot, zRot));
      }
   }

   if (ignoreRecording)
   {
      mLogController->RequestAddIgnoredActor(object->GetId());
   }

   GetGameManager()->AddActor(*object,false,false);

   // set mesh, velocity, and turn rate
   dtDAL::StringActorProperty* prop = static_cast<dtDAL::StringActorProperty *>(object->GetProperty("mesh"));
   prop->SetValue(meshName);
   dtDAL::FloatActorProperty* velocityProp = static_cast<dtDAL::FloatActorProperty *>(object->GetProperty("velocity"));
   velocityProp->SetValue(velocity);
   dtDAL::FloatActorProperty* turnRateProp = static_cast<dtDAL::FloatActorProperty *>(object->GetProperty("turnrate"));
   turnRateProp->SetValue(turnRate);

   return object;
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::PlaceActor(bool ignored)
{
   float turn, velocity;
   float chance, chance2;
   dtCore::RefPtr<dtGame::GameActorProxy> obj;

   turn = dtUtil::RandFloat(-0.60f, 0.60f);
   if (turn < 0.1f && turn > -0.1f)
   {
      turn = 0.1f;
   }

   velocity = dtUtil::RandFloat(-12.0f, 12.0f);
   if (velocity < 0.5f && velocity > -0.5f)
   {
      velocity = 0.0f;
   }

   chance = dtUtil::RandFloat(0.0, 1.0f);

   // make only some of them move cause it causes problems computing
   // the intersection with the ground. (Performance bug..)
   chance2 = dtUtil::RandFloat(0.0f, 1.0f);
   if (chance2 <= 0.75f)
   {
      velocity = 0.0f;
   }

   std::string path;

   if (ignored)
   {
      path = dtCore::FindFileInPathList("models/ignore_me.ive");
      if (!path.empty())
      {
         obj = CreateNewMovingActor(path,velocity,turn,true,ignored);
      }
      else
      {
         LOG_ERROR("Failed to find the ignore_me model file.");
      }
   }
   else if (chance <= 0.5f)
   {
      path = dtCore::FindFileInPathList("models/physics_crate.ive");
      if (!path.empty())
      {
         obj = CreateNewMovingActor(path,velocity,turn,true,ignored);
      }
      else
      {
         LOG_ERROR("Failed to find the physics_crate model file.");
      }
   }
   else
   {
      path = dtCore::FindFileInPathList("models/physics_barrel.ive");
      if (!path.empty())
      {
         obj = CreateNewMovingActor(path,velocity,turn,true,ignored);
      }
      else
      {
         LOG_ERROR("Failed to find the physics_barrel model file.");
      }
   }

   // fire a box created event
   dtCore::RefPtr<dtGame::GameEventMessage> eventMsg = static_cast<dtGame::GameEventMessage*>
      (GetGameManager()->GetMessageFactory().CreateMessage(dtGame::MessageType::INFO_GAME_EVENT).get());
   eventMsg->SetGameEvent(*TestAARGameEvent::EVENT_BOX_PLACED);
   GetGameManager()->SendMessage(*eventMsg);
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::OnReceivedStatus(const dtGame::LogStatus& newStatus)
{
   static bool isFirstPlayback = true;

   // so we don't update again if user just requested it
   mLastAutoRequestStatus = GetGameManager()->GetSimulationTime();

   if (newStatus.GetStateEnum() == dtGame::LogStateEnumeration::LOGGER_STATE_IDLE)
   {
      isFirstPlayback = true;
   }

   //The following is a great big hack due to the fact that we do not yet
   //support the task hierarchy as actor properties.  Since the task hierarchy
   //is not wrapped within actor properties, it does not get recreated
   //properly when changing to playback mode.  However, the since the task
   //actors themselves got properly recreated, we just need to manually set
   //the hierarchy when changing from IDLE state to PLAYBACK state.
   if (newStatus.GetStateEnum() == dtGame::LogStateEnumeration::LOGGER_STATE_PLAYBACK
      && isFirstPlayback)
   {
      isFirstPlayback = false;

      dtActors::TaskActorProxy* placeObjects =
         static_cast<dtActors::TaskActorProxy*>(mLmsComponent->GetTaskByName("Place Objects (Ordered)"));

      dtActors::TaskActorProxy* movePlayerRollup =
         static_cast<dtActors::TaskActorProxy*>(mLmsComponent->GetTaskByName("Move the Player (Rollup)"));

      dtActors::TaskActorProxy* movePlayerLeft =
         static_cast<dtActors::TaskActorProxy*>(mLmsComponent->GetTaskByName("Turn Player Left"));

      dtActors::TaskActorProxy* movePlayerRight =
         static_cast<dtActors::TaskActorProxy*>(mLmsComponent->GetTaskByName("Turn Player Right"));

      dtActors::TaskActorProxy* movePlayerForward =
         static_cast<dtActors::TaskActorProxy*>(mLmsComponent->GetTaskByName("Move Player Forward"));

      dtActors::TaskActorProxy* movePlayerBack =
         static_cast<dtActors::TaskActorProxy*>(mLmsComponent->GetTaskByName("Move Player Back"));

      dtActors::TaskActorProxy* drop5Boxes =
         static_cast<dtActors::TaskActorProxy*>(mLmsComponent->GetTaskByName("Drop 5 boxes"));

      //Recreate the hierarchy...
      movePlayerRollup->AddSubTask(*movePlayerLeft);
      movePlayerRollup->AddSubTask(*movePlayerRight);
      movePlayerRollup->AddSubTask(*movePlayerForward);
      movePlayerRollup->AddSubTask(*movePlayerBack);

      placeObjects->AddSubTask(*movePlayerRollup);
      placeObjects->AddSubTask(*drop5Boxes);

      //mTaskComponent->CheckTaskHierarchy();
      mLmsComponent->CheckTaskHierarchy();
   }
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::OnReceivedRejection(const dtGame::Message& newMessage)
{
   const dtGame::ServerMessageRejected& rejMsg = static_cast<const dtGame::ServerMessageRejected&>(newMessage);

   std::ostringstream ss;
   ss << "## REJECTION RECEIVED ##: Reason[" << rejMsg.GetCause() << "]...";
   std::cout << ss.str() << std::endl;

   const dtGame::Message* causeMsg = rejMsg.GetCausingMessage();
   if (causeMsg != NULL)
   {
      ss.str("");
      std::string paramsString;
      causeMsg->ToString(paramsString);
      ss << "     CAUSE: Type[" << causeMsg->GetMessageType().GetName() << "], Params["
         << paramsString << "]";
      std::cout << ss.str() << std::endl;
   }
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::RequestAllControllerUpdates()
{
   mLogController->RequestServerGetStatus();
   mLogController->RequestServerGetKeyframes();
   mLogController->RequestServerGetTags();
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::PreFrame(const double deltaFrameTime)
{
   // roughly every 3 real seconds, we force status update so the HUD updates and doesn't look broken.
   if (mLastAutoRequestStatus > GetGameManager()->GetSimulationTime())
   {
      mLastAutoRequestStatus = GetGameManager()->GetSimulationTime();
   }
   else if ((mLastAutoRequestStatus + 3.0*GetGameManager()->GetTimeScale()) < GetGameManager()->GetSimulationTime())
   {
      RequestAllControllerUpdates();
      mLastAutoRequestStatus = GetGameManager()->GetSimulationTime();
   }
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::Reset()
{
   mLmsComponent->ClearTaskList();

   //dtCore::System::GetInstance().Step();

   TestAARGameEvent::InitEvents();

   // setup terrain
   dtCore::RefPtr<dtCore::Object> terrain = new dtCore::Object();
   std::string path = dtCore::FindFileInPathList("models/terrain_simple.ive");
   if (path.empty())
   {
      LOG_ERROR("Failed to find the terrain model.");
   }
   else
   {
      terrain->LoadFile(path);
      GetGameManager()->GetScene().AddDrawable(terrain.get());
   }

   dtCore::RefPtr<const dtDAL::ActorType> playerType = GetGameManager()->FindActorType("ExampleActors", "TestPlayer");
   dtCore::RefPtr<dtDAL::ActorProxy> player = GetGameManager()->CreateActor(*playerType);
   mPlayer = dynamic_cast<dtGame::GameActorProxy*>(player.get());
   GetGameManager()->AddActor(*mPlayer, false, false);

   dtDAL::StringActorProperty* prop = static_cast<dtDAL::StringActorProperty*>(mPlayer->GetProperty("mesh"));
   path = dtCore::FindFileInPathList("models/physics_happy_sphere.ive");
   if (!path.empty())
   {
      prop->SetValue(path);
   }
   else
   {
      LOG_ERROR("Failed to find the physics_happy_sphere file.");
   }

   dtGame::GMComponent* gmc = GetGameManager()->GetComponentByName("TestInputComponent");
   if (gmc != NULL)
   {
      static_cast<TestAARInput*>(gmc)->SetPlayerActor(*mPlayer);
   }

   //SetupTasks();
   std::vector<dtDAL::ActorProxy*> toFill;
   GetGameManager()->FindActorsByName("Move Camera", toFill);

   if (toFill.size() == 0)
   {
      LOG_ERROR("Unable to find the \"Move Camera\" task.  The application will likely fail.");
      return;
   }

   mTaskMoveCameraProxy = dynamic_cast<dtActors::TaskActorProxy*>(toFill[0]);
   if (mTaskMoveCameraProxy == NULL)
   {
      LOG_ERROR("The \"Move Camera\" actor was found but it is not a task.  The application will likely fail.");
      return;
   }
   mTaskMoveCamera = &static_cast<dtActors::TaskActor&>(mTaskMoveCameraProxy->GetGameActor());
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::PrintTasks()
{
   std::ostringstream printer;
   std::vector< dtCore::RefPtr<dtGame::GameActorProxy> > tasks;

   printer << "Number of Top Level Tasks: " << mLmsComponent->GetNumTopLevelTasks() <<
      " Total Number of Tasks: " << mLmsComponent->GetNumTasks();

   mLmsComponent->GetAllTasks(tasks);
   printer << std::endl << "Task List:" << std::endl;

   for (std::vector< dtCore::RefPtr<dtGame::GameActorProxy> >::iterator itor = tasks.begin();
        itor != tasks.end();
        ++itor)
   {
      printer << "\tTask Name: " << (*itor)->GetName() <<
         " Complete: " << (*itor)->GetProperty("Complete")->ToString() << std::endl;
   }

   std::cout << printer.str() << std::endl;
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::OnReceivedTags(const std::vector<dtGame::LogTag>& newTagList)
{
   // left this cruft in for debugging
   //std::ostringstream ss;
   //ss << "## RECEIVED TAG LIST ##: [" << newTagList.size() << "] tags";
   //std::cout << ss.str() << std::endl;
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::OnReceivedKeyframes(const std::vector<dtGame::LogKeyframe>& newKeyframesList)
{
   // left this cruft in for debugging...
   //std::ostringstream ss;
   //ss << "## RECEIVED Keyframes LIST ##: [" << newKeyframesList.size() << "] keyframes";
   //std::cout << ss.str() << std::endl;
}

//////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::UpdateTaskCamera()
{
   if (mLogController->GetLastKnownStatus().GetStateEnum() != dtGame::LogStateEnumeration::LOGGER_STATE_PLAYBACK)
   {
      if (!mTaskMoveCamera->IsComplete())
      {
         mTaskMoveCamera->SetScore(1.0);
         mTaskMoveCamera->SetComplete(true);

         dtActors::TaskActorProxy& proxy =
            static_cast<dtActors::TaskActorProxy&>(mTaskMoveCamera->GetGameActorProxy());
         proxy.NotifyActorUpdate();
      }
   }
}

///////////////////////////////////////////////////////////////////////////
void TestAARMessageProcessor::UpdatePlayerActor(const dtGame::ActorUpdateMessage& aum)
{
   dtDAL::ActorProxy* gap = GetGameManager()->FindActorById(aum.GetAboutActorId());
   if (gap != mPlayer)
   {
      return;
   }

   dtDAL::FloatActorProperty* playerVelocity = static_cast<dtDAL::FloatActorProperty*>(mPlayer->GetProperty("velocity"));
   dtDAL::FloatActorProperty* playerTurnRate = static_cast<dtDAL::FloatActorProperty*>(mPlayer->GetProperty("turnrate"));

   const dtGame::MessageParameter* mp = aum.GetUpdateParameter("Velocity");
   if (mp != NULL)
   {
      playerVelocity->SetValue(static_cast<const dtGame::FloatMessageParameter*>(mp)->GetValue());
   }

   mp = aum.GetUpdateParameter("Turn Rate");
   if (mp != NULL)
   {
      playerTurnRate->SetValue(static_cast<const dtGame::FloatMessageParameter*>(mp)->GetValue());
   }
}
