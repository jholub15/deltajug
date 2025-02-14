/*
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2005, BMH Associates, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * William E. Johnson II, David Guthrie, Curtiss Murphy
 */

#include <prefix/dtgameprefix-src.h>
#include <dtDAL/enginepropertytypes.h>
#include <dtDAL/functor.h>
#include <dtGame/message.h>
#include <dtGame/messagetype.h>
#include <dtGame/messageparameter.h>
#include <dtGame/gameactor.h>
#include <dtGame/actorupdatemessage.h>
#include <dtGame/gamemanager.h>
#include <dtGame/gmcomponent.h>
#include <dtGame/invokable.h>
#include <dtGame/basemessages.h>
#include <dtGame/exceptionenum.h>

#include <dtCore/shadergroup.h>
#include <dtCore/shaderprogram.h>
#include <dtCore/shadermanager.h>
#include <dtDAL/actortype.h>
#include <dtDAL/exceptionenum.h>

#include <dtUtil/log.h>
#include <dtUtil/exception.h>

namespace dtGame
{
   const dtUtil::RefString GameActorProxy::PROPERTY_NAME("Name");

   // invokable names
   const std::string GameActorProxy::PROCESS_MSG_INVOKABLE("Process Message");
   const std::string GameActorProxy::TICK_LOCAL_INVOKABLE("Tick Local");
   const std::string GameActorProxy::TICK_REMOTE_INVOKABLE("Tick Remote");

   ///////////////////////////////////////////

   IMPLEMENT_ENUM(GameActorProxy::Ownership);

   GameActorProxy::Ownership::Ownership(const std::string& name)
   : dtUtil::Enumeration(name)
   {
      AddInstance(this);
   }

   GameActorProxy::Ownership GameActorProxy::Ownership::SERVER_PUBLISHED("Server+Published");
   GameActorProxy::Ownership GameActorProxy::Ownership::SERVER_LOCAL("Server Local");
   GameActorProxy::Ownership GameActorProxy::Ownership::CLIENT_LOCAL("Client Local");
   GameActorProxy::Ownership GameActorProxy::Ownership::CLIENT_AND_SERVER_LOCAL("Client and Server Local");
   GameActorProxy::Ownership GameActorProxy::Ownership::PROTOTYPE("PROTOTYPE");

   ///////////////////////////////////////////

   IMPLEMENT_ENUM(GameActorProxy::LocalActorUpdatePolicy);

   GameActorProxy::LocalActorUpdatePolicy::LocalActorUpdatePolicy(const std::string& name)
   : dtUtil::Enumeration(name)
   {
      AddInstance(this);
   }

   GameActorProxy::LocalActorUpdatePolicy GameActorProxy::LocalActorUpdatePolicy::IGNORE_ALL("IGNORE_ALL");
   GameActorProxy::LocalActorUpdatePolicy GameActorProxy::LocalActorUpdatePolicy::ACCEPT_ALL("ACCEPT_ALL");
   GameActorProxy::LocalActorUpdatePolicy GameActorProxy::LocalActorUpdatePolicy::ACCEPT_WITH_PROPERTY_FILTER("ACCEPT_WITH_PROPERTY_FILTER");

   ///////////////////////////////////////////
   // Actor Proxy code
   ///////////////////////////////////////////

   /////////////////////////////////////////////////////////////////////////////
   GameActorProxy::GameActorProxy()
      : mParent(NULL)
      , mOwnership(&GameActorProxy::Ownership::SERVER_LOCAL)
      , mLocalActorUpdatePolicy(&GameActorProxy::LocalActorUpdatePolicy::ACCEPT_ALL)
      , mLogger(dtUtil::Log::GetInstance("gameactor.cpp"))
      , mIsInGM(false)
   {
      SetClassName("dtGame::GameActor");
   }

   /////////////////////////////////////////////////////////////////////////////
   GameActorProxy::~GameActorProxy()
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::Init(const dtDAL::ActorType& actorType)
   {
      BaseClass::Init(actorType);
      BuildInvokables();
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::BuildPropertyMap()
   {
      GameActor& ga = GetGameActor();

      dtDAL::PhysicalActorProxy::BuildPropertyMap();

      static const dtUtil::RefString PROPERTY_IS_GAME_ACTOR("IsGameActor");
      static const dtUtil::RefString PROPERTY_IS_GAME_ACTOR_LABEL("Is Game Actor");
      static const dtUtil::RefString PROPERTY_IS_GAME_ACTOR_DESC("Read only property that always returns true, used to show in STAGE");
      dtDAL::BooleanActorProperty *bap = new dtDAL::BooleanActorProperty(PROPERTY_IS_GAME_ACTOR, PROPERTY_IS_GAME_ACTOR_LABEL,
               dtDAL::BooleanActorProperty::SetFuncType(),
               dtDAL::BooleanActorProperty::GetFuncType(this, &GameActorProxy::IsGameActorProxy),
               PROPERTY_IS_GAME_ACTOR_DESC, "");
      bap->SetReadOnly(true);
      AddProperty(bap);

      static const dtUtil::RefString PROPERTY_IS_REMOTE("IsRemote");
      static const dtUtil::RefString PROPERTY_IS_REMOTE_LABEL("Is Remote");
      static const dtUtil::RefString PROPERTY_IS_REMOTE_DESC("Sets/Gets if a game actor is remote");
      bap = new dtDAL::BooleanActorProperty(PROPERTY_IS_REMOTE, PROPERTY_IS_REMOTE_LABEL,
               dtDAL::BooleanActorProperty::SetFuncType(),
               dtDAL::BooleanActorProperty::GetFuncType(this, &GameActorProxy::IsRemote),
               PROPERTY_IS_REMOTE_DESC, "");
      bap->SetReadOnly(true);
      AddProperty(bap);

      static const dtUtil::RefString PROPERTY_IS_PUBLISHED("IsPublished");
      static const dtUtil::RefString PROPERTY_IS_PUBLISHED_LABEL("Is Pubilshed");
      static const dtUtil::RefString PROPERTY_IS_PUBLISHED_DESC("Sets/Gets if a game actor is published");
      bap = new dtDAL::BooleanActorProperty(PROPERTY_IS_PUBLISHED, PROPERTY_IS_PUBLISHED_LABEL,
               dtDAL::BooleanActorProperty::SetFuncType(),
               dtDAL::BooleanActorProperty::GetFuncType(this, &GameActorProxy::IsPublished),
               PROPERTY_IS_PUBLISHED_DESC, "");
      bap->SetReadOnly(true);
      AddProperty(bap);

      static const dtUtil::RefString PROPERTY_INITIAL_OWNERSHIP("Initial Ownership");
      static const dtUtil::RefString PROPERTY_INITIAL_OWNERSHIP_DESC("Sets/Gets the initial ownership of the actor proxy");
      AddProperty(new dtDAL::EnumActorProperty<Ownership>(PROPERTY_INITIAL_OWNERSHIP, PROPERTY_INITIAL_OWNERSHIP,
               dtDAL::EnumActorProperty<Ownership>::SetFuncType(this, &GameActorProxy::SetInitialOwnership),
               dtDAL::EnumActorProperty<Ownership>::GetFuncType(this, &GameActorProxy::GetInitialOwnership),
               PROPERTY_INITIAL_OWNERSHIP_DESC));

      static const dtUtil::RefString PROPERTY_LOCAL_ACTOR_ACCEPT_UPDATE_POLICY("Local Actor Update Policy");
      static const dtUtil::RefString PROPERTY_LOCAL_ACTOR_ACCEPT_UPDATE_POLICY_DESC(
               "Sets/Gets the policy of what to do when a local actor receives a remote message that it was updated.");
      AddProperty(new dtDAL::EnumActorProperty<LocalActorUpdatePolicy>(PROPERTY_LOCAL_ACTOR_ACCEPT_UPDATE_POLICY,
               PROPERTY_LOCAL_ACTOR_ACCEPT_UPDATE_POLICY,
               dtDAL::EnumActorProperty<LocalActorUpdatePolicy>::SetFuncType(this, &GameActorProxy::SetLocalActorUpdatePolicy),
               dtDAL::EnumActorProperty<LocalActorUpdatePolicy>::GetFuncType(this, &GameActorProxy::GetLocalActorUpdatePolicy),
               PROPERTY_LOCAL_ACTOR_ACCEPT_UPDATE_POLICY_DESC));

      static const dtUtil::RefString PROPERTY_SHADER_GROUP("ShaderGroup");
      static const dtUtil::RefString PROPERTY_SHADER_GROUP_DESC("Sets the shader group on the game actor.");
      static const dtUtil::RefString GROUPNAME("ShaderParams");

      AddProperty(new dtDAL::StringActorProperty(PROPERTY_SHADER_GROUP, PROPERTY_SHADER_GROUP,
               dtDAL::StringActorProperty::SetFuncType(&ga, &GameActor::SetShaderGroup),
               dtDAL::StringActorProperty::GetFuncType(&ga, &GameActor::GetShaderGroup),
               PROPERTY_SHADER_GROUP_DESC,GROUPNAME));

      /** let game actor components add their properties */
      ga.BuildComponentPropertyMaps();
   }

   /////////////////////////////////////////////////////////////////////////////
   GameActor& GameActorProxy::GetGameActor()
   {
      GameActor* ga;
      GetActor(ga);
      return *ga;
   }

   /////////////////////////////////////////////////////////////////////////////
   const GameActor& GameActorProxy::GetGameActor() const
   {
      const GameActor* ga;
      GetActor(ga);
      return *ga;
   }

   /////////////////////////////////////////////////////////////////////////////
   bool GameActorProxy::IsInGM() const
   {
      return mIsInGM;
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::SetIsInGM(bool value)
   {
      mIsInGM = value;
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::SetGameManager(GameManager* gm)
   {
      mParent = gm;
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::NotifyActorUpdate()
   {
      NotifyFullActorUpdate();
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::NotifyFullActorUpdate()
   {
      if (GetGameManager() == NULL || IsRemote())
      {
         return;
      }

      dtCore::RefPtr<dtGame::Message> updateMsg =
         GetGameManager()->GetMessageFactory().CreateMessage(dtGame::MessageType::INFO_ACTOR_UPDATED);

      dtGame::ActorUpdateMessage* message = static_cast<dtGame::ActorUpdateMessage*>(updateMsg.get());

      PopulateActorUpdate(*message);
      GetGameManager()->SendMessage(*updateMsg);
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::NotifyPartialActorUpdate(const std::vector<dtUtil::RefString>& propNames)
   {
      if (GetGameManager() == NULL || IsRemote())
      {
         return;
      }

      dtCore::RefPtr<dtGame::Message> updateMsg =
         GetGameManager()->GetMessageFactory().CreateMessage(dtGame::MessageType::INFO_ACTOR_UPDATED);

      dtGame::ActorUpdateMessage *message = static_cast<dtGame::ActorUpdateMessage *>(updateMsg.get());

      PopulateActorUpdate(*message, propNames, true);
      GetGameManager()->SendMessage(*updateMsg);
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::NotifyPartialActorUpdate(const std::vector<std::string>& propNames)
   {
      // This method is DEPRECATED - we just have to convert the vector types.
      std::vector<dtUtil::RefString> refStringPropNames;
      refStringPropNames.reserve(propNames.size());
      for (unsigned i = 0; i < propNames.size(); ++i)
      {
         refStringPropNames.push_back(propNames[i]);
      }

      NotifyPartialActorUpdate(refStringPropNames);
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::NotifyPartialActorUpdate()
   {
      std::vector<dtUtil::RefString> propNames;
      GetPartialUpdateProperties(propNames);
      NotifyPartialActorUpdate(propNames);
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::GetPartialUpdateProperties(std::vector<dtUtil::RefString>& propNamesToFill)
   {
      // The default of this does nothing except log a warning.
      mLogger.LogMessage(dtUtil::Log::LOG_WARNING, __FUNCTION__, __LINE__,
         "If you use NotifyPartialActorUpdate(), you should override GetPartialUpdateProperties().");
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::PopulateActorUpdate(ActorUpdateMessage& update, const std::vector<dtUtil::RefString>& propNames)
   {
      PopulateActorUpdate(update, propNames, true);
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::PopulateActorUpdate(ActorUpdateMessage& update, const std::vector<std::string>& propNames)
   {
      // This method is DEPRECATED - we just have to convert the vector types.
      std::vector<dtUtil::RefString> refStringPropNames;
      refStringPropNames.reserve(propNames.size());
      for (unsigned i = 0; i < propNames.size(); ++i)
      {
         refStringPropNames.push_back(propNames[i]);
      }

      PopulateActorUpdate(update, refStringPropNames, true);
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::PopulateActorUpdate(ActorUpdateMessage& update)
   {
      PopulateActorUpdate(update, std::vector<dtUtil::RefString>(), false);
   }

   /////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::PopulateActorUpdate(ActorUpdateMessage& update, const std::vector<dtUtil::RefString>& propNames, bool limitProperties)
   {
      update.SetName(GetName());
      update.SetActorType(GetActorType());

      update.SetPrototypeName(GetGameActor().GetPrototypeName());

      update.SetSendingActorId(GetId());
      update.SetAboutActorId(GetId());

      if (limitProperties)
      {
         for (unsigned i = 0; i < propNames.size(); ++i)
         {
            dtDAL::ActorProperty* property = GetProperty(propNames[i]);
            if (property != NULL && !property->IsReadOnly())
            {
               try
               {
                  MessageParameter* mp = update.AddUpdateParameter(property->GetName(), property->GetDataType());
                  mp->SetFromProperty(*property);
                  //if (mp != NULL)
                  //   mp->FromString(property->GetStringValue());
               }
               catch (const dtUtil::Exception&)
               {
                  // hmm, someone should not have added a property already.
                  update.GetUpdateParameter(property->GetName());
               }
            }
         }
      }
      else
      {
         std::vector<dtDAL::ActorProperty* > toFill;
         GetPropertyList(toFill);
         for (unsigned i = 0; i < toFill.size(); ++i)
         {
            dtDAL::ActorProperty& property = *toFill[i];

            // don't send read-only properties
            if (property.IsReadOnly())
            {
               continue;
            }

            try
            {
               MessageParameter* mp = update.AddUpdateParameter(property.GetName(), property.GetDataType());
               if (mp != NULL)
                  mp->SetFromProperty(property);
               //   mp->FromString(property.GetStringValue());
            }
            catch (const dtUtil::Exception&)
            {
               // hmm, someone should not have added a property already.
               update.GetUpdateParameter(property.GetName());
            }
         }

      }
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::ApplyActorUpdate(const ActorUpdateMessage& msg, bool checkLocalUpdatePolicy)
   {
      bool isLocal = !IsRemote();
      bool filterProps = isLocal && GetLocalActorUpdatePolicy() == LocalActorUpdatePolicy::ACCEPT_WITH_PROPERTY_FILTER;

      if (checkLocalUpdatePolicy && isLocal && GetLocalActorUpdatePolicy() == LocalActorUpdatePolicy::IGNORE_ALL)
      {
         if (mLogger.IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
         {
            mLogger.LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                     "Ignoring update message on actor with type \"%s\" and name \"%s\".",
                     GetActorType().GetFullName().c_str(),
                     GetName().c_str()
            );
         }
         return;
      }

      if (!filterProps || ShouldAcceptPropertyInLocalUpdate(PROPERTY_NAME))
      {
         const std::string& nameInMessage = msg.GetName().c_str();
         if (mLogger.IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
         {
            mLogger.LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                     "Setting name on actor type \"%s\" to value \"%s\"",
                     GetActorType().GetFullName().c_str(),
                     nameInMessage.c_str()
            );
         }
         // we prevent users from setting an empty name because there are many cases where a component will
         // generate an actor update message without knowing what the actor's name is (for instance the HLA
         // component doesn't know) in which case, it would overwrite the real name with an empty string == bad.
         // But, we go ahead and allow this if it is the create message.  So, you can create an actor with an
         // empty name, but can't ever override it to an empty string after that.
         if (msg.GetMessageType() == MessageType::INFO_ACTOR_CREATED || !nameInMessage.empty())
            SetName(nameInMessage);
      }

      std::vector<const MessageParameter*> params;
      msg.GetUpdateParameters(params);

      for (unsigned int i = 0; i < params.size(); ++i)
      {
         const dtUtil::RefString& paramName = params[i]->GetName();

         if (filterProps && !ShouldAcceptPropertyInLocalUpdate(paramName))
         {
            if (mLogger.IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
            {
               mLogger.LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                        "On actor with type \"%s\" Ignoring property named \"%s\" because the actor is local,"
                        "it is set to ACCEPT_WITH_PROPERTY_FILTER, and the property is not in the accept list.",
                        GetActorType().GetFullName().c_str(),
                        paramName.c_str()
               );
            }
            continue;
         }

         const dtDAL::DataType& paramType = params[i]->GetDataType();

         dtDAL::ActorProperty* property = GetProperty(paramName);

         if (property == NULL)
         {
            LOG_WARNING(("Property \"" + paramName +
                     "\" was not found on actor type \"" +
                     GetActorType().GetFullName() +
                     "\""));
            continue;
         }

         //can't set a read-only property.
         if (property->IsReadOnly())
         {
            if (mLogger.IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
            {
               mLogger.LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                        "Not setting property \"%s\" on actor type \"%s\" to value \"%s\" because the property is read only.",
                        paramName.c_str(), GetActorType().GetFullName().c_str(),
                        params[i]->ToString().c_str()
               );
            }
            continue;
         }

         if (mLogger.IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
         {
            mLogger.LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                     "Setting property \"%s\" on actor type \"%s\" to value \"%s\"",
                     paramName.c_str(), GetActorType().GetFullName().c_str(),
                     params[i]->ToString().c_str()
            );
         }


         dtDAL::ActorActorProperty* aap = NULL;

         if (paramType == dtDAL::DataType::ACTOR)
         {
            aap = dynamic_cast<dtDAL::ActorActorProperty*>(property);
         }

         // If the property is of type ACTOR AND it is an ActorActor property not an ActorID property, it's a special case.
         if (aap != NULL)
         {
            const ActorMessageParameter* amp = static_cast<const ActorMessageParameter*>(params[i]);
            if ( GetGameManager() != NULL )
            {
               dtGame::GameActorProxy* valueProxy = GetGameManager()->FindGameActorById(amp->GetValue());
               aap->SetValue(valueProxy);
            }
            else
            {
               std::stringstream ss;
               ss << GetActorType().GetName().c_str() << "." << GetClassName().c_str()
               << " GameActorProxy (" << GetId().ToString().c_str()
               << ") could not access the GameManager." << std::endl;
               mLogger.LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__, ss.str() );
            }
         }
         else
         {
            try
            {
               params[i]->ApplyValueToProperty(*property);
            }
            catch (const dtUtil::Exception& ex)
            {
               ex.LogException(dtUtil::Log::LOG_ERROR, mLogger);
            }
         }

      }

   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   bool GameActorProxy::IsRemote() const
   {
      return GetGameActor().IsRemote();
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   bool GameActorProxy::IsPublished() const
   {
      return GetGameActor().IsPublished();
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   GameActorProxy::Ownership& GameActorProxy::GetInitialOwnership()
   {
      return *mOwnership;
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::SetInitialOwnership(GameActorProxy::Ownership &newOwnership)
   {
      mOwnership = &newOwnership;
   }
   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   GameActorProxy::LocalActorUpdatePolicy& GameActorProxy::GetLocalActorUpdatePolicy()
   {
      return *mLocalActorUpdatePolicy;
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::SetLocalActorUpdatePolicy(GameActorProxy::LocalActorUpdatePolicy& newPolicy)
   {
      mLocalActorUpdatePolicy = &newPolicy;
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::AddInvokable(Invokable& newInvokable)
   {
      std::map<std::string,dtCore::RefPtr<Invokable> >::iterator itor =
         mInvokables.find(newInvokable.GetName());
      if (itor != mInvokables.end())
      {
         std::ostringstream ss;
         ss << "Could not add new invokable " << newInvokable.GetName() << " because "
         << "an invokable with that name already exists.";
         LOGN_ERROR("gameactor.cpp", ss.str());
      }
      else
      {
         mInvokables.insert(std::make_pair(newInvokable.GetName(), dtCore::RefPtr<Invokable>(&newInvokable)));
      }
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::RemoveInvokable(const std::string& name)
   {
      std::map<std::string,dtCore::RefPtr<Invokable> >::iterator itor =
         mInvokables.find(name);
      if (itor != mInvokables.end())
      {
         mInvokables.erase(itor);
      }
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::RemoveInvokable(Invokable* inv)
   {
      RemoveInvokable(inv->GetName());
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::BuildInvokables()
   {
      AddInvokable(*new Invokable(TICK_LOCAL_INVOKABLE,
               dtDAL::MakeFunctor(GetGameActor(), &GameActor::OnTickLocal)));

      AddInvokable(*new Invokable(TICK_REMOTE_INVOKABLE,
               dtDAL::MakeFunctor(GetGameActor(), &GameActor::OnTickRemote)));

      AddInvokable(*new Invokable(PROCESS_MSG_INVOKABLE,
               dtDAL::MakeFunctor(GetGameActor(), &GameActor::ProcessMessage)));
   }

   Invokable* GameActorProxy::GetInvokable(const std::string& name)
   {
      std::map<std::string,dtCore::RefPtr<Invokable> >::iterator itor =
         mInvokables.find(name);

      if (itor == mInvokables.end())
      {
         return NULL;
      }
      else
      {
         return itor->second.get();
      }
   }

   void GameActorProxy::GetInvokables(std::vector<Invokable*>& toFill)
   {
      toFill.clear();
      toFill.reserve(mInvokables.size());

      for (std::map<std::string,dtCore::RefPtr<Invokable> >::iterator i = mInvokables.begin();
         i != mInvokables.end(); ++i)
      {
         toFill.push_back(i->second.get());
      }
   }

   void GameActorProxy::GetInvokables(std::vector<const Invokable*>& toFill) const
   {
      toFill.clear();
      toFill.reserve(mInvokables.size());

      for (std::map<std::string,dtCore::RefPtr<Invokable> >::const_iterator i = mInvokables.begin();
         i != mInvokables.end(); ++i)
      {
         toFill.push_back(i->second.get());
      }
   }

   void GameActorProxy::GetMessageHandlers(const MessageType& type, std::vector<Invokable*>& toFill)
   {
      toFill.clear();
      toFill.reserve(mMessageHandlers.size());

      for (std::multimap<const MessageType*, dtCore::RefPtr<Invokable> >::iterator i = mMessageHandlers.find(&type);
         (i != mMessageHandlers.end()) && (*i->first == type); ++i)
      {
         toFill.push_back(i->second.get());
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::RegisterForMessages(const MessageType& type, const std::string& invokableName)
   {
      if (IsInGM())
      {
         GetGameManager()->RegisterForMessages(type,*this, invokableName);
      }
      else
      {
         std::ostringstream oss;
         oss << "Could not register the messagetype: " << type.GetName() << " with the invokable: " <<
         invokableName << " because the actor is not in the Game Manager yet.";
         mLogger.LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__, oss.str());
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::RegisterForMessagesAboutOtherActor(const MessageType& type,
            const dtCore::UniqueId& targetActorId, const std::string& invokableName)
   {
      if (IsInGM())
      {
         GetGameManager()->RegisterForMessagesAboutActor(type,targetActorId, *this, invokableName);
      }
      else
      {
         std::ostringstream oss;
         oss << "Could not register the messagetype: " << type.GetName() << " with the invokable: " <<
         invokableName << " because the actor is not in the Game Manager yet.";
         mLogger.LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__, oss.str());
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::RegisterForMessagesAboutSelf(const MessageType& type, const std::string& invokableName)
   {
      Invokable* invokable = GetInvokable(invokableName);
      if (invokable != NULL)
      {
         mMessageHandlers.insert(std::make_pair(&type, invokable));
      }
      else
      {
         std::ostringstream oss;
         oss << "Could not register invokable " << invokableName << " as a handler because "
         << "no invokable with that name exists.";

         mLogger.LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__, oss.str());
      }

   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::UnregisterForMessages(const MessageType& type, const std::string& invokableName)
   {
      if ( GetGameManager() != NULL)
      {
         GetGameManager()->UnregisterForMessages(type,*this, invokableName);
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::UnregisterForMessagesAboutOtherActor(const MessageType& type,
            const dtCore::UniqueId& targetActorId, const std::string& invokableName)
   {
      if (GetGameManager() != NULL)
      {
         GetGameManager()->UnregisterForMessagesAboutActor(type, targetActorId, *this, invokableName);
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::UnregisterForMessagesAboutSelf(const MessageType& type, const std::string& invokableName)
   {
      for (std::multimap<const MessageType*, dtCore::RefPtr<Invokable> >::iterator i = mMessageHandlers.find(&type);
      (i != mMessageHandlers.end()) && (*i->first == type); ++i)
      {
         if (i->second->GetName() == invokableName)
         {
            mMessageHandlers.erase(i);
            break;
         }
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::SetRemote(bool remote)
   {
      GameActor& ga = GetGameActor();
      ga.SetRemote(remote);
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::SetPublished(bool published)
   {
      GameActor& ga = GetGameActor();
      ga.SetPublished(published);
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::InvokeEnteredWorld()
   {
      /**
       * We will preform a check to make sure this actor actually is a GameActor
       */

      GameActor* ga = dynamic_cast<GameActor*>(GetActor());
      if (ga == NULL)
      {
         // throw exception
         throw dtUtil::Exception(ExceptionEnum::GENERAL_GAMEMANAGER_EXCEPTION,
                  "ERROR: Actor has the type of a GameActor, but casting it to a GameActorProxy failed.", __FILE__, __LINE__);
      }

      ga->OnEnteredWorld();
      ga->InitComponents();

      OnEnteredWorld();
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActorProxy::InvokeRemovedFromWorld()
   {
      /**
       * We will preform a check to make sure this actor actually is a GameActor
       */

      GameActor* ga = dynamic_cast<GameActor*>(GetActor());
      if (ga == NULL)
      {
         // throw exception
         throw dtUtil::Exception(ExceptionEnum::GENERAL_GAMEMANAGER_EXCEPTION,
                  "ERROR: Actor has the type of a GameActor, but casting it to a GameActorProxy failed.", __FILE__, __LINE__);
      }

      ga->RemoveAllComponents();

      OnRemovedFromWorld();
   }

   ///////////////////////////////////////////
   void GameActorProxy::AddPropertyToLocalUpdateAcceptFilter(const dtUtil::RefString& propName)
   {
      mLocalUpdatePropertyAcceptList.insert(propName);
   }

   ///////////////////////////////////////////
   void GameActorProxy::RemovePropertyFromLocalUpdateAcceptFilter(const dtUtil::RefString& propName)
   {
      std::set<dtUtil::RefString>::iterator i = mLocalUpdatePropertyAcceptList.find(propName);
      if (i != mLocalUpdatePropertyAcceptList.end())
      {
         mLocalUpdatePropertyAcceptList.erase(i);
      }
   }

   ///////////////////////////////////////////
   bool GameActorProxy::ShouldAcceptPropertyInLocalUpdate(const dtUtil::RefString& propName) const
   {
      return mLocalUpdatePropertyAcceptList.find(propName) != mLocalUpdatePropertyAcceptList.end();
   }

   ///////////////////////////////////////////
   // Actor code
   ///////////////////////////////////////////
   const std::string GameActor::NULL_PROXY_ERROR("The actor proxy for a game actor is NULL.  This usually happens if the actor is held in RefPtr, but not the proxy.");

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   GameActor::GameActor(GameActorProxy& proxy)
      : mProxy(&proxy)
      , mPublished(false)
      , mRemote(false)
      , mLogger(dtUtil::Log::GetInstance("gameactor.cpp"))
      , mPrototypeName("")
   {
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   GameActor::~GameActor()
   {
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   GameActorProxy& GameActor::GetGameActorProxy()
   {
      if (!mProxy.valid())
      {
         throw dtUtil::Exception(ExceptionEnum::INVALID_ACTOR_STATE,
                  NULL_PROXY_ERROR,
                  __FILE__, __LINE__);
      }
      return *mProxy;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   const GameActorProxy& GameActor::GetGameActorProxy() const
   {
      if (!mProxy.valid())
      {
         throw dtUtil::Exception(ExceptionEnum::INVALID_ACTOR_STATE,
                  NULL_PROXY_ERROR,
                  __FILE__, __LINE__);
      }
      return *mProxy;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::OnTickLocal(const TickMessage& tickMessage)
   {
      //Call to support older code.
      TickLocal(tickMessage);
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::OnTickRemote(const TickMessage& tickMessage)
   {
      //Call to support older code.
      TickRemote(tickMessage);
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::TickLocal(const Message& tickMessage)
   {
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::TickRemote(const Message& tickMessage)
   {
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::ProcessMessage(const Message& message)
   {
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::SetShaderGroup(const std::string &groupName)
   {
      // Setting the shader group, when it didn't change can cause a massive
      // hit on performance because it unassigns everything and will make a new 
      // instance of the shader and all its params. Could also cause anomalies with 
      // oscilating shader params.
      if (groupName != mShaderGroup)
      {
         mShaderGroup = groupName;
         OnShaderGroupChanged();
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::SetRemote(bool remote)
   {
      mRemote = remote;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::SetPublished(bool published)
   {
      mPublished = published;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////
   void GameActor::OnShaderGroupChanged()
   {
      // Unassign any old setting on this, if any - works regardless if there's a node or not
      dtCore::ShaderManager::GetInstance().UnassignShaderFromNode(*GetOSGNode());

      if (mShaderGroup.empty())
         return; // Do nothing, since we have nothing to load

      //First get the shader group assigned to this actor.
      const dtCore::ShaderGroup *shaderGroup =
         dtCore::ShaderManager::GetInstance().FindShaderGroupPrototype(mShaderGroup);

      if (shaderGroup == NULL)
      {
         mLogger.LogMessage(dtUtil::Log::LOG_INFO, __FUNCTION__, __LINE__,
                  "Could not find shader group [" + mShaderGroup + "] for actor [" + GetName());
         return;
      }

      const dtCore::ShaderProgram *defaultShader = shaderGroup->GetDefaultShader();

      try
      {
         if (defaultShader != NULL)
         {
            dtCore::ShaderManager::GetInstance().AssignShaderFromPrototype(*defaultShader, *GetOSGNode());
         }
         else
         {
            mLogger.LogMessage(dtUtil::Log::LOG_WARNING, __FUNCTION__, __LINE__,
                     "Could not find a default shader in shader group: " + mShaderGroup);
            return;
         }
      }
      catch (const dtUtil::Exception &e)
      {
         mLogger.LogMessage(dtUtil::Log::LOG_WARNING, __FUNCTION__, __LINE__,
                  "Caught Exception while assigning shader: " + e.ToString());
         return;
      }
   }

   //////////////////////////////////////////////////////////////////////////////
   void GameActor::SetPrototypeName(const std::string& prototypeName)
   {
      mPrototypeName = prototypeName;
   }

   //////////////////////////////////////////////////////////////////////////////
   const std::string& GameActor::GetPrototypeName() const
   {
      return mPrototypeName;
   }

}
