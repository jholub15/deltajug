/* -*-c++-*-
* testGameActorLibrary - This source file (.h & .cpp) - Using 'The MIT License'
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
#include <iostream>
#include "testgameactor.h"
#include <dtCore/scene.h>
#include <dtDAL/enginepropertytypes.h>
#include <dtUtil/datastream.h>
#include <dtGame/messageparameter.h>
#include <dtGame/messagetype.h>
#include <dtGame/invokable.h>

////////////////////////////////////////////////////////////////////
// Actor Component Code
////////////////////////////////////////////////////////////////////

const dtGame::ActorComponent::ACType TestActorComponent1::TYPE("TestActorComponent1");

TestActorComponent1::TestActorComponent1()
: dtGame::ActorComponent(TYPE)
, mWasAdded(false)
, mWasRemoved(false)
{
}

void TestActorComponent1::OnAddedToActor(dtGame::GameActor& actor)
{
   mWasAdded = true;
}

void TestActorComponent1::OnRemovedFromActor(dtGame::GameActor& actor)
{
   mWasRemoved = true;
}

const dtGame::ActorComponent::ACType TestActorComponent2::TYPE("TestActorComponent2");

TestActorComponent2::TestActorComponent2()
: dtGame::ActorComponent(TYPE)
, mWasAdded(false)
, mWasRemoved(false)
{
}

void TestActorComponent2::OnAddedToActor(dtGame::GameActor& actor)
{
   mWasAdded = true;
}

void TestActorComponent2::OnRemovedFromActor(dtGame::GameActor& actor)
{
   mWasRemoved = true;
}
////////////////////////////////////////////////////////////////////
// Proxy Code
////////////////////////////////////////////////////////////////////
TestGameActorProxy1::TestGameActorProxy1():ticksEnabled(false)
{
   SetClassName("TestGameActor1");
}

TestGameActorProxy1::~TestGameActorProxy1()
{

}

void TestGameActorProxy1::BuildPropertyMap()
{
   dtGame::GameActorProxy::BuildPropertyMap();

   TestGameActor1* actor = NULL;
   GetActor(actor);

   static const dtUtil::RefString PROPERTY_HAS_FIRED("Has Fired");
   static const dtUtil::RefString PROPERTY_HAS_FIRED_LABEL("Has this actor fired");
   static const dtUtil::RefString PROPERTY_HAS_FIRED_DESC("Sets/Gets if this actor has fired.");
   AddProperty(new dtDAL::BooleanActorProperty(PROPERTY_HAS_FIRED, PROPERTY_HAS_FIRED_LABEL,
      dtDAL::BooleanActorProperty::SetFuncType(actor, &TestGameActor1::SetOneIsFired),
      dtDAL::BooleanActorProperty::GetFuncType(actor, &TestGameActor1::OneIsFired),
      PROPERTY_HAS_FIRED_DESC, ""));

   static const dtUtil::RefString PROPERTY_LOCAL_TICK_COUNT("Local Tick Count");
   static const dtUtil::RefString PROPERTY_LOCAL_TICK_COUNT_LABEL("The number of local tick messages received");
   static const dtUtil::RefString PROPERTY_LOCAL_TICK_COUNT_DESC("Sets/Gets the number of remote tick messages counted.");
   AddProperty(new dtDAL::IntActorProperty(PROPERTY_LOCAL_TICK_COUNT, PROPERTY_LOCAL_TICK_COUNT_LABEL,
      dtDAL::IntActorProperty::SetFuncType(actor, &TestGameActor1::SetTickLocals),
      dtDAL::IntActorProperty::GetFuncType(actor, &TestGameActor1::GetTickLocals),
      PROPERTY_LOCAL_TICK_COUNT_DESC, ""));

   static const dtUtil::RefString PROPERTY_REMOTE_TICK_COUNT("Remote Tick Count");
   static const dtUtil::RefString PROPERTY_REMOTE_TICK_COUNT_LABEL("The number of remote tick messages received");
   static const dtUtil::RefString PROPERTY_REMOTE_TICK_COUNT_DESC("Sets/Gets the number of remote tick messages counted.");
   AddProperty(new dtDAL::IntActorProperty(PROPERTY_REMOTE_TICK_COUNT, PROPERTY_REMOTE_TICK_COUNT_LABEL,
      dtDAL::IntActorProperty::SetFuncType(actor, &TestGameActor1::SetTickRemotes),
      dtDAL::IntActorProperty::GetFuncType(actor, &TestGameActor1::GetTickRemotes),
      PROPERTY_REMOTE_TICK_COUNT_DESC, ""));

   static const dtUtil::RefString PROPERTY_TEST_ACTOR_ID("Test_Actor_Id");
   static const dtUtil::RefString PROPERTY_TEST_ACTOR_ID_LABEL("Test Actor Id");
   static const dtUtil::RefString PROPERTY_TEST_ACTOR_ID_DESC("An example linked actor property");
   AddProperty(new dtDAL::ActorIDActorProperty(*this, PROPERTY_TEST_ACTOR_ID, PROPERTY_TEST_ACTOR_ID_LABEL,
      dtDAL::ActorIDActorProperty::SetFuncType(actor, &TestGameActor1::SetTestActorId),
      dtDAL::ActorIDActorProperty::GetFuncType(actor, &TestGameActor1::GetTestActorId),
      "dtCore::Transformable",
      PROPERTY_TEST_ACTOR_ID_DESC, ""));

   //add these to the accept filter so the tests can check them
   AddPropertyToLocalUpdateAcceptFilter(PROPERTY_HAS_FIRED);
   AddPropertyToLocalUpdateAcceptFilter(PROPERTY_REMOTE_TICK_COUNT);

   // Add and remove a property so the tests can verify this works.
   AddPropertyToLocalUpdateAcceptFilter(PROPERTY_LOCAL_TICK_COUNT);
   RemovePropertyFromLocalUpdateAcceptFilter(PROPERTY_LOCAL_TICK_COUNT);
}

void TestGameActorProxy1::BuildInvokables()
{
   dtGame::GameActorProxy::BuildInvokables();

   TestGameActor1* actor = NULL;
   GetActor(actor);

   AddInvokable(*new dtGame::Invokable("Fire One",
      dtUtil::MakeFunctor(&TestGameActor1::FireOne, actor)));

   AddInvokable(*new dtGame::Invokable("Reset",
      dtUtil::MakeFunctor(&TestGameActor1::Reset, actor)));

   AddInvokable(*new dtGame::Invokable("Toggle Ticks",
      dtUtil::MakeFunctor(&TestGameActorProxy1::ToggleTicks, this)));

   //register local tick handles.
   //This is just to test local handler registration.  If you want to
   //register to receive tick messages, you would override OnEnteredWorld()
   //and add code like GetGameManager()->RegisterForMessages(dtGame::MessageType::TICK_LOCAL, *this, "Tick Local")
   RegisterForMessagesAboutSelf(dtGame::MessageType::TICK_LOCAL, dtGame::GameActorProxy::TICK_LOCAL_INVOKABLE);
   RegisterForMessagesAboutSelf(dtGame::MessageType::TICK_REMOTE, dtGame::GameActorProxy::TICK_REMOTE_INVOKABLE);
   //RegisterMessageHandler(dtGame::MessageType::TICK_LOCAL, "Tick Local");
   //RegisterMessageHandler(dtGame::MessageType::TICK_REMOTE, "Tick Remote");
   ticksEnabled = true;
}

void TestGameActorProxy1::CreateActor()
{
   SetActor(*new TestGameActor1(*this));
}

void TestGameActorProxy1::ToggleTicks(const dtGame::Message& message)
{
   if (ticksEnabled)
   {
      UnregisterForMessagesAboutSelf(dtGame::MessageType::TICK_LOCAL,
         dtGame::GameActorProxy::TICK_LOCAL_INVOKABLE);
      UnregisterForMessagesAboutSelf(dtGame::MessageType::TICK_REMOTE,
         dtGame::GameActorProxy::TICK_REMOTE_INVOKABLE);
      ticksEnabled = false;
   }
   else
   {
      RegisterForMessagesAboutSelf(dtGame::MessageType::TICK_LOCAL,
         dtGame::GameActorProxy::TICK_LOCAL_INVOKABLE);
      RegisterForMessagesAboutSelf(dtGame::MessageType::TICK_REMOTE,
         dtGame::GameActorProxy::TICK_REMOTE_INVOKABLE);
      ticksEnabled = true;
   }
}
////////////////////////////////////////////////////////////////////
// Actor Code
////////////////////////////////////////////////////////////////////
TestGameActor1::TestGameActor1(dtGame::GameActorProxy& proxy): dtGame::GameActor(proxy), fired(false), tickLocals(0), tickRemotes(0)
{
}

TestGameActor1::~TestGameActor1()
{

}

void TestGameActor1::FireOne(const dtGame::Message& message)
{
   fired = true;
}

void TestGameActor1::Reset(const dtGame::Message& message)
{
   fired = false;
}

void TestGameActor1::OnTickLocal(const dtGame::TickMessage& tickMessage)
{
   tickLocals++;
}

void TestGameActor1::OnTickRemote(const dtGame::TickMessage& tickMessage)
{
   tickRemotes++;
}
