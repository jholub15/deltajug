/* -*-c++-*-
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2007, Alion Science and Technology
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
 * Bradley Anderegg 07/17/2007
 */
#ifndef DELTA_CAL3D_MODEL_DATA
#define DELTA_CAL3D_MODEL_DATA

#include <dtAnim/export.h>

#include <dtCore/refptr.h>

#include <osg/Referenced>

#include <vector>

class CalCoreModel;

namespace dtAnim
{
   class AnimationWrapper;
   class Animatable;

   /**
    * A simple data class that stores the configuration options for level of detail.
    */
   class DT_ANIM_EXPORT LODOptions
   {
      public:
         LODOptions();

         ///@return the distance at which to start decreasing the level of detail.
         double GetStartDistance() const { return mStartDistance; }
         void SetStartDistance(double newDistance);

         ///@return the distance at which the level of detail should be the minimum.
         double GetEndDistance() const { return mEndDistance; }
         void SetEndDistance(double newDistance);

         ///@return the maximum distance that the model will be drawn at all.
         double GetMaxVisibleDistance() const { return mMaxVisibleDistance; }
         void SetMaxVisibleDistance(double newDistance);
         
      private:
         double mStartDistance, mEndDistance, mMaxVisibleDistance;
   };

   class DT_ANIM_EXPORT Cal3DModelData : public osg::Referenced
   {
      public:
         //we will hold all the animation wrappers for each CalCoreModel
         typedef std::vector<dtCore::RefPtr<AnimationWrapper> > AnimationWrapperArray;
         //we will hold a vector of animatables for each CalCoreModel
         typedef std::vector<dtCore::RefPtr<Animatable> > AnimatableArray;

      public:
         Cal3DModelData(CalCoreModel* coreModel, const std::string& filename);

         void Add(AnimationWrapper*);
         void Add(Animatable*);

         void Remove(AnimationWrapper*);
         void Remove(Animatable*);

         const std::string& GetFilename() const;

         CalCoreModel* GetCoreModel();
         const CalCoreModel* GetCoreModel() const;

         AnimationWrapperArray& GetAnimationWrappers();
         const AnimationWrapperArray& GetAnimationWrappers() const;

         AnimatableArray& GetAnimatables();
         const AnimatableArray& GetAnimatables() const;

         /**
          * @return the id of the of the Vertex Buffer Object being used with this 
          *    character core model, or 0 for none.
          */
         unsigned GetVertexVBO() const;
         
         /// Sets the id of the of the Vertex Buffer Object being used with this character core model
         void SetVertexVBO(unsigned);

         /**
          * @return the id of the of the Index Vertex Buffer Object being used with this 
          *    character core model, or 0 for none
          */
         unsigned GetIndexVBO() const;
         
         /// Sets the id of the of the Index Vertex Buffer Object being used with this character core model
         void SetIndexVBO(unsigned);
         
         /**
          * @see dtCore::ShaderManager
          * @return the shader group used to lookup the shader for this character model.
          */
         const std::string& GetShaderGroupName() const;

         /// Sets the shader group name
         void SetShaderGroupName(const std::string& groupName);

         /**
          * @see dtCore::ShaderManager
          * @see #GetShaderGroupName
          * @return the name of the shader within the shader group to use.
          */
         const std::string& GetShaderName() const;

         /// Sets the shader group name
         void SetShaderName(const std::string& name);

         const std::string& GetPoseMeshFilename() const;

         void SetPoseMeshFilename(const std::string& name);

         /**
          * @return the maximum number of bones the skinning shader supports.
          */
         unsigned GetShaderMaxBones() const;

         /// Sets the maximum number of bones the shader supports
         void SetShaderMaxBones(unsigned maxBones);

         LODOptions& GetLODOptions() { return mLODOptions; }
         const LODOptions& GetLODOptions() const { return mLODOptions; }
         
      protected:
         virtual ~Cal3DModelData();

         Cal3DModelData(const Cal3DModelData&); //not implemented
         Cal3DModelData& operator=(const Cal3DModelData&); //not implemented

      private:
         std::string mFilename;
         std::string mShaderName, mShaderGroupName;
         std::string mPoseMeshFilename;
         CalCoreModel* mCoreModel;
         AnimationWrapperArray mAnimWrappers;
         AnimatableArray mAnimatables;
         unsigned mVertexVBO, mIndexVBO;
         unsigned mShaderMaxBones;

         LODOptions mLODOptions;

   };
}

#endif /*DELTA_CAL3D_MODEL_DATA*/
