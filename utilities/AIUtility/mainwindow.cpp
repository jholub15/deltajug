/* -*-c++-*-
 * Delta3D
 * Copyright 2009, Alion Science and Technology
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
 * This software was developed by Alion Science and Technology Corporation under
 * circumstances in which the U. S. Government may have rights in the software.
 *
 * David Guthrie
 */

#include "mainwindow.h"
#include <ui_mainwindow.h>
#include <dtQt/deltastepper.h>
#include <dtQt/projectcontextdialog.h>
#include <dtQt/dialoglistselection.h>

#include <QtGui/QCloseEvent>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>
#include <QtCore/QSettings>

#include <dtCore/transform.h>
#include <dtDAL/map.h>
#include <dtDAL/project.h>
#include <dtDAL/propertycontainer.h>
#include <dtDAL/actorproperty.h>
#include "aipropertyeditor.h"
#include "waypointbrowser.h"

#include <dtAI/aiplugininterface.h>
#include <dtAI/aidebugdrawable.h>
#include <dtAI/waypointrenderinfo.h>
#include <dtAI/waypointpropertycontainer.h>
#include <dtAI/waypointpropertycache.h>

#include <set>

const std::string MainWindow::ORG_NAME("delta3d.org");
const std::string MainWindow::APP_NAME("AIUtility");
const std::string MainWindow::PROJECT_CONTEXT_SETTING("ProjectContext");
const std::string MainWindow::CURRENT_MAP_SETTING("CurrentMap");
const std::string MainWindow::WINDOW_SETTINGS("WindowSettings");

//////////////////////////////////////////////
MainWindow::MainWindow(QWidget& mainWidget)
   : mUi(new Ui::MainWindow)
   , mCentralWidget(mainWidget)
   , mPropertyEditor(*new AIPropertyEditor(*this))
   , mWaypointBrowser(*new WaypointBrowser(*this))
   , mPluginInterface(NULL)
{
   mUi->setupUi(this);

   setCentralWidget(&mCentralWidget);
   setWindowTitle(tr("AI Utility"));

   mCurrentCameraTransform.MakeIdentity();

   connect(mUi->mActionOpenMap, SIGNAL(triggered()), this, SLOT(OnOpenMap()));
   connect(mUi->mActionCloseMap, SIGNAL(triggered()), this, SLOT(OnCloseMap()));
   connect(mUi->mActionSave, SIGNAL(triggered()), this, SLOT(OnSave()));
   connect(mUi->mChangeContextAction, SIGNAL(triggered()), this, SLOT(ChangeProjectContext()));
   connect(mUi->mActionRenderingOptions, SIGNAL(triggered()), this, SLOT(SelectRenderingOptions()));

   connect(mUi->mActionPropertyEditorVisible, SIGNAL(toggled(bool)), this, SLOT(OnPropertyEditorShowHide(bool)));
   connect(mUi->mActionWaypointBrowserVisible, SIGNAL(toggled(bool)), this, SLOT(OnWaypointBrowserShowHide(bool)));

   connect(&mPropertyEditor, SIGNAL(SignalPropertyChangedFromControl(dtDAL::PropertyContainer&, dtDAL::ActorProperty&)),
            this, SLOT(PropertyChangedFromControl(dtDAL::PropertyContainer&, dtDAL::ActorProperty&)));

   connect(&mWaypointBrowser, SIGNAL(RequestCameraTransformChange(const dtCore::Transform&)),
            this, SLOT(OnChildRequestCameraTransformChange(const dtCore::Transform&)));

   connect(&mWaypointBrowser, SIGNAL(WaypointSelectionChanged(std::vector<dtAI::WaypointInterface*>&)),
            this, SLOT(OnWaypointSelectionChanged(std::vector<dtAI::WaypointInterface*>&)));

   addDockWidget(Qt::LeftDockWidgetArea, &mPropertyEditor);
   addDockWidget(Qt::RightDockWidgetArea, &mWaypointBrowser);

   mPropertyEditor.installEventFilter(this);
   mWaypointBrowser.installEventFilter(this);
}

//////////////////////////////////////////////
MainWindow::~MainWindow()
{
   delete mUi;
   mUi = NULL;
}
//////////////////////////////////////////////
void MainWindow::showEvent(QShowEvent* e)
{
   if (!e->spontaneous())
   {
      QSettings settings(ORG_NAME.c_str(), APP_NAME.c_str());

      restoreGeometry(settings.value(WINDOW_SETTINGS.c_str()).toByteArray());

      QString projectContext = settings.value(PROJECT_CONTEXT_SETTING.c_str()).toString();


      if (!projectContext.isEmpty())
      {
         emit ProjectContextChanged(projectContext.toStdString());
      }

      ChangeMap(settings.value(CURRENT_MAP_SETTING.c_str()).toString());

      EnableOrDisableControls();
   }
}

//////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent* e)
{
   //Disconnect the central widget because OSG wants to close it itself.
   mCentralWidget.setParent(NULL);

   QSettings settings(ORG_NAME.c_str(), APP_NAME.c_str());
   settings.setValue(WINDOW_SETTINGS.c_str(), saveGeometry());
   settings.sync();

   QApplication::quit();
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnError(const std::string& message)
{
   QMessageBox::critical(this, "Error", tr(message.c_str()));
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::ChangeProjectContext()
{
   dtQt::ProjectContextDialog dialog(this);

   if (dialog.exec() == QDialog::Accepted)
   {
      QSettings settings(ORG_NAME.c_str(), APP_NAME.c_str());

      emit ProjectContextChanged(dialog.getProjectPath().toStdString());

      settings.setValue(PROJECT_CONTEXT_SETTING.c_str(), dialog.getProjectPath());
      settings.sync();
      EnableOrDisableControls();
   }
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnOpenMap()
{
   dtQt::DialogListSelection openMapDialog(this, tr("Open Existing Map"), tr("Available Maps"));

   QStringList listItems;
   const std::set<std::string>& mapNames = dtDAL::Project::GetInstance().GetMapNames();
   for (std::set<std::string>::const_iterator i = mapNames.begin(); i != mapNames.end(); ++i)
   {
      listItems << i->c_str();
   }

   openMapDialog.SetListItems(listItems);
   if (openMapDialog.exec() == QDialog::Accepted)
   {
      ChangeMap(openMapDialog.GetSelectedItem());
   }
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::ChangeMap(const QString& newMap)
{
   QApplication::setOverrideCursor(Qt::WaitCursor);
   if (newMap.isEmpty())
   {
      emit CloseMapSelected();
   }
   else
   {
      emit MapSelected(newMap.toStdString());
   }
   QApplication::restoreOverrideCursor();

   mCurrentMapName = newMap;
   QSettings settings(ORG_NAME.c_str(), APP_NAME.c_str());
   settings.setValue(CURRENT_MAP_SETTING.c_str(), mCurrentMapName);
   settings.sync();
   EnableOrDisableControls();
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnCloseMap()
{
   if (QMessageBox::question(this, tr("Close Map"), tr("Do you want to close the currently opened map?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
   {
      ChangeMap(tr(""));
   }
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnSave()
{
   if (mPluginInterface != NULL)
   {
      // save the one that was last opened.  It would be nice to have a save as.
      if (!mPluginInterface->SaveWaypointFile())
      {
         QMessageBox::critical(this, "Error Saving", "Saving the waypoint file failed for an unknown reason");
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::EnableOrDisableControls()
{
   mUi->mActionOpenMap->setEnabled(!dtDAL::Project::GetInstance().GetContext().empty());
   mUi->mActionCloseMap->setEnabled(!mCurrentMapName.isEmpty());
   // Stop from changing context unless the map is closed. It works around a bug.
   // since the map doesn't change immediately in the GM, we can't just change maps.
   mUi->mChangeContextAction->setEnabled(mCurrentMapName.isEmpty());

   mUi->mActionWaypointBrowserVisible->setChecked(mWaypointBrowser.isVisible());
   mUi->mActionPropertyEditorVisible->setChecked(mPropertyEditor.isVisible());
}

////////////////////////////////////////////////////////////////////////////////
dtAI::AIPluginInterface* MainWindow::GetAIPluginInterface()
{
   return mPluginInterface;
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::SetAIPluginInterface(dtAI::AIPluginInterface* interface)
{
   mPluginInterface = interface;
   EnableOrDisableControls();
   // clear the selection
   dtQt::BasePropertyEditor::PropertyContainerRefPtrVector selected;
   mPropertyEditor.HandlePropertyContainersSelected(selected);
   mWaypointBrowser.SetPluginInterface(interface);

   if (interface == NULL && !mCurrentMapName.isEmpty())
   {
      if (QMessageBox::question(this, tr("AI Interface Actor"), tr("No AI interface actor was found in the map that was opened.\n"
               "Would you like to add the default one and re-save the map?\n"
               "(No will close the map)"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
         emit AddAIInterfaceToMap(mCurrentMapName.toStdString());
      }
      else
      {
         // Close the map.
         ChangeMap(tr(""));
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
   bool result = false;
   if (event->type() == QEvent::Close)
   {
      if (object == &mPropertyEditor)
      {
         mUi->mActionPropertyEditorVisible->setChecked(false);
         result = true;
      }
      else if (object == &mWaypointBrowser)
      {
         mUi->mActionWaypointBrowserVisible->setChecked(false);
         result = true;
      }
   }
   return result;
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::SelectRenderingOptions()
{
   if (mPluginInterface != NULL)
   {
      dtQt::BasePropertyEditor::PropertyContainerRefPtrVector selected;
      dtAI::WaypointRenderInfo& ri = mPluginInterface->GetDebugDrawable()->GetRenderInfo();
      selected.push_back(&ri);
      mPropertyEditor.HandlePropertyContainersSelected(selected);
   }
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::PropertyChangedFromControl(dtDAL::PropertyContainer& pc, dtDAL::ActorProperty& ap)
{
   dtAI::WaypointRenderInfo& ri = mPluginInterface->GetDebugDrawable()->GetRenderInfo();
   if (&pc == &ri)
   {
      mPluginInterface->GetDebugDrawable()->OnRenderInfoChanged();
   }
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnCameraTransformChanged(const dtCore::Transform& xform)
{
   mCurrentCameraTransform = xform;
   mWaypointBrowser.SetCameraTransform(xform);
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnChildRequestCameraTransformChange(const dtCore::Transform& xform)
{
   emit RequestCameraTransformChange(xform);
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnWaypointSelectionChanged(std::vector<dtAI::WaypointInterface*>& waypoints)
{
   if (mPluginInterface == NULL)
   {
      return;
   }

   std::vector<dtCore::RefPtr<dtDAL::PropertyContainer> > propertyContainers;
   propertyContainers.reserve(waypoints.size());

   for (size_t i = 0; i < waypoints.size(); ++i)
   {
      dtAI::WaypointInterface* wpi = waypoints[i];
      propertyContainers.push_back(mPluginInterface->CreateWaypointPropertyContainer(wpi->GetWaypointType(), wpi));
   }

   mPropertyEditor.HandlePropertyContainersSelected(propertyContainers);
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnPropertyEditorShowHide(bool checked)
{
   mPropertyEditor.setVisible(checked);
}

////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnWaypointBrowserShowHide(bool checked)
{
   mWaypointBrowser.setVisible(checked);
}

