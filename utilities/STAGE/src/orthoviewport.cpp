/* -*-c++-*-
 * Delta3D Simulation Training And Game Editor (STAGE)
 * STAGE - This source file (.h & .cpp) - Using 'The MIT License'
 * Copyright (C) 2005-2008, Alion Science and Technology Corporation
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
 * Matthew W. Campbell
 */
#include <prefix/dtstageprefix-src.h>
#include <QtGui/QMouseEvent>
#include <dtEditQt/orthoviewport.h>
#include <dtDAL/exceptionenum.h>

namespace dtEditQt
{

   ///////////////////////////////////////////////////////////////////////////////
   IMPLEMENT_ENUM(OrthoViewport::OrthoViewType);
   const OrthoViewport::OrthoViewType OrthoViewport::OrthoViewType::TOP("TOP");
   const OrthoViewport::OrthoViewType OrthoViewport::OrthoViewType::FRONT("FRONT");
   const OrthoViewport::OrthoViewType OrthoViewport::OrthoViewType::SIDE("SIDE");
   ///////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////
   IMPLEMENT_ENUM(OrthoViewport::CameraMode);
   const OrthoViewport::CameraMode
      OrthoViewport::CameraMode::CAMERA_PAN("CAMERA_PAN");
   const OrthoViewport::CameraMode
      OrthoViewport::CameraMode::CAMERA_ZOOM("CAMERA_ZOOM");
   const OrthoViewport::CameraMode
      OrthoViewport::CameraMode::NOTHING("NOTHING");
   ///////////////////////////////////////////////////////////////////////////////

   ///////////////////////////////////////////////////////////////////////////////
   OrthoViewport::OrthoViewport(const std::string& name, QWidget* parent,
      osg::GraphicsContext* shareWith)
      : EditorViewport(ViewportManager::ViewportType::ORTHOGRAPHIC, name, parent, shareWith)
   {
      mCameraMode = &OrthoViewport::CameraMode::NOTHING;
      setViewType(OrthoViewType::TOP,false);

      mObjectMotionModel->SetScale(450.0f);
   }

   ///////////////////////////////////////////////////////////////////////////////
   void OrthoViewport::initializeGL()
   {
      EditorViewport::initializeGL();

      // We do not want OSG to compute our near and far clipping planes when in
      // orthographic view
      mCamera->getDeltaCamera()->SetNearFarCullingMode(dtCore::Camera::NO_AUTO_NEAR_FAR);

      // Default to wireframe view.
      setRenderStyle(Viewport::RenderStyle::WIREFRAME,false);
   }

   ///////////////////////////////////////////////////////////////////////////////
   void OrthoViewport::resizeGL(int width, int height)
   {
      double xDim = (double)width * 0.5;
      double yDim = (double)height * 0.5;

      getCamera()->makeOrtho(-xDim, xDim, -yDim, yDim, -5000.0, 5000.0);
      EditorViewport::resizeGL(width, height);
   }

   ///////////////////////////////////////////////////////////////////////////////
   void OrthoViewport::setViewType(const OrthoViewType& type, bool refreshView)
   {
      if (type == OrthoViewType::TOP)
      {
         mViewType = &OrthoViewType::TOP;
         getCamera()->resetRotation();
         getCamera()->pitch(-90);
      }
      else if (type == OrthoViewType::FRONT)
      {
         mViewType = &OrthoViewType::FRONT;
         getCamera()->resetRotation();
      }
      else if (type == OrthoViewType::SIDE)
      {
         mViewType = &OrthoViewType::SIDE;
         getCamera()->resetRotation();
         getCamera()->yaw(90);
      }

      if (refreshView)
      {
         if (!isInitialized())
         {
            throw dtUtil::Exception(dtDAL::ExceptionEnum::BaseException,"Cannot refresh the viewport. "
               "It has not been initialized.", __FILE__, __LINE__);
         }
         refresh();
      }
   }


   ///////////////////////////////////////////////////////////////////////////////
   bool OrthoViewport::moveCamera(float dx, float dy)
   {
      if (!EditorViewport::moveCamera(dx, dy))
      {
         return false;
      }

      if (*mCameraMode == OrthoViewport::CameraMode::NOTHING || getCamera() == NULL)
      {
         return true;
      }

      float xAmount = (-dx/getMouseSensitivity()*4.0f) / getCamera()->getZoom();
      float yAmount = (dy/getMouseSensitivity()*4.0f) / getCamera()->getZoom();

      if (*mCameraMode == OrthoViewport::CameraMode::CAMERA_PAN)
      {
         getCamera()->move(getCamera()->getRightDir() * xAmount);
         getCamera()->move(getCamera()->getUpDir() * yAmount);
      }
      else if (*mCameraMode == OrthoViewport::CameraMode::CAMERA_ZOOM)
      {
         osg::Vec3 moveVec = mZoomToPosition-getCamera()->getPosition();

         moveVec.normalize();
         if (dy <= -1.0f)
         {
            getCamera()->zoom(1.1f);
         }
         else if (dy >= 1.0f)
         {
            getCamera()->zoom(0.9f);
         }
      }

      return true;
   }


   ///////////////////////////////////////////////////////////////////////////////
   void OrthoViewport::wheelEvent(QWheelEvent* e)
   {
      ViewportManager::GetInstance().emitWheelEvent(this, e);

      if (e->delta() > 0)
      {
         getCamera()->zoom(1.3f);
      }
      else
      {
         getCamera()->zoom(0.7f);
      }

      // The motion model will not update unless the mouse moves,
      // because of this we need to force it to update its' widget
      // geometry.
      mObjectMotionModel->UpdateWidgets();
   }

   ///////////////////////////////////////////////////////////////////////////////
   bool OrthoViewport::beginCameraMode(QMouseEvent* e)
   {
      if (!EditorViewport::beginCameraMode(e))
      {
         return false;
      }

      if (mMouseButton == Qt::LeftButton)
      {
         mCameraMode = &OrthoViewport::CameraMode::CAMERA_PAN;
      }
      else if (mMouseButton == Qt::RightButton)
      {
         osg::Vec3 nearPoint,farPoint;
         //TODO
         //int xLoc = e->pos().x();
         //int yLoc = int(getSceneView()->getViewport()->height()-e->pos().y());
         //getSceneView()->projectWindowXYIntoObject(xLoc, yLoc, nearPoint, farPoint);
         mZoomToPosition = nearPoint;
         mCameraMode = &OrthoViewport::CameraMode::CAMERA_ZOOM;
      }

      setInteractionMode(Viewport::InteractionMode::CAMERA);
      trapMouseCursor();

      return true;
   }

   ///////////////////////////////////////////////////////////////////////////////
   bool OrthoViewport::endCameraMode(QMouseEvent* e)
   {
      if (!EditorViewport::endCameraMode(e))
      {
         return false;
      }

      mObjectMotionModel->SetInteractionEnabled(true);
      mCameraMode = &OrthoViewport::CameraMode::NOTHING;
      setInteractionMode(Viewport::InteractionMode::NOTHING);
      releaseMouseCursor();

      return true;
   }

   ///////////////////////////////////////////////////////////////////////////////
   bool OrthoViewport::beginActorMode(QMouseEvent* e)
   {
      if (!EditorViewport::beginActorMode(e))
      {
         return false;
      }

      return true;
   }

   ///////////////////////////////////////////////////////////////////////////////
   bool OrthoViewport::endActorMode(QMouseEvent* e)
   {
      if (!EditorViewport::endActorMode(e))
      {
         return false;
      }

      return true;
   }

} // namespace dtEditQt
