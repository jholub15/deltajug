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

#include <prefix/dtgameprefix-src.h>
#include <dtGame/basemessages.h>
#include <dtDAL/gameeventmanager.h>
#include <dtUtil/stringutils.h>
#include <algorithm>
#include <sstream>

namespace dtGame
{
   const dtUtil::RefString TickMessage::PARAM_DELTA_SIM_TIME("DeltaSimTime");
   const dtUtil::RefString TickMessage::PARAM_DELTA_REAL_TIME("DeltaRealTime");
   const dtUtil::RefString TickMessage::PARAM_SIM_TIME_SCALE("SimTimeScale");
   const dtUtil::RefString TickMessage::PARAM_SIMULATION_TIME("SimulationTime");

   //////////////////////////////////////////////////////////////////////////////
   /// Constructor
   TickMessage::TickMessage() : Message()
   {
      mDeltaSimTime = new FloatMessageParameter(PARAM_DELTA_SIM_TIME); 
      mDeltaRealTime = new FloatMessageParameter(PARAM_DELTA_REAL_TIME);
      mSimTimeScale = new FloatMessageParameter(PARAM_SIM_TIME_SCALE);
      mSimulationTime = new DoubleMessageParameter(PARAM_SIMULATION_TIME);   
      AddParameter(mDeltaSimTime.get());
      AddParameter(mDeltaRealTime.get());
      AddParameter(mSimTimeScale.get());
      AddParameter(mSimulationTime.get());
   }

   //////////////////////////////////////////////////////////////////////////////
   float TickMessage::GetDeltaSimTime() const
   {
      return mDeltaSimTime->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   float TickMessage::GetDeltaRealTime() const
   {
      return mDeltaRealTime->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   float TickMessage::GetSimTimeScale() const
   {
      return mSimTimeScale->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   double TickMessage::GetSimulationTime() const
   {
      return mSimulationTime->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void TickMessage::SetSimulationTime(double newSimulationTime)
   {
      mSimulationTime->SetValue(newSimulationTime);
   }

   //////////////////////////////////////////////////////////////////////////////
   void TickMessage::SetDeltaSimTime(float newTime)
   {
      mDeltaSimTime->SetValue(newTime);
   }

   //////////////////////////////////////////////////////////////////////////////
   void TickMessage::SetDeltaRealTime(float newTime)
   {
      mDeltaRealTime->SetValue(newTime);
   }

   //////////////////////////////////////////////////////////////////////////////
   void TickMessage::SetSimTimeScale(float newScale)
   {
      mSimTimeScale->SetValue(newScale);
   }

   //////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////

   const std::string& TimerElapsedMessage::GetTimerName() const
   {
      const StringMessageParameter *mp = static_cast<const StringMessageParameter*> (GetParameter("TimerName"));
      return mp->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   float TimerElapsedMessage::GetLateTime() const
   {
      const FloatMessageParameter *mp = static_cast<const FloatMessageParameter*> (GetParameter("LateTime"));
      return mp->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void TimerElapsedMessage::SetTimerName(const std::string &name)
   {
      StringMessageParameter *mp = static_cast<StringMessageParameter*> (GetParameter("TimerName"));
      mp->SetValue(name);
   }

   //////////////////////////////////////////////////////////////////////////////
   void TimerElapsedMessage::SetLateTime(float newTime)
   {
      FloatMessageParameter *mp = static_cast<FloatMessageParameter*> (GetParameter("LateTime"));
      mp->SetValue(newTime);
   }

   //////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////

   float TimeChangeMessage::GetTimeScale() const
   {
      return static_cast<const FloatMessageParameter*>(GetParameter("TimeScale"))->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void TimeChangeMessage::SetTimeScale(float newTimeScale)
   {
      FloatMessageParameter *mp = static_cast<FloatMessageParameter*>(GetParameter("TimeScale"));
      mp->SetValue(newTimeScale);
   }

   //////////////////////////////////////////////////////////////////////////////
   double TimeChangeMessage::GetSimulationTime() const
   {
      return static_cast<const DoubleMessageParameter*>(GetParameter("SimulationTime"))->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void TimeChangeMessage::SetSimulationTime(double newSimulationTime)
   {
      DoubleMessageParameter *mp = static_cast<DoubleMessageParameter*>(GetParameter("SimulationTime"));
      mp->SetValue(newSimulationTime);
   }

   //////////////////////////////////////////////////////////////////////////////
   double TimeChangeMessage::GetSimulationClockTime() const
   {
      return static_cast<const DoubleMessageParameter*>(GetParameter("SimulationClockTime"))->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void TimeChangeMessage::SetSimulationClockTime(double newSimClockTime)
   {
      DoubleMessageParameter *mp = static_cast<DoubleMessageParameter*>(GetParameter("SimulationClockTime"));
      mp->SetValue(newSimClockTime);
   }

   //////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////

   const dtUtil::RefString MapMessage::PARAM_MAP_NAMES("MapNames");

   class GetStringParameterFunc
   {
      public:
         GetStringParameterFunc(std::vector<std::string>& toFill):
            mVec(toFill)
         {
         }
         
         void operator() (const dtCore::RefPtr<MessageParameter>& parameter)
         {
            mVec.push_back(parameter->ToString());
         }
      
         std::vector<std::string>& mVec;
   };
   
   class InsertStringParameterFunc
   {
      public:
         InsertStringParameterFunc(GroupMessageParameter& param):
            mCount(0),
            mGroupParam(param)
         {
         }

         void operator() (const std::string& str)
         {
            std::string name;
            dtUtil::MakeIndexString(mCount, name);
            mGroupParam.AddParameter(*new StringMessageParameter(name, str));
            ++mCount;
         }

         int mCount;
         GroupMessageParameter& mGroupParam;
   };
   //////////////////////////////////////////////////////////////////////////////
   /// Constructor
   MapMessage::MapMessage() : Message()
   {
      mMapNames = new GroupMessageParameter(PARAM_MAP_NAMES); 
      AddParameter(mMapNames.get());
   }

   //////////////////////////////////////////////////////////////////////////////
   void MapMessage::GetMapNames(std::vector<std::string>& toFill) const
   {  
      toFill.clear();
      toFill.reserve(mMapNames->GetParameterCount());
      GetStringParameterFunc parameterFunc(toFill);
      mMapNames->ForEachParameter(parameterFunc);
   }

   //////////////////////////////////////////////////////////////////////////////
   void MapMessage::SetMapNames(const std::vector<std::string>& nameVec)
   {
      InsertStringParameterFunc parameterFunc(*mMapNames);
      std::for_each(nameVec.begin(), nameVec.end(), parameterFunc);
   }

   //////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////

   void GameEventMessage::SetGameEvent(const dtDAL::GameEvent& event)
   {
      GameEventMessageParameter* mp = static_cast<GameEventMessageParameter*>(GetParameter("GameEvent"));
      mp->SetValue(event.GetUniqueId());
   }

   //////////////////////////////////////////////////////////////////////////////
   const dtDAL::GameEvent* GameEventMessage::GetGameEvent() const
   {
      const GameEventMessageParameter* mp = static_cast<const GameEventMessageParameter*>(GetParameter("GameEvent"));
      const dtCore::UniqueId id = mp->GetValue();

      //Need to look up in the event manager for the specified event.
      dtDAL::GameEvent *event = dtDAL::GameEventManager::GetInstance().FindEvent(id);
      if (event == NULL)
         LOG_WARNING("Game event message parameter had an invalid game event id.");
      
      return event;
   }

   //////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////

   const std::string& NetServerRejectMessage::GetRejectionMessage() const
   {
      const StringMessageParameter *mp = static_cast<const StringMessageParameter*> (GetParameter("RejectionMessage"));
      return mp->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void NetServerRejectMessage::SetRejectionMessage(const std::string &msg)
   {
      StringMessageParameter *mp = static_cast<StringMessageParameter*> (GetParameter("RejectionMessage"));
      mp->SetValue(msg);
   }

   //////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////

   const std::string& RestartMessage::GetReason() const
   {
      const StringMessageParameter *mp = static_cast<const StringMessageParameter*> (GetParameter("Reason"));
      return mp->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void RestartMessage::SetReason(const std::string &reason)
   {
      StringMessageParameter *mp = static_cast<StringMessageParameter*> (GetParameter("Reason"));
      mp->SetValue(reason);
   }

   //////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////

   const std::string& ServerMessageRejected::GetCause() const
   {
      const StringMessageParameter *mp = static_cast<const StringMessageParameter*> (GetParameter("Cause"));
      return mp->GetValue();
   }

   //////////////////////////////////////////////////////////////////////////////
   void ServerMessageRejected::SetCause(const std::string &cause)
   {
      StringMessageParameter *mp = static_cast<StringMessageParameter*> (GetParameter("Cause"));
      mp->SetValue(cause);
   }
}
