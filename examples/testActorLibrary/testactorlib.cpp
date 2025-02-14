/* -*-c++-*-
* testActorLibrary - testactorlib (.h & .cpp) - Using 'The MIT License'
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
* William E. Johnson II
*/

#include "testactorlib.h"
#include "testpropertyproxy.h"
#include "testdalenvironmentactor.h"

extern "C" DT_PLUGIN_EXPORT dtDAL::ActorPluginRegistry* CreatePluginRegistry()
{
   return new ExampleActorLib;
}

extern "C" DT_PLUGIN_EXPORT void DestroyPluginRegistry(dtDAL::ActorPluginRegistry *registry)
{
   if (registry != NULL)
      delete registry;
}

ExampleActorLib::ExampleActorLib() : ActorPluginRegistry("ExampleActors")
{
    mDescription = "These are example actors";
}

void ExampleActorLib::RegisterActorTypes()
{
    dtDAL::ActorType *testAllPropertiesType = new dtDAL::ActorType("Test All Properties",
        "dtcore.examples", "Used to test any property types that haven't been tested.");

    dtDAL::ActorType *testEnvActor = new dtDAL::ActorType("Test Environment Actor", "Test Environment Actor");

    mActorFactory->RegisterType<ExampleTestPropertyProxy>(testAllPropertiesType);
    mActorFactory->RegisterType<TestDALEnvironmentActorProxy>(testEnvActor);
}
