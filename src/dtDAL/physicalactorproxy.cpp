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
#include <prefix/dtdalprefix-src.h>
#include <dtDAL/physicalactorproxy.h>
#include <dtDAL/enginepropertytypes.h>
#include <dtDAL/exceptionenum.h>
#include <dtCore/scene.h>

namespace dtDAL
{
   const dtUtil::RefString PhysicalActorProxy::PROPERTY_ENABLE_DYNAMICS("Enable Dynamics");
   const dtUtil::RefString PhysicalActorProxy::PROPERTY_MASS("Mass");
   const dtUtil::RefString PhysicalActorProxy::PROPERTY_CENTER_OF_GRAVITY("Center of Gravity");

   ///////////////////////////////////////////////////////////////////////////////
   void PhysicalActorProxy::BuildPropertyMap()
   {
      static const dtUtil::RefString GROUPNAME("ODE Physics");

      TransformableActorProxy::BuildPropertyMap();

      dtCore::Physical *phys = static_cast<dtCore::Physical*>(GetActor());

      static const dtUtil::RefString ODE_PREFIX("ODE ");
      //PHYSICS PROPS...
      AddProperty(new BooleanActorProperty(PROPERTY_ENABLE_DYNAMICS, ODE_PREFIX + PROPERTY_ENABLE_DYNAMICS,
               BooleanActorProperty::SetFuncType(phys, &dtCore::Physical::EnableDynamics),
               BooleanActorProperty::GetFuncType(phys, &dtCore::Physical::DynamicsEnabled),
               "Enables physics calculations on this actor (using ODE).", GROUPNAME));

      AddProperty(new FloatActorProperty(PROPERTY_MASS, ODE_PREFIX + PROPERTY_MASS,
               FloatActorProperty::SetFuncType(this, &PhysicalActorProxy::SetMass),
               FloatActorProperty::GetFuncType(this, &PhysicalActorProxy::GetMass),
               "Sets the mass of this actor (using ODE).",GROUPNAME));

      AddProperty(new Vec3ActorProperty(PROPERTY_CENTER_OF_GRAVITY, ODE_PREFIX + PROPERTY_CENTER_OF_GRAVITY,
               Vec3ActorProperty::SetFuncType(this, &PhysicalActorProxy::SetCenterOfGravity),
               Vec3ActorProperty::GetFuncType(this, &PhysicalActorProxy::GetCenterOfGravity),
               "Sets the center of gravity for this actor (using ODE).", GROUPNAME));
   }

   ///////////////////////////////////////////////////////////////////////////////
   void PhysicalActorProxy::SetMass(float mass)
   {
      dtCore::Physical *phys = static_cast<dtCore::Physical*>(GetActor());

      phys->SetMass(mass);
   }

   ///////////////////////////////////////////////////////////////////////////////
   float PhysicalActorProxy::GetMass() const
   {
      const dtCore::Physical *phys = static_cast<const dtCore::Physical*>(GetActor());

      return phys->GetMass();
   }

   ///////////////////////////////////////////////////////////////////////////////
   void PhysicalActorProxy::SetCenterOfGravity(const osg::Vec3 &g)
   {
      dtCore::Physical *phys = static_cast<dtCore::Physical*>(GetActor());

      phys->SetCenterOfGravity(g);
   }

   ///////////////////////////////////////////////////////////////////////////////
   osg::Vec3 PhysicalActorProxy::GetCenterOfGravity() const
   {
      const dtCore::Physical *phys = static_cast<const dtCore::Physical*>(GetActor());

      osg::Vec3 r;
      phys->GetCenterOfGravity(r);
      return r;
   }
}
