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
 */

#include <dtAnim/submesh.h>
#include <dtAnim/cal3dmodelwrapper.h>
#include <dtAnim/cal3dmodeldata.h>
#include <dtAnim/cal3ddatabase.h>

#include <dtUtil/log.h>
#include <dtUtil/mathdefines.h>

#include <osg/Material>
#include <osg/Texture2D>
#include <osg/PolygonMode>
#include <osg/Geometry>
#include <osg/CullFace>
#include <osg/Math>
#include <osg/BlendFunc>
#include <osg/GLExtensions>

#include <cassert>

namespace dtAnim
{

   class SubmeshComputeBound : public osg::Drawable::ComputeBoundingBoxCallback
   {
      public:
         SubmeshComputeBound(const osg::BoundingBox& boundingBox)
            : mBoundingBox(boundingBox)
         {
         }

         /*virtual*/ osg::BoundingBox computeBound(const osg::Drawable&) const
         {
            // temp until a better solution is implemented
            return mBoundingBox;
         }

         const osg::BoundingBox& mBoundingBox;
   };


   ////////////////////////////////////////////////////////////////////////////////////////
   osg::Object* SubmeshUserData::cloneType() const
   {
      return new SubmeshUserData;
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   osg::Object* SubmeshUserData::clone(const osg::CopyOp& op) const
   {
      SubmeshUserData* theClone = static_cast<SubmeshUserData*>(cloneType());
      theClone->mLOD = mLOD;
      return theClone;
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   bool SubmeshUserData::isSameKindAs(const Object* o) const
   {
      return dynamic_cast<const SubmeshUserData*>(o) != NULL;
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   const char* SubmeshUserData::libraryName() const { return "dtAnim"; }

   ////////////////////////////////////////////////////////////////////////////////////////
   const char* SubmeshUserData::className() const { return "SubmeshUserData";}

   ////////////////////////////////////////////////////////////////////////////////////////
   void SubmeshDirtyCallback::update (osg::NodeVisitor*, osg::Drawable* d)
   {
      d->dirtyBound();
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   bool SubmeshCullCallback::cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::RenderInfo* renderInfo) const
   {
      if (!mWrapper->IsMeshVisible(mMeshID) || mWrapper->GetMeshCount() <= mMeshID)
      {
         return true;
      }

      osg::Node* parent = nv->getNodePath().back();
      if (parent != NULL)
      {
         float distance = nv->getDistanceToEyePoint(parent->getBound().center(), true);

         SubmeshDrawable* submeshDraw = dynamic_cast<SubmeshDrawable*>(drawable);
         if (submeshDraw != NULL)
         {
            // disappear once the max distance is reached
            if (distance > submeshDraw->GetLODOptions().GetMaxVisibleDistance())
               return true;

            float start = submeshDraw->GetLODOptions().GetStartDistance();
            float end   = submeshDraw->GetLODOptions().GetEndDistance();
            float slope = 1.0f / (end - start);

            float lod = 1.0f - (slope*(distance - start));
            dtUtil::Clamp(lod, 0.0f, 1.0f);
            if (!osg::isNaN(lod))
            {
               ///copy user data to the state.
               //dtCore::RefPtr<SubmeshUserData> userData = new SubmeshUserData;
               //userData->mLOD = lod;
               //renderInfo->setUserData(userData.get());

               submeshDraw->SetCurrentLOD(lod);

               //std::cout << "Setting LOD to " << lod << std::endl;
            }
         }
      }

      return osg::Geometry::CullCallback::cull(nv, drawable, renderInfo);
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   SubmeshDrawable::SubmeshDrawable(Cal3DModelWrapper* wrapper, unsigned mesh, unsigned Submesh)
      : mMeshID(mesh)
      , mSubmeshID(Submesh)
      , mMeshVBO(0)
      , mMeshIndices(0)
      , mCurrentLOD(1.0f)
      , mInitalized(false)
      , mVBOContextID(-1)
      , mWrapper(wrapper)
      , mMeshVertices(NULL)
      , mMeshNormals(NULL)
      , mMeshTextureCoordinates(NULL)
      , mMeshFaces(NULL)
   {
      setUseDisplayList(false);
      setUseVertexBufferObjects(true);
      SetUpMaterial();

      osg::StateSet* ss = getOrCreateStateSet();
      ss->setAttributeAndModes(new osg::CullFace);

      mModelData = Cal3DDatabase::GetInstance().GetModelData(*mWrapper);

      if (!mModelData.valid())
      {
         LOG_ERROR("Model does not have model data.  Unable to cache vertex buffers.");
      }

      //setUpdateCallback(new SubmeshDirtyCallback());
      setCullCallback(new SubmeshCullCallback(*mWrapper, mMeshID));
      setComputeBoundingBoxCallback(new SubmeshComputeBound(mBoundingBox));
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   SubmeshDrawable::~SubmeshDrawable(void)
   {
      if (mVBOContextID > 0)
      {
         osg::Drawable::Extensions* glExt = osg::Drawable::getExtensions(mVBOContextID, true);

         GLuint bufferID = mMeshVBO;
         if (bufferID > 0)
         {
            glExt->glDeleteBuffers(1, &bufferID);
         }

         bufferID = mMeshIndices;
         if (bufferID > 0)
         {
            glExt->glDeleteBuffers(1, &bufferID);
         }
      }

      delete [] mMeshVertices;
      delete [] mMeshNormals;
      delete [] mMeshFaces;
      delete [] mMeshTextureCoordinates;
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   void SubmeshDrawable::SetUpMaterial()
   {
      osg::StateSet* set = this->getOrCreateStateSet();

      if (mWrapper->BeginRenderingQuery())
      {
         // select mesh and Submesh for further data access
         if (mWrapper->SelectMeshSubmesh(mMeshID, mSubmeshID))
         {
            osg::Material* material = new osg::Material();
            set->setAttributeAndModes(material, osg::StateAttribute::ON);

            osg::BlendFunc* bf = new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            set->setMode(GL_BLEND, osg::StateAttribute::ON);
            set->setAttributeAndModes(bf, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            set->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

            unsigned char meshColor[4];
            osg::Vec4 materialColor;

            // set the material ambient color
            mWrapper->GetAmbientColor(&meshColor[0]);
            materialColor[0] = meshColor[0] / 255.0f;
            materialColor[1] = meshColor[1] / 255.0f;
            materialColor[2] = meshColor[2] / 255.0f;
            materialColor[3] = meshColor[3] / 255.0f;
            //if (materialColor[3] == 0) materialColor[3]=1.0f;
            material->setAmbient(osg::Material::FRONT_AND_BACK, materialColor);

            // set the material diffuse color
            mWrapper->GetDiffuseColor(&meshColor[0]);
            materialColor[0] = meshColor[0] / 255.0f;
            materialColor[1] = meshColor[1] / 255.0f;
            materialColor[2] = meshColor[2] / 255.0f;
            materialColor[3] = meshColor[3] / 255.0f;
            //if (materialColor[3] == 0) materialColor[3]=1.0f;
            material->setDiffuse(osg::Material::FRONT_AND_BACK, materialColor);

            // set the material specular color
            mWrapper->GetSpecularColor(&meshColor[0]);
            materialColor[0] = meshColor[0] / 255.0f;
            materialColor[1] = meshColor[1] / 255.0f;
            materialColor[2] = meshColor[2] / 255.0f;
            materialColor[3] = meshColor[3] / 255.0f;
            //if (materialColor[3] == 0) materialColor[3]=1.0f;
            material->setSpecular(osg::Material::FRONT_AND_BACK, materialColor);

            // set the material shininess factor
            float shininess;
            shininess = mWrapper->GetShininess();
            material->setShininess(osg::Material::FRONT_AND_BACK, shininess);

            if (mWrapper->GetMapCount() > 0)
            {
               unsigned i = 0;
               osg::Texture2D* texture = reinterpret_cast<osg::Texture2D*>(mWrapper->GetMapUserData(i));

               while (texture != NULL)
               {
                  set->setTextureAttributeAndModes(i, texture, osg::StateAttribute::ON);
                  texture = reinterpret_cast<osg::Texture2D*>(mWrapper->GetMapUserData(++i));
               }
            }
         }
         mWrapper->EndRenderingQuery();
      }
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   void SubmeshDrawable::InitVertexBuffers(osg::State& state) const
   {
      osg::Drawable::Extensions* glExt = osg::Drawable::getExtensions(state.getContextID(), true);

      mVBOContextID = state.getContextID();

      int vertexCountTotal = 0;
      int faceCountTotal = 0;

      for (unsigned i = 0; i < LOD_COUNT; ++i)
      {
         float lodToQuery = float(i + 1) / float(LOD_COUNT);
         mWrapper->SetLODLevel(lodToQuery);

         if (mWrapper->BeginRenderingQuery())
         {
            // select mesh and Submesh for further data access
            if (mWrapper->SelectMeshSubmesh(mMeshID, mSubmeshID))
            {
               // begin the rendering loop t get the faces.

               mVertexCount[i] = mWrapper->GetVertexCount();
               mFaceCount[i] = mWrapper->GetFaceCount();

               vertexCountTotal += mVertexCount[i];
               faceCountTotal += mFaceCount[i];
            }
            mWrapper->EndRenderingQuery();
         }
      }

      //save the offsets for easy lookup.
      mVertexOffsets[0] = 0;
      mFaceOffsets[0] = 0;

      for (unsigned i = 1; i < LOD_COUNT; ++i)
      {
         mVertexOffsets[i] = mVertexOffsets[i - 1] + mVertexCount[i - 1];
         mFaceOffsets[i] = mFaceOffsets[i - 1 ] + mFaceCount[i - 1];
      }

      GLuint tempId;
      glExt->glGenBuffers(1, &tempId);
      mMeshVBO = tempId;
      glExt->glBindBuffer(GL_ARRAY_BUFFER_ARB, mMeshVBO);
      glExt->glBufferData(GL_ARRAY_BUFFER_ARB, STRIDE_BYTES * vertexCountTotal, NULL, GL_DYNAMIC_DRAW_ARB);

      float* vertexArray = reinterpret_cast<float*>(glExt->glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB));

      glExt->glGenBuffers(1, &tempId);
      mMeshIndices = tempId;
      mModelData->SetIndexVBO(mMeshIndices);

      glExt->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, mMeshIndices);
      glExt->glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB, faceCountTotal * 3 * sizeof(CalIndex), NULL, GL_STATIC_DRAW_ARB);

      CalIndex* indexArray = reinterpret_cast<CalIndex*>(glExt->glMapBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB));

      ///Fill the index and vertex VBOs once for each level of detail
      for (unsigned i = 0; i < LOD_COUNT; ++i)
      {
         float lodToQuery = float(i + 1) / float(LOD_COUNT);
         mWrapper->SetLODLevel(lodToQuery);

         // begin the rendering loop t get the faces.
         if (mWrapper->BeginRenderingQuery())
         {
            // select mesh and Submesh for further data access
            if (mWrapper->SelectMeshSubmesh(mMeshID, mSubmeshID))
            {
               int vertexCount = mWrapper->GetVertices(vertexArray, STRIDE_BYTES);

               // get the transformed normals of the Submesh
               mWrapper->GetNormals(vertexArray + 3, STRIDE_BYTES);

               mWrapper->GetTextureCoords(0, vertexArray + 6, STRIDE_BYTES);

               mWrapper->GetTextureCoords(1, vertexArray + 8, STRIDE_BYTES);

               //invert texture coordinates.
               for (unsigned i = 0; i < vertexCount * STRIDE; i += STRIDE)
               {
                  vertexArray[i + 7] = 1.0f - vertexArray[i + 7]; //the odd texture coordinates in cal3d are flipped, not sure why
                  vertexArray[i + 9] = 1.0f - vertexArray[i + 9]; //the odd texture coordinates in cal3d are flipped, not sure why
               }

               int indexCount = mWrapper->GetFaces(indexArray);

               ///offset into the vbo to fill the correct lod.
               vertexArray += vertexCount * STRIDE;
               indexArray += indexCount * 3;
            }
         }

         mWrapper->EndRenderingQuery();
      }

      glExt->glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
      glExt->glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB);

      glExt->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
      glExt->glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
   }


#define BUFFER_OFFSET(x)((GLvoid*) (0 + ((x) * sizeof(float))))
#define INDEX_OFFSET(x)((GLvoid*) (0 + ((x) * sizeof(CalIndex))))

   ////////////////////////////////////////////////////////////////////////////////////////
   void SubmeshDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
   {
      if (VBOAvailable(renderInfo)) 
      {
         DrawUsingVBO(renderInfo);
      }
      else 
      {
         DrawUsingPrimitives(renderInfo);
      }
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   void SubmeshDrawable::accept(osg::PrimitiveFunctor& functor) const
   {
      if (mMeshVBO == 0)
      {
         return;
      }

      /// this processes the lowest LOD at the moment,
      /// because that's what's loaded at the front of the VBO.
      osg::Drawable::Extensions* glExt = osg::Drawable::getExtensions(0, true);
      glExt->glBindBuffer(GL_ARRAY_BUFFER_ARB, mMeshVBO);
      osg::Vec3f* vertexArray = reinterpret_cast<osg::Vec3f*>(glExt->glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB));

      functor.setVertexArray(mVertexCount[0], vertexArray);

      glExt->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, mMeshIndices);
      void* indexArray = glExt->glMapBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB);

      if (sizeof(CalIndex) == sizeof(short))
      {
         osg::ref_ptr<osg::DrawElementsUShort> pset = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES,
                  mFaceCount[0] * 3, (GLushort*) indexArray);
         pset->accept(functor);
      }
      else
      {
         osg::ref_ptr<osg::DrawElementsUInt> pset = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES,
                  mFaceCount[0] * 3, (GLuint*) indexArray);
         pset->accept(functor);
      }

      glExt->glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB);
      glExt->glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
      glExt->glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
      glExt->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   osg::Object* SubmeshDrawable::clone(const osg::CopyOp&) const
   {
      return new SubmeshDrawable(mWrapper.get(), mMeshID, mSubmeshID);
   }

   ////////////////////////////////////////////////////////////////////////////////////////
   osg::Object* SubmeshDrawable::cloneType() const
   {
      return new SubmeshDrawable(mWrapper.get(), mMeshID, mSubmeshID);
   }

   //////////////////////////////////////////////////////////////////////////
   bool SubmeshDrawable::VBOAvailable(const osg::RenderInfo& renderInfo) const
   {
      const osg::State *state = renderInfo.getState();
      if (state != NULL)
      {
         return getUseVertexBufferObjects() && state->isVertexBufferObjectSupported();
      }
      else
      {
         return false;
      }
   }

   //////////////////////////////////////////////////////////////////////////
   void SubmeshDrawable::DrawUsingVBO(osg::RenderInfo &renderInfo) const
   {
      osg::State& state = *renderInfo.getState();

      //bind the VBO's
      state.disableAllVertexArrays();

      bool initializedThisDraw = false;
      if (!mInitalized)
      {
         InitVertexBuffers(state);
         mInitalized = true;
         initializedThisDraw = true;
      }

      // begin the rendering loop
      if(mWrapper->BeginRenderingQuery())
      {
         // select mesh and Submesh for further data access
         if(mWrapper->SelectMeshSubmesh(mMeshID, mSubmeshID))
         {
            SubmeshUserData* userData = 
               dynamic_cast<SubmeshUserData*>(renderInfo.getUserData());

            float finalLOD = 0.0;
            if (userData != NULL)
            {
               finalLOD = userData->mLOD;
            }
            else
            {
               finalLOD = GetCurrentLOD();
            }

            /// the inverse of ((index + 1) / LOD_COUNT) = lodfloat as seen in InitVertexBuffers.

            unsigned lodIndex = unsigned((float(LOD_COUNT) * finalLOD) - 1.0f);
            dtUtil::Clamp(lodIndex, 0U, LOD_COUNT - 1U);

            ///quantify the lod floating point number.
            finalLOD = (float(lodIndex + 1) / float(LOD_COUNT));

            mWrapper->SetLODLevel(finalLOD);

            const Extensions* glExt = getExtensions(state.getContextID(), true);

            glExt->glBindBuffer(GL_ARRAY_BUFFER_ARB, mMeshVBO);
            if (!initializedThisDraw)
            {
               float* vertexArray = reinterpret_cast<float*>(glExt->glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB));

               ///offset into the vbo to fill the correct lod.
               vertexArray += mVertexOffsets[lodIndex] * STRIDE;

               mWrapper->GetVertices(vertexArray, STRIDE_BYTES);

               // get the transformed normals of the Submesh
               mWrapper->GetNormals(vertexArray + 3, STRIDE_BYTES);
               glExt->glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
            }

            unsigned offset = mVertexOffsets[lodIndex] * STRIDE;

            state.setVertexPointer(3, GL_FLOAT, STRIDE_BYTES, BUFFER_OFFSET(0 + offset));

            state.setNormalPointer(GL_FLOAT, STRIDE_BYTES, BUFFER_OFFSET(3 + offset));
            state.setTexCoordPointer(0, 2, GL_FLOAT, STRIDE_BYTES, BUFFER_OFFSET(6 + offset));
            state.setTexCoordPointer(1, 2, GL_FLOAT, STRIDE_BYTES, BUFFER_OFFSET(8 + offset));

            //make the call to render
            glExt->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, mMeshIndices);

            glDrawElements(GL_TRIANGLES, mFaceCount[lodIndex] * 3U, (sizeof(CalIndex) < 4) ?
                           GL_UNSIGNED_SHORT: GL_UNSIGNED_INT, INDEX_OFFSET(3U * mFaceOffsets[lodIndex]));

            glExt->glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
            glExt->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

            // This data could potential cause problems
            // so we clear it out here (i.e CEGUI incompatible)
            state.setVertexPointer(NULL);

            state.setNormalPointer(NULL);
            state.setTexCoordPointer(0, NULL);
            state.setTexCoordPointer(1, NULL);
         }
      }

      // end the rendering
      mWrapper->EndRenderingQuery();
   }

   //////////////////////////////////////////////////////////////////////////
   void SubmeshDrawable::DrawUsingPrimitives(osg::RenderInfo &renderInfo) const
   {
      osg::State& state = *renderInfo.getState();

      // VBO's arn't available, so use conventional rendering path.
      // begin the rendering loop
      if(mWrapper->BeginRenderingQuery())
      {
         // select mesh and submesh for further data access
         if(mWrapper->SelectMeshSubmesh(mMeshID, mSubmeshID))
         {
            int vertexCount = mWrapper->GetVertexCount();
            int faceCount = mWrapper->GetFaceCount();
            if (!mMeshVertices) 
            {
               mMeshVertices           = new float[vertexCount*3];
               mMeshNormals            = new float[vertexCount*3];
               mMeshTextureCoordinates = new float[vertexCount*2];
               mMeshFaces              = new int[faceCount*3];
               mWrapper->GetFaces(mMeshFaces);
            }

            // get the transformed vertices of the submesh
            vertexCount = mWrapper->GetVertices(mMeshVertices);

            // get the transformed normals of the submesh
            mWrapper->GetNormals(mMeshNormals);

            // get the texture coordinates of the submesh
            // this is still buggy, it renders only the first texture.
            // it should be a loop rendering each texture on its own texture unit
            unsigned tcount = mWrapper->GetTextureCoords(0, mMeshTextureCoordinates);

            // flip vertical coordinates
            for (int i = 1; i < vertexCount * 2; i += 2)
            {
               mMeshTextureCoordinates[i] = 1.0f - mMeshTextureCoordinates[i];
            }

            // set the vertex and normal buffers
            state.setVertexPointer(3, GL_FLOAT, 0, mMeshVertices);
            state.setNormalPointer(GL_FLOAT, 0, mMeshNormals);

            // set the texture coordinate buffer and state if necessary
            if((mWrapper->GetMapCount() > 0) && (tcount > 0))
            {
               // set the texture coordinate buffer
               state.setTexCoordPointer(0, 2, GL_FLOAT, 0, mMeshTextureCoordinates);
            }

            // White 
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);


            // draw the submesh
            if(sizeof(CalIndex) == sizeof(short))
            {
               glDrawElements(GL_TRIANGLES, faceCount * 3, GL_UNSIGNED_SHORT, mMeshFaces);
            }
            else
            {
               glDrawElements(GL_TRIANGLES, faceCount * 3, GL_UNSIGNED_INT, mMeshFaces);
            }

            // get the faces of the submesh for the next frame
            faceCount = mWrapper->GetFaces(mMeshFaces);
         }

         // end the rendering
         mWrapper->EndRenderingQuery();
      }
   }
} // namespace dtAnim
