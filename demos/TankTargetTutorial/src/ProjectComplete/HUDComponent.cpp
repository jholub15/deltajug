/* -*-c++-*-
 * TutorialLibrary - This source file (.h & .cpp) - Using 'The MIT License'
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
 * @author Curtiss Murphy
 */

#include "HUDComponent.h"

#include <dtUtil/macros.h>
#include <dtCore/globals.h>
#include <dtCore/scene.h>
#include <dtUtil/exception.h>

#include <dtGame/basemessages.h>
#include <dtGame/messagetype.h>
#include <dtGame/exceptionenum.h>

#include <dtCore/globals.h>
#include <dtDAL/project.h>



//////////////////////////////////////////////////////////////////////////
HUDComponent::HUDComponent(dtCore::DeltaWin* win,
                           dtCore::Keyboard* keyboard,
                           dtCore::Mouse* mouse,
                           const std::string& name)
   : dtGame::GMComponent(name)
   , mUnHandledMessages(0)
{
   SetupGUI(win, keyboard, mouse);
}

//////////////////////////////////////////////////////////////////////////
HUDComponent::~HUDComponent()
{
   mGUI->ShutdownGUI();
}

//////////////////////////////////////////////////////////////////////////
void HUDComponent::ProcessMessage(const dtGame::Message& message)
{
   if (message.GetMessageType() == dtGame::MessageType::TICK_LOCAL)
   {
      UpdateSimTime(GetGameManager()->GetSimulationTime());

      // Update AAR text
      if (!mLogController.valid())
      {
         UpdateStaticText(mStateText, "", 1.0f, 1.0f, 1.0f);
      }
      else if (dtGame::LogStateEnumeration::LOGGER_STATE_IDLE == mLogController->GetLastKnownStatus().GetStateEnum())
      {
         UpdateStaticText(mStateText, "IDLE", 1.0f, 1.0f, 1.0f);
      }
      else if (dtGame::LogStateEnumeration::LOGGER_STATE_PLAYBACK == mLogController->GetLastKnownStatus().GetStateEnum())
      {
         UpdateStaticText(mStateText, "PLAYBACK", 0.1f, 0.1f, 1.0f);
      }
      else // if (dtGame::LogStateEnumeration::LOGGER_STATE_RECORD == mLogController->GetLastKnownStatus().GetStateEnum())
      {
         UpdateStaticText(mStateText, "RECORD", 1.0f, 0.1f, 0.1f);
      }
   }
   else if (message.GetMessageType() == dtGame::MessageType::TICK_REMOTE)
   {
      // Do nothing when we get remote tick
   }
   else if (message.GetMessageType() == dtGame::MessageType::INFO_RESTARTED)
   {
      mLogController = dynamic_cast<dtGame::LogController*> (GetGameManager()->
         GetComponentByName("LogController"));
   }
   // sum up all the unhandled messages
   else
   {
      mUnHandledMessages++;
      UpdateNumMessages(mUnHandledMessages);

      UpdateLastMessageName(message.GetMessageType().GetName());
   }
}

//////////////////////////////////////////////////////////////////////////
void HUDComponent::UpdateSimTime(double newTime)
{
   char clin[HUDCONTROLMAXTEXTSIZE]; // general buffer to print

   // Sim Time
   snprintf(clin, HUDCONTROLMAXTEXTSIZE, "SimTime: %.2f", newTime);
   UpdateStaticText(mSimTimeText, clin);
}

//////////////////////////////////////////////////////////////////////////
void HUDComponent::UpdateNumMessages(int messageCount)
{
   char clin[HUDCONTROLMAXTEXTSIZE]; // general buffer to print

   // Num Messages
   snprintf(clin, HUDCONTROLMAXTEXTSIZE, "Num Msgs: %lu", mUnHandledMessages);
   UpdateStaticText(mNumMessagesText, clin);
}

//////////////////////////////////////////////////////////////////////////
void HUDComponent::UpdateLastMessageName(const std::string& messageName)
{
   char clin[HUDCONTROLMAXTEXTSIZE]; // general buffer to print

   // Last Message
   snprintf(clin, HUDCONTROLMAXTEXTSIZE, "Last Msg: %s", messageName.c_str());
   UpdateStaticText(mLastMessageText, clin);
}

//////////////////////////////////////////////////////////////////////////
void HUDComponent::OnAddedToGM()
{
   GetGameManager()->GetScene().AddDrawable(GetGUIDrawable().get());
}

//////////////////////////////////////////////////////////////////////////
void HUDComponent::SetupGUI(dtCore::DeltaWin* win,
                            dtCore::Keyboard* keyboard,
                            dtCore::Mouse* mouse)
{
   try
   {
      // Initialize CEGUI
      mGUI = new dtGUI::CEUIDrawable(win, keyboard, mouse);

      // BEGIN - MAKE THIS PART OF BaseHUDComponent ???
      // probably have params for the scheme, the default font, and main win name

      // get our scheme path
      std::string scheme = "gui/schemes/WindowsLook.scheme";
      std::string path = dtCore::FindFileInPathList(scheme);
      if (path.empty())
      {
         throw dtUtil::Exception(dtGame::ExceptionEnum::GENERAL_GAMEMANAGER_EXCEPTION,
            "Failed to find the scheme file.", __FILE__, __LINE__);
      }

      std::string dir = path.substr(0, path.length() - (scheme.length() - 3));
      dtUtil::FileUtils::GetInstance().PushDirectory(dir);
      CEGUI::SchemeManager::getSingleton().loadScheme(path);
      dtUtil::FileUtils::GetInstance().PopDirectory();

      CEGUI::WindowManager* wm = CEGUI::WindowManager::getSingletonPtr();
      CEGUI::System::getSingleton().setDefaultFont("DejaVuSans-10");
      mMainWindow = wm->createWindow("DefaultGUISheet", "root");
      CEGUI::System::getSingleton().setGUISheet(mMainWindow);

      // END - MAKE THIS PART OF BaseHUDComponent ???


      // main HUD window for drawing, covers full window size
      mOverlay = static_cast<CEGUI::Window*>(
         wm->createWindow("WindowsLook/StaticImage", "Main Overlay"));
      mMainWindow->addChildWindow(mOverlay);
      mOverlay->setPosition(CEGUI::UVector2(cegui_absdim(0), cegui_absdim(0)));
      mOverlay->setSize(CEGUI::UVector2(cegui_reldim(1.0f), cegui_reldim(1.0f)));
      mOverlay->setProperty("FrameEnabled", "false");
      mOverlay->setProperty("BackgroundEnabled", "false");

      // Sim Time
      mSimTimeText = CreateText("Sim Time", mOverlay, "",
         5.0f, 20.0f, 600.0f, 60.0f);
      mSimTimeText->setProperty("TextColours",
         CEGUI::PropertyHelper::colourToString(CEGUI::colour(1.0f, 1.0f, 1.0f)));

      // Num Messages
      mNumMessagesText = CreateText("Num Messages", mOverlay, "",
         5.0f, 45.0f, 600.0f, 60.0f);
      mNumMessagesText->setProperty("TextColours",
         CEGUI::PropertyHelper::colourToString(CEGUI::colour(1.0f, 1.0f, 1.0f)));

      // Last Message
      mLastMessageText = CreateText("Last Message", mOverlay, "Last Msg:",
         5.0f, 70.0f, 600.0f, 60.0f);
      mLastMessageText->setProperty("TextColours",
         CEGUI::PropertyHelper::colourToString(CEGUI::colour(1.0f, 1.0f, 1.0f)));

      // AAR playback/idle state
      mStateText = CreateText("State Text", mOverlay, "",
         5.0f, 100.0f, 600.0f, 60.0f);
      mStateText->setProperty("TextColours",
         CEGUI::PropertyHelper::colourToString(CEGUI::colour(1.0f, 0.1f, 0.1f)));
      //mStateText->setFont("Tahoma-14");

      // Note - don't forget to add the cegui drawable to the scene after this method, or you get nothing.
   }
   catch(CEGUI::Exception& e)
   {
      std::ostringstream oss;
      oss << "CEGUI while setting up GUI: " << e.getMessage().c_str();
      throw dtUtil::Exception(dtGame::ExceptionEnum::GENERAL_GAMEMANAGER_EXCEPTION,
         oss.str(), __FILE__, __LINE__);
   }

}

//////////////////////////////////////////////////////////////////////////
void HUDComponent::UpdateStaticText(CEGUI::Window* textControl, const char* newText,
                                  float red, float blue, float green, float x, float y)
{
   if (textControl != NULL)
   {
      // text and color
      if (newText != NULL && textControl->getText() != std::string(newText))
      {
         textControl->setText(newText);
         if (red >= 0.00 && blue >= 0.0 && green >= 0.0)
         {
            textControl->setProperty("TextColours",
               CEGUI::PropertyHelper::colourToString(CEGUI::colour(red, green, blue)));
         }
      }
      // position
      if (x > 0.0 && y > 0.0)
      {
         CEGUI::UVector2 position = textControl->getPosition();
         CEGUI::UVector2 newPos(cegui_absdim(x), cegui_absdim(y));
         if (position != newPos)
         {
            textControl->setPosition(newPos);
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////
CEGUI::Window* HUDComponent::CreateText(const std::string& name, CEGUI::Window* parent, const std::string& text,
                                        float x, float y, float width, float height)
{
   CEGUI::WindowManager* wm = CEGUI::WindowManager::getSingletonPtr();

   // create base window and set our default attribs
   CEGUI::Window* result = wm->createWindow("WindowsLook/StaticText", name);
   parent->addChildWindow(result);
   result->setText(text);
   result->setPosition(CEGUI::UVector2(cegui_absdim(x), cegui_absdim(y)));
   result->setSize(CEGUI::UVector2(cegui_absdim(width), cegui_absdim(height)));
   result->setProperty("FrameEnabled", "false");
   result->setProperty("BackgroundEnabled", "false");
   result->setHorizontalAlignment(CEGUI::HA_LEFT);
   result->setVerticalAlignment(CEGUI::VA_TOP);
   result->show();

   return result;
}

