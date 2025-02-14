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
 * William E. Johnson II
 */

#ifndef DELTA_GAMEACTOR
#define DELTA_GAMEACTOR

#include <dtCore/observerptr.h>
#include <dtCore/physical.h>
#include <dtDAL/physicalactorproxy.h>
#include <dtGame/export.h>
#include <dtGame/actorcomponentbase.h>

namespace dtUtil
{
   class Log;
}

namespace dtGame
{
   class Message;
   class ActorUpdateMessage;
   class GameManager;
   class Invokable;
   class MessageType;
   class TickMessage;

   class GameActorProxy;
   /**
    * @class GameActor
    * This class will be the main base class for actors being used by the
    * Game Manager
    * @see dtGame::GameManager
    */
   class DT_GAME_EXPORT GameActor 
      : public dtCore::Physical
      , public dtGame::ActorComponentBase
   {
   public:
      /// Constructor
      GameActor(GameActorProxy& proxy);

      /**
       * @return the GameActorProxy for this game actor.
       */
      GameActorProxy& GetGameActorProxy();

      /**
       * @return the GameActorProxy for this game actor.
       */
      const GameActorProxy& GetGameActorProxy() const;

      /**
       * Returns if the actor is remote
       * @return True is the actor is remote, false if not
       */
      inline bool IsRemote() const { return mRemote; }

      /**
       * Returns is the actor is published
       * @return True is the actor is published, false if not
       */
      inline bool IsPublished() const { return mPublished; }

      /**
       * Method for handling local ticks.  This will called by the "Tick Local" invokable.
       * This is designed to be registered to receive TICK_LOCAL messages, but that registration is not done
       * be default
       * @see GameManager#RegisterForMessages
       * @param tickMessage the actual message
       */
      virtual void OnTickLocal(const TickMessage& tickMessage);

      ///Deprecated, override the one that takes a TickMessage;
      virtual void TickLocal(const Message& tickMessage);

      /**
       * Method for handling remote ticks.  This will called by the "Tick Remote" invokable
       * This is designed to be registered to receive TICK_REMOTE messages, but that registration is not done
       * be default
       * @see GameManager#RegisterForMessages
       * @param tickMessage the actual message
       */
      virtual void OnTickRemote(const TickMessage& tickMessage);

      ///Deprecated, override the one that takes a TickMessage;
      virtual void TickRemote(const Message& tickMessage);

      /**
       * Default invokable for handling messages. This is only called if you register
       * a message using the default PROCESS_MSG_INVOKABLE name. To use this, override it
       * and handle any messages that you want. Then, in the OnEnteredWorld() method on your
       * proxy, add a line, something either of these:
       *    RegisterForMessages(dtGame::MessageType::INFO_GAME_EVENT);
       *    RegisterForMessages(dtGame::MessageType::INFO_GAME_EVENT, PROCESS_MSG_INVOKABLE);
       * @param message the actual message
       */
      virtual void ProcessMessage(const Message& message);

      /**
       * Sets the shader group on the game actor.  This implementation uses
       * the default shader in the group to shade the game actor.
       * @param groupName The name of the shader group.
       */
      virtual void SetShaderGroup(const std::string& groupName);

      /**
       * Gets the current shader group assigned to this game actor.
       * @return The name of the group.
       */
      std::string GetShaderGroup() const { return mShaderGroup; }

      virtual void OnShaderGroupChanged();

      /**
      * See GetPrototypeName().
      * @param prototypeName The prototype that was used to create this actor. Set by the GM.
      */
      void SetPrototypeName(const std::string& prototypeName);

      /**
      * This value is used for updating/creating remote actors that need to be recreated from prototype
      * This value is set automatically by the GM when an actor is created from prototype.
      * If the prototype is non-null, then when the actor is created by the message
      * processor, it will attempt to look up the prototype first. Extremely useful for networking.
      * @return The prototype that was used to create this actor. Set by the GM.
      */
      const std::string& GetPrototypeName() const;

   protected:

      /// Destructor
      virtual ~GameActor();

      /**
       * Called when an actor is first placed in the "world"
       */
      virtual void OnEnteredWorld() { }

   private:

      static const std::string NULL_PROXY_ERROR;
      /**
       * Sets is an actor is remote
       * @param remote Should be true is the actor is remote, false if not
       */
      void SetRemote(bool remote);

      /**
       * Sets is an actor is published
       * @param remote Should be true is the actor is published, false if not
       */
      void SetPublished(bool published);

      friend class GameActorProxy;
      dtCore::ObserverPtr<GameActorProxy> mProxy;
      bool mPublished;
      bool mRemote;
      std::string mShaderGroup;
      dtUtil::Log& mLogger;
      std::string mPrototypeName;
   };

   /**
    * class GameActorProxy
    * This will be the base class for all of the actor proxies utilized by the
    * Game Manager
    * @see dtGame::GameManager
    */
   class DT_GAME_EXPORT GameActorProxy : public dtDAL::PhysicalActorProxy
   {
   public:
      typedef dtDAL::PhysicalActorProxy BaseClass;

      /**
       * Name is intended to become a property, so this constant exists for that and for the LocalActorUpdatePolicy
       * filter property list.
       */
      static const dtUtil::RefString PROPERTY_NAME;

      /// Use this when you register a message type and want to receive it in ProcessMessage()
      static const std::string PROCESS_MSG_INVOKABLE;
      /// invokables for tick local and remote - will call TickLocal() and TickRemote();
      static const std::string TICK_LOCAL_INVOKABLE;
      static const std::string TICK_REMOTE_INVOKABLE;

      /// Internal class to represent the ownership of an actor proxy
      class DT_GAME_EXPORT Ownership : public dtUtil::Enumeration
      {
         DECLARE_ENUM(Ownership);
      public:
         static Ownership SERVER_PUBLISHED;
         static Ownership SERVER_LOCAL;
         static Ownership CLIENT_LOCAL;
         static Ownership CLIENT_AND_SERVER_LOCAL;
         static Ownership PROTOTYPE;
         Ownership(const std::string& name);
      };

      /// Internal class to represent the ownership of an actor proxy
      class DT_GAME_EXPORT LocalActorUpdatePolicy : public dtUtil::Enumeration
      {
         DECLARE_ENUM(LocalActorUpdatePolicy);
      public:
         static LocalActorUpdatePolicy IGNORE_ALL;
         static LocalActorUpdatePolicy ACCEPT_ALL;
         static LocalActorUpdatePolicy ACCEPT_WITH_PROPERTY_FILTER;
         LocalActorUpdatePolicy(const std::string& name);
      };

      /// Constructor
      GameActorProxy();

      /// Overridden to call BuildInvokables
      virtual void Init(const dtDAL::ActorType& actorType);

      /**
       * This is a shortcut to avoid having to dynamic cast to a GameActorProxy.
       * @return true always
       */
      virtual bool IsGameActorProxy() const { return true; }

      /**
       * @return a const pointer to the parent game manager that owns this actor.
       */
      const GameManager* GetGameManager() const { return mParent; }

      /**
       * @return a pointer to the parent game manager that owns this actor.
       */
      GameManager* GetGameManager() { return mParent; }

      /**
       * Retrieves the game actor that this proxy represents.
       * @return The Game Actor
       */
      GameActor& GetGameActor();

      /**
       * Retrieves the game actor that this proxy represents.
       * @return The Game Actor
       */
      const GameActor& GetGameActor() const;

      /**
       * Creates the properties associated with this proxy
       */
      virtual void BuildPropertyMap();

      /**
       * Creates the invokables associated with this proxy.
       */
      virtual void BuildInvokables();

      /**
       * Adds a new invokable to the this proxy's list of invokables.
       * @param the new invokable to add.
       * @note
       *      Invokables must have unique names, therefore, if one
       *      is added that who's name collides with another, the invokable
       *      is not added and an error message is logged.
       */
      void AddInvokable(Invokable& newInvokable);

      /**
       * Remove this invokable from the proxy's list
       */
      void RemoveInvokable(const std::string& name);

      /**
       * Remove this invokable from the proxy's list
       */
      void RemoveInvokable(Invokable*);

      /**
       * @return the invokable with the given name or NULL if it doesn't have one with that name.
       */
      Invokable* GetInvokable(const std::string& name);

      /**
       * Gets a list of the invokables currently registered for this
       * Game actor proxy.
       */
      void GetInvokables(std::vector<Invokable*>& toFill);

      /**
       * Gets a const list of the invokables currently registered for this
       * Game actor proxy.
       */
      void GetInvokables(std::vector<const Invokable*>& toFill) const;

      /**
       * Creates an ActorUpdateMessage, populates it with ALL properties on the actor
       * and calls SendMessage() on the Game Manager.
       * Note - This will do nothing if the actor is Remote.
       */
      virtual void NotifyFullActorUpdate();

      /**
       * Creates an ActorUpdateMessage, populates it with specific properties on the actor
       * and calls SendMessage() on the Game Manager.
       * Note - This will do nothing if the actor is Remote.
       * @param propNames  the properties to include in the update message.
       */
      virtual void NotifyPartialActorUpdate(const std::vector<dtUtil::RefString>& propNames);
      /**
       * Deprecated - Use NotifyPartialActorUpdate() with dtUtil::RefString instead.
       * @see NotifyPartialActorUpdate
       */
      DEPRECATE_FUNC virtual void NotifyPartialActorUpdate(const std::vector<std::string>& propNames);

      /**
       * This is like NotifyFullActorUpdate() except that your subclass might only want to 
       * send some properties. If you use this, you MUST override GetPartialUpdateProperties()
       * to set which properties will be sent.
       * Note - This will do nothing if the actor is Remote.
       */
      virtual void NotifyPartialActorUpdate();

      /**
       * Override this and add whatever properties you want to go out when you call 
       * NotifyPartialActorUpdate(). Note - you should not use NotifyPartialActorUpdate() 
       * without overriding this - the default implementation logs a warning.
       * Note - This will do nothing if the actor is Remote.
       */
      virtual void GetPartialUpdateProperties(std::vector<dtUtil::RefString>& propNamesToFill);

      /**
       * Deprecated - Use NotifyFullActorUpdate() for a full or NotifyPartialActorUpdate() 
       * for a partial. 
       * @see NotifyFullActorUpdate
       * @see NotifyPartialActorUpdate
       */
      DEPRECATE_FUNC virtual void NotifyActorUpdate();

      /**
       * Populates an update message from the actor proxy.  When overwriting this method, be sure to call or
       * duplicate the functionality provided in GameActor::PopulateActorUpdate().
       * @param update The message to populate.
       * @param propNames  the properties to include in the message.
       */
      virtual void PopulateActorUpdate(ActorUpdateMessage& update, const std::vector<dtUtil::RefString>& propNames);
      /**
       * Deprecated - Use PopulateActorUpdate() with dtUtil::RefString instead.
       * @see PopulateActorUpdate
       */
      DEPRECATE_FUNC virtual void PopulateActorUpdate(ActorUpdateMessage& update, const std::vector<std::string>& propNames);

      /**
       * Populates an update message from the actor proxy.  It will add all property values to the message.
       * When overwriting this method, be sure to call or
       * duplicate the functionality provided in GameActor::PopulateActorUpdate().
       * @param update The message to populate.
       */
      virtual void PopulateActorUpdate(ActorUpdateMessage& update);

      /**
       * Takes and actor update message and applies the parameter values to change the
       * the property values of the this actor proxy.  This is virtual so it can be extended
       * or replaced to accommodate special behavior or special subclasses of actor update.
       *
       * @param msg the message to apply.
       * @param checkLocalUpdatePolicy set to true if the policy filtering should be consulted, if false the update will
       *                               just be set with no checking.
       * @see #SetLocalActorUpdatePolicy
       */
      virtual void ApplyActorUpdate(const ActorUpdateMessage& msg, bool checkLocalUpdatePolicy = false);

      /**
       * Get all of the invokables registered for a given message type.
       * @param type the message type to query for.
       * @param toFill a vector to fill with the invokables.
       */
      void GetMessageHandlers(const MessageType& type, std::vector<Invokable*>& toFill);

      /**
       * Returns if this proxy is remote, calls the private actor definition
       * @see dtGame::GameActor
       * @return True if the actor is remote, false if not
       */
      bool IsRemote() const;

      /**
       * Returns if this proxy is published, calls the private actor definition
       * @see dtGame::GameActor
       * @return True if the actor is published, false if not
       */
      bool IsPublished() const;

      /**
       * Returns the ownership of the actor proxy
       * @return The ownership, corresponding with the ownership class
       * @see dtGame::GameActorProxy::Ownership
       */
      Ownership& GetInitialOwnership();

      /**
       * Sets the ownership of the actor proxy
       * @return The ownership, corresponding with the ownership class
       * @see dtGame::GameActorProxy::Ownership
       */
      void SetInitialOwnership(Ownership& newOwnership);

      /**
       * Returns the local actor update policy.  This defines what the actor should do if an INFO_ACTOR_UPDATED
       * message comes in remotely about this actor when it is locally created and simulated.  Essentially that would
       * mean that another system would be trying to change property values on this actor.
       * @return The current policy
       * @see dtGame::GameActorProxy::LocalActorUpdatePolicy
       */
      LocalActorUpdatePolicy& GetLocalActorUpdatePolicy();

      /**
       * Sets a new local actor update policy.  This defines what the actor should do if an INFO_ACTOR_UPDATED
       * message comes in remotely about this actor when it is locally created and simulated.  Essentially that would
       * mean that another system would be trying to change property values on this actor.
       * @see dtGame::GameActorProxy::LocalActorUpdatePolicy
       */
      void SetLocalActorUpdatePolicy(LocalActorUpdatePolicy& newPolicy);

      /**
       * Registers to receive a specific type of message from the GM.  You will receive
       * all instances of this message, regardless of whether you are the about actor or not.
       * By default, it will use the ProcessMessage() invokable on GameActor (PROCESS_MSG_INVOKABLE)
       * @see dtGame::GameActorProxy::RegisterForMessagesAboutOtherActor
       * @see dtGame::GameActorProxy::RegisterForMessagesAboutSelf
       * @see dtGame::GameActor::ProcessMessage
       * @see dtGame::GameActorProxy::PROCESS_MSG_INVOKABLE
       */
      void RegisterForMessages(const MessageType& type,
               const std::string& invokableName = PROCESS_MSG_INVOKABLE);

      /**
       * Registers to receive a specific type of message from the GM.  You will ONLY receive
       * messages about this other actor.  Use this when you want to track interactions with
       * other actors - such as a player listening for when a vehicle he is in gets damaged.
       * By default, it will use the ProcessMessage() invokable on GameActor (PROCESS_MSG_INVOKABLE)
       * @see dtGame::GameActorProxy::RegisterForMessages
       * @see dtGame::GameActorProxy::RegisterForMessagesAboutSelf
       * @see dtGame::GameActor::ProcessMessage
       * @see dtGame::GameActorProxy::PROCESS_MSG_INVOKABLE
       */
      void RegisterForMessagesAboutOtherActor(const MessageType& type,
               const dtCore::UniqueId& targetActorId,
               const std::string& invokableName = PROCESS_MSG_INVOKABLE);

      /**
       * Registers to receive a specific type of message from the GM.  You will ONLY receive
       * messages about yourself. This is the normal use case for registering for messages.
       * Typically, you only want to know when YOU have fired a weapon, or when YOU have been
       * shot. Not when another player is shot.
       * By default, it will use the ProcessMessage() invokable on GameActor (PROCESS_MSG_INVOKABLE)
       * @see dtGame::GameActorProxy::RegisterForMessages
       * @see dtGame::GameActorProxy::RegisterForMessagesAboutOtherActor
       * @see dtGame::GameActor::ProcessMessage
       * @see dtGame::GameActorProxy::PROCESS_MSG_INVOKABLE
       */
      void RegisterForMessagesAboutSelf(const MessageType& type,
               const std::string& invokableName = PROCESS_MSG_INVOKABLE);

      /**
       * Unregisters the invokable for a specific type of message from the GM.  This is the
       * reverse of RegisterForMessages for global messages. By default, it will unregister the
       * ProcessMessage() invokable on GameActor (PROCESS_MSG_INVOKABLE)
       * @see dtGame::GameActorProxy::RegisterForMessages
       * @see dtGame::GameActorProxy::PROCESS_MSG_INVOKABLE
       */
      void UnregisterForMessages(const MessageType& type,
               const std::string& invokableName = PROCESS_MSG_INVOKABLE);

      /**
       * Unregisters the invokable for a specific type of message for a specific Actor.  This
       * is the reverse of RegisterForMessagesAboutOtherActor. By default, it will unregister the
       * ProcessMessage() invokable on GameActor (PROCESS_MSG_INVOKABLE)
       * @see dtGame::GameActorProxy::RegisterForMessagesAboutOtherActor
       * @see dtGame::GameActorProxy::PROCESS_MSG_INVOKABLE
       */
      void UnregisterForMessagesAboutOtherActor(const MessageType& type,
               const dtCore::UniqueId& targetActorId,
               const std::string& invokableName = PROCESS_MSG_INVOKABLE);

      /**
       * Unregisters the invokable for a specific type of message from the GM.  This is the
       * reverse of RegisterForMessagesAboutSelf. By default, it will unregister the
       * ProcessMessage() invokable on GameActor (PROCESS_MSG_INVOKABLE)
       * @see dtGame::GameActorProxy::RegisterForMessagesAboutSelf
       * @see dtGame::GameActorProxy::PROCESS_MSG_INVOKABLE
       */
      void UnregisterForMessagesAboutSelf(const MessageType& type,
               const std::string& invokableName = PROCESS_MSG_INVOKABLE);

      /**
       * Returns true if the actor has been added to the
       * Game Manager yet or if not.
       * then is set to true upon being added to the GM
       */
      bool IsInGM() const;

      /**
       * Moved to public, since map change state data needs to call this now as well.
       * for actors within the map.
       */
      void SetGameManager(GameManager* gm);

      /**
       * This essetially checks to see if a property name is in the accept filter
       * @see dtGame::GameActorProxy::LocalActorUpdatePolicy::ACCEPT_WITH_PROPERTY_FILTER
       */
      bool ShouldAcceptPropertyInLocalUpdate(const dtUtil::RefString& propName) const;

   protected:
      /// Destructor
      virtual ~GameActorProxy();

      /**
       * Instantiates the actor that the proxy abstracts.
       * This must ALSO call SetGameActorProxy
       */
      virtual void CreateActor() = 0;

      /**
       * Called when an actor is first placed in the "world"
       */
      virtual void OnEnteredWorld() { }

      /**
       * Called when the GM deletes the actor in a NORMAL way, such as DeleteActor().
       * This may not get called from every possible path that an actor can be removed
       * or deleted. For instance, calling DeleteAllActors(true) on the GM would bypass
       * the 'normal' actor delete process. Note, this is sent at the end of the tick and
       * at the end of a 'normal' map change. If you want to find out immediately that you
       * are being deleted, register for the INFO_ACTOR_DELETED with RegisterForMessagesAboutSelf().
       */
      virtual void OnRemovedFromWorld() { }

      /**
       * Adds a property to the accept list for local updates.
       * If you want to accept a the actor name from a message, you must add PROPERTY_NAME to this list.
       * @see dtGame::GameActorProxy::PROPERTY_NAME
       * @see dtGame::GameActorProxy::LocalActorUpdatePolicy::ACCEPT_WITH_PROPERTY_FILTER
       */
      void AddPropertyToLocalUpdateAcceptFilter(const dtUtil::RefString& propName);

      /**
       * Removes a property to the accept list for local updates.
       * @see dtGame::GameActorProxy::LocalActorUpdatePolicy::ACCEPT_WITH_PROPERTY_FILTER
       */
      void RemovePropertyFromLocalUpdateAcceptFilter(const dtUtil::RefString& propName);

   private:

      /**
       * Populates an update message from the actor proxy.
       * @param update The message to populate.
       * @param propNames the list of properties to include in the message.
       * @param limitProperties true if the propNames list should be respected or false if all properties should added.
       */
      void PopulateActorUpdate(ActorUpdateMessage& update, const std::vector<dtUtil::RefString>& propNames, bool limitProperties);

      /**
       * Sets if the actor is remote by invoking the actor implementation
       * @param True if the actor should be remote, false if not
       */
      void SetRemote(bool remote);

      /**
       * Sets if the actor is published by invoking the actor implementation
       * @param True if the actor should be remote, false if not
       */
      void SetPublished(bool published);

      /**
       * Invokes the OnEnteredWorld function of the actor, and then the proxy
       */
      void InvokeEnteredWorld();

      /**
       * Invokes the OnRemovedFromWorld function of the proxy
       */
      void InvokeRemovedFromWorld();

      /// This was added so the GameManager can be set on creation.
      void SetIsInGM(bool value);

      friend class GameManager;
      GameManager* mParent;
      Ownership* mOwnership;
      LocalActorUpdatePolicy* mLocalActorUpdatePolicy;
      dtUtil::Log& mLogger;
      std::map<std::string, dtCore::RefPtr<Invokable> > mInvokables;
      std::multimap<const MessageType*, dtCore::RefPtr<Invokable> > mMessageHandlers;
      std::set<dtUtil::RefString> mLocalUpdatePropertyAcceptList;
      bool mIsInGM;
   };
}

#endif
