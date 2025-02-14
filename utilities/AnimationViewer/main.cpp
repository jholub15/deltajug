#include <QtGui/QApplication>
#include <dtQt/qtguiwindowsystemwrapper.h>
#include <dtQt/deltastepper.h>
#include <dtCore/system.h>
#include "MainWindow.h"
#include "Viewer.h"


int main(int argc, char *argv[]) 
{
   QApplication qapp(argc, argv);  

   //Create special QGLWidget's when we create DeltaWin instances
   dtQt::QtGuiWindowSystemWrapper::EnableQtGUIWrapper();

   //The main UI window
   MainWindow win;

   dtCore::RefPtr<Viewer> mViewer = new Viewer();
   mViewer->Config();

   win.SetViewer(mViewer.get());
   win.OnConfiged();
   win.show();

   //create a little class to ensure Delta3D performs Window "steps"
   dtCore::System::GetInstance().Start();
   dtQt::DeltaStepper stepper;
   stepper.Start();

   if (argc >= 2)
   {
      win.LoadCharFile(QCoreApplication::arguments().at(1));
   }

   qapp.exec();
   stepper.Stop();

   return EXIT_SUCCESS;
}
