/* -*-c++-*-
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
 * Matthew W. Campbell
 */

#ifndef DELTA_ACTOR_PLUGIN_REGISTRY
#define DELTA_ACTOR_PLUGIN_REGISTRY

#include <vector>
#include <dtUtil/objectfactory.h>
#include <dtDAL/actortype.h>
#include <dtDAL/actorproxy.h>
#include <dtDAL/export.h>

namespace dtDAL
{
   /**
    * The ActorPluginRegistry is the base class that developers extend to
    * build their own registries.  Its main purpose is to serve as an
    * object factory which knows how to build ActorProxies using
    * ActorTypes as the tool by which to do so.
    * @note
    *      Registry objects should only be used with the dtCore::RefPtr<>
    *      construct since they are reference counted objects.
    * @see ActorType
    * @see ActorProxy
    */
   class DT_DAL_EXPORT ActorPluginRegistry
   {
      public:

         /**
          * Constructs the registry.  Sets the name and description for
          * this registry.
          */
         ActorPluginRegistry(const std::string& name, const std::string& desc = "")
            : mName(name)
            , mDescription(desc)
         {
            mActorFactory = new dtUtil::ObjectFactory<dtCore::RefPtr<const ActorType>, ActorProxy, ActorType::RefPtrComp>;
         }

         /**
          * Empty destructor. This class is not reference counted since we need
          * to manually free pointers to the registry objects from their
          * corresponding dynamic library, therefore, we need access to the
          * object's destructor.
          */
         virtual ~ActorPluginRegistry() { }

         /**
          * Registers the actor types that this registry knows how to create.
          * This method is the first method to get called by the LibraryManager
          * after it loads a dynamic library and gets a pointer to the
          * registry object it contains.
          */
         virtual void RegisterActorTypes() = 0;

         /**
          * Sets the name of this registry.
          * @param name Name to assign to the registry.
          */
         void SetName(const std::string& name) { mName = name; }

         /**
          * Gets the name currently assigned to this registry.
          */
         const std::string& GetName() const { return mName; }

         /**
          * Sets the description for this registry.
          * @param desc Couple sentence description for this actor registry.
          */
         void SetDescription(const std::string& desc) { mDescription = desc; }

         /**
          * Gets the description of this registry.
          */
         const std::string& GetDescription() const { return mDescription; }

         /**
          * Gets a list of actor types that this registry supports.
          */
         void GetSupportedActorTypes(std::vector<dtCore::RefPtr<const ActorType> >& actors);

         /** 
           * Container of <old, new> ActorType names.  First entry is the full name of the
           * old ActorType.  Second entry is the full name of the new ActorType to
           * use instead.
           */
         typedef std::vector<std::pair<std::string, std::string> > ActorTypeReplacements;

         /** 
          * Get the ActorTypeReplacements for this ActorPluginRegistry.  This list
          * is used to provide some backwards compatibility with applications or maps
          * referring to older, deprecated ActorTypes.  Override in derived classes
          * if previous ActorTypes have been modified and backwards compatibility is 
          * desired.
          * @param The container to fill out with ActorType replacements
          */
         virtual void GetReplacementActorTypes(ActorTypeReplacements &replacements) const;

         /**
          * Checks to see if this registry supports the given actor type.
          * @param type The type to check support for.
          * @return True if supported, false otherwise.
          */
         bool IsActorTypeSupported(dtCore::RefPtr<const ActorType> type);

         /**
          * Creates a new actor proxy based on the ActorType given.
          * @param type Type of actor to create.
          * @return Returns a smart pointer to the newly created
          * proxy object.
          * @throws ExceptionEnum::ObjectFactoryUnknownType
          */
         dtCore::RefPtr<ActorProxy> CreateActorProxy(const ActorType& type);

      protected:
         std::string mName;
         std::string mDescription;

         /**
          * Factory object which stores the actor types and knows how to
          * create proxy objects for each type.
          * @see ObjectFactory
          */
         dtCore::RefPtr<dtUtil::ObjectFactory<dtCore::RefPtr<const ActorType>,
            ActorProxy, ActorType::RefPtrComp> > mActorFactory;
   };
} // namespace dtDAL

#endif // DELTA_ACTOR_PLUGIN_REGISTRY
