#include <cstdio>
#include <cstdlib>

#include <QtGui/QApplication>

#include "mainwindow.h"
#include "aiutilityapp.h"

#include <dtUtil/macros.h>

#include <dtCore/deltawin.h>

#include <dtQt/qtguiwindowsystemwrapper.h>
#include <dtQt/osggraphicswindowqt.h>
#include <dtQt/osgadapterwidget.h>

#include <dtUtil/log.h>

int main(int argc, char* argv[])
{
   QApplication qapp(argc, argv);

   dtQt::QtGuiWindowSystemWrapper::EnableQtGUIWrapper();

   dtCore::RefPtr<AIUtilityApp> app = new AIUtilityApp;

   dtQt::OSGGraphicsWindowQt* osgGraphWindow = dynamic_cast<dtQt::OSGGraphicsWindowQt*>(app->GetWindow()->GetOsgViewerGraphicsWindow());

   if (osgGraphWindow == NULL)
   {
      LOG_ERROR("Unable to initialize. The deltawin could not be created with a QGLWidget.");
      return EXIT_FAILURE;
   }

   MainWindow win(*osgGraphWindow->GetQGLWidget());

   QObject::connect(QApplication::instance(), SIGNAL(lastWindowClosed()), app.get(), SLOT(DoQuit()));
   QObject::connect(app.get(), SIGNAL(AIPluginInterfaceChanged(dtAI::AIPluginInterface*)),
            &win, SLOT(SetAIPluginInterface(dtAI::AIPluginInterface*)));
   QObject::connect(app.get(), SIGNAL(CameraTransformChanged(const dtCore::Transform&)),
            &win, SLOT(OnCameraTransformChanged(const dtCore::Transform&)));
   QObject::connect(app.get(), SIGNAL(Error(const std::string&)),
            &win, SLOT(OnError(const std::string&)));
   QObject::connect(&win, SIGNAL(RequestCameraTransformChange(const dtCore::Transform&)),
            app.get(), SLOT(TransformCamera(const dtCore::Transform&)));
   QObject::connect(&win, SIGNAL(AddAIInterfaceToMap(const std::string&)),
            app.get(), SLOT(AddAIInterfaceToMap(const std::string&)));

   QObject::connect(&win, SIGNAL(ProjectContextChanged(const std::string&)), app.get(), SLOT(SetProjectContext(const std::string&)));
   QObject::connect(&win, SIGNAL(MapSelected(const std::string&)), app.get(), SLOT(ChangeMap(const std::string&)));
   QObject::connect(&win, SIGNAL(CloseMapSelected()), app.get(), SLOT(CloseMap()));

   win.show();

   app->Config();
   qapp.exec();

   return EXIT_SUCCESS;
}
