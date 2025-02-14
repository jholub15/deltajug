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
 * David Guthrie
 */

#include <prefix/dtdalprefix-src.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4267) // for warning C4267: 'argument' : conversion from 'size_t' to 'const unsigned int', possible loss of data
#endif

#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/internal/XMLGrammarPoolImpl.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#include <osgDB/FileNameUtils>

#include <dtCore/globals.h>
#include <dtCore/transformable.h>
#include <dtCore/transform.h>

#include <dtDAL/mapxml.h>
#include <dtDAL/map.h>
#include <dtDAL/exceptionenum.h>
#include <dtDAL/enginepropertytypes.h>
#include <dtDAL/groupactorproperty.h>
#include <dtDAL/arrayactorpropertybase.h>
#include <dtDAL/containeractorproperty.h>
#include <dtDAL/actorproperty.h>
#include <dtDAL/actorproxy.h>
#include <dtDAL/actortype.h>
#include <dtDAL/datatype.h>
#include <dtDAL/gameevent.h>
#include <dtDAL/gameeventmanager.h>
#include <dtDAL/namedparameter.h>
#include <dtDAL/mapxmlconstants.h>
#include <dtDAL/mapcontenthandler.h>
#include <dtDAL/transformableactorproxy.h>

#include <dtUtil/fileutils.h>
#include <dtUtil/datetime.h>
#include <dtUtil/xercesutils.h>
#include <dtUtil/log.h>

#include <iostream>

XERCES_CPP_NAMESPACE_USE

namespace dtDAL
{

   static const std::string logName("mapxml.cpp");

   /////////////////////////////////////////////////////////////////

   void MapParser::StaticInit()
   {
      try
      {
         XMLPlatformUtils::Initialize();
      }
      catch (const XMLException& toCatch)
      {
         //if this happens, something is very very wrong.
         char* message = XMLString::transcode( toCatch.getMessage() );
         std::string msg(message);
         LOG_ERROR("Error during parser initialization!: "+ msg)
            XMLString::release( &message );
         return;
      }
   }

   /////////////////////////////////////////////////////////////////

   void MapParser::StaticShutdown()
   {
      //This causes too many problems and it in only called at app shutdown
      //so the memory leak in not a problem.
      //XMLPlatformUtils::Terminate();
   }

   /////////////////////////////////////////////////////////////////

   Map* MapParser::Parse(const std::string& path)
   {
      try
      {
         mParsing = true;
         mHandler->SetMapMode();
         mXercesParser->setContentHandler(mHandler.get());
         mXercesParser->setErrorHandler(mHandler.get());
         mXercesParser->parse(path.c_str());
         mLogger->LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__,  __LINE__, "Parsing complete.\n");
         dtCore::RefPtr<Map> mapRef = mHandler->GetMap();
         mHandler->ClearMap();
         mParsing = false;
         return mapRef.release();
      }
      catch (const OutOfMemoryException&)
      {
         mParsing = false;
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__,  __LINE__, "Ran out of memory parsing!");
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Ran out of memory parsing save file.", __FILE__, __LINE__);
      }
      catch (const XMLException& toCatch)
      {
         mParsing = false;
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__,  __LINE__, "Error during parsing! %ls :\n",
                             toCatch.getMessage());
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Error while parsing map file. See log for more information.", __FILE__, __LINE__);
      }
      catch (const SAXParseException&)
      {
         mParsing = false;
         //this will already by logged by the
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Error while parsing map file. See log for more information.", __FILE__, __LINE__);
      }
      return NULL;
   }

   /////////////////////////////////////////////////////////////////

   bool MapParser::ParsePrefab(const std::string& path, std::vector<dtCore::RefPtr<dtDAL::ActorProxy> >& proxyList)
   {
      try
      {
         mParsing = true;
         mHandler->SetPrefabMode(proxyList);
         mXercesParser->setContentHandler(mHandler.get());
         mXercesParser->setErrorHandler(mHandler.get());
         mXercesParser->parse(path.c_str());
         mLogger->LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__,  __LINE__, "Parsing complete.\n");
         mHandler->ClearMap();
         mParsing = false;
         return true;
      }
      catch (const OutOfMemoryException&)
      {
         mParsing = false;
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__,  __LINE__, "Ran out of memory parsing!");
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Ran out of memory parsing save file.", __FILE__, __LINE__);
      }
      catch (const XMLException& toCatch)
      {
         mParsing = false;
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__,  __LINE__, "Error during parsing! %ls :\n",
            toCatch.getMessage());
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Error while parsing map file. See log for more information.", __FILE__, __LINE__);
      }
      catch (const SAXParseException&)
      {
         mParsing = false;
         //this will already by logged by the
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Error while parsing map file. See log for more information.", __FILE__, __LINE__);
      }
      return false;
   }

   ///////////////////////////////////////////////////////////////////////////////
   const std::string MapParser::GetPrefabIconFileName(const std::string& path)
   {
      std::vector<dtCore::RefPtr<dtDAL::ActorProxy> > proxyList; //just an empty list
      std::string iconFileName = "";

      mParsing = true;
      mHandler->SetPrefabMode(proxyList, MapContentHandler::PREFAB_ICON_ONLY);
      mXercesParser->setContentHandler(mHandler.get());
      mXercesParser->setErrorHandler(mHandler.get());

      try
      {
         mXercesParser->parse(path.c_str());
      }
      catch(dtUtil::Exception iconFoundWeAreDone)
      {
         //Probably the icon has been found, the exception to stop parsing has
         //been thrown, so there's nothing to do here.  
      }
      
      iconFileName = mHandler->GetPrefabIconFileName();
   
      mHandler->ClearMap();      
      mParsing = false;

      return iconFileName;
   }
   
   /////////////////////////////////////////////////////////////////
   const std::string MapParser::ParseMapName(const std::string& path)
   {
      //this is a flag that will make sure
      //the parser gets reset if an exception is thrown.
      bool parserNeedsReset = false;
      XMLPScanToken token;
      try
      {
         mXercesParser->setContentHandler(mHandler.get());
         mXercesParser->setErrorHandler(mHandler.get());

         if (mXercesParser->parseFirst(path.c_str(), token))
         {
            parserNeedsReset = true;

            bool cont = mXercesParser->parseNext(token);
            while (cont && !mHandler->HasFoundMapName())
            {
               cont = mXercesParser->parseNext(token);
            }

            parserNeedsReset = false;
            //reSet the parser and close the file handles.
            mXercesParser->parseReset(token);

            if (mHandler->HasFoundMapName())
            {
               mLogger->LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__,  __LINE__, "Parsing complete.");
               std::string name = mHandler->GetMap()->GetName();
               mHandler->ClearMap();
               return name;
            }
            else
            {
               throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Parser stopped without finding the map name.", __FILE__, __LINE__);
            }
         }
         else
         {
            throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Parsing to find the map name did not begin.", __FILE__, __LINE__);
         }
      }
      catch (const OutOfMemoryException&)
      {
         if (parserNeedsReset)
            mXercesParser->parseReset(token);

         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__,  __LINE__, "Ran out of memory parsing!");
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Ran out of memory parsing save file.", __FILE__, __LINE__);
      }
      catch (const XMLException& toCatch)
      {
         if (parserNeedsReset)
            mXercesParser->parseReset(token);

         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__,  __LINE__, "Error during parsing! %ls :\n",
                             toCatch.getMessage());
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Error while parsing map file. See log for more information.", __FILE__, __LINE__);
      }
      catch (const SAXParseException&)
      {
         if (parserNeedsReset)
            mXercesParser->parseReset(token);

         //this will already by logged by the content handler
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapLoadParsingError, "Error while parsing map file. See log for more information.", __FILE__, __LINE__);
      }
   }

   /////////////////////////////////////////////////////////////////
   Map* MapParser::GetMapBeingParsed()
   {
      if (!IsParsing())
      {
         return NULL;
      }

      return mHandler->GetMap();
   }

   /////////////////////////////////////////////////////////////////
   const Map* MapParser::GetMapBeingParsed() const
   {
      if (!IsParsing())
      {
         return NULL;
      }

      return mHandler->GetMap();
   }

   /////////////////////////////////////////////////////////////////
   MapParser::MapParser()
      : mHandler(new MapContentHandler())
      , mParsing(false)
   {
      mLogger = &dtUtil::Log::GetInstance(logName);

      mXercesParser = XMLReaderFactory::createXMLReader();

      mXercesParser->setFeature(XMLUni::fgSAX2CoreValidation, true);
      mXercesParser->setFeature(XMLUni::fgXercesDynamic, false);

      mXercesParser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
      mXercesParser->setFeature(XMLUni::fgXercesSchema, true);
      mXercesParser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
      mXercesParser->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, true);
      mXercesParser->setFeature(XMLUni::fgXercesUseCachedGrammarInParse, true);
      mXercesParser->setFeature(XMLUni::fgXercesCacheGrammarFromParse, true);

      std::string schemaFileName = dtCore::FindFileInPathList("map.xsd");

      if (!dtUtil::FileUtils::GetInstance().FileExists(schemaFileName))
      {
         throw dtUtil::Exception(dtDAL::ExceptionEnum::ProjectException, "Unable to load required file \"map.xsd\", can not load map.", __FILE__, __LINE__);
      }

      XMLCh* value = XMLString::transcode(schemaFileName.c_str());
      LocalFileInputSource inputSource(value);
      //cache the schema
      mXercesParser->loadGrammar(inputSource, Grammar::SchemaGrammarType, true);
      XMLString::release(&value);
   }

   /////////////////////////////////////////////////////////////////

   MapParser::~MapParser()
   {
      delete mXercesParser;
   }

   /////////////////////////////////////////////////////////////////

   const std::set<std::string>& MapParser::GetMissingActorTypes()
   {
      return mHandler->GetMissingActorTypes();
   }

   /////////////////////////////////////////////////////////////////

   const std::vector<std::string>& MapParser::GetMissingLibraries()
   {
      return mHandler->GetMissingLibraries();
   }

   ////////////////////////////////////////////////////////////////////////////////
   bool MapParser::HasDeprecatedProperty() const
   {
      return mHandler->HasDeprecatedProperty();
   }

   /////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////

   MapWriter::MapFormatTarget::MapFormatTarget(): mOutFile(NULL)
   {
      mLogger = &dtUtil::Log::GetInstance(logName);
   }

   /////////////////////////////////////////////////////////////////

   MapWriter::MapFormatTarget::~MapFormatTarget()
   {
      SetOutputFile(NULL);
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::MapFormatTarget::SetOutputFile(FILE* newFile)
   {
      if (mOutFile != NULL)
         fclose(mOutFile);

      mOutFile = newFile;
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::MapFormatTarget::writeChars(
      const XMLByte* const toWrite,
      const unsigned int count,
      xercesc::XMLFormatter* const formatter)
   {
      if (mOutFile != NULL)
      {
         size_t size = fwrite((char *) toWrite, sizeof(char), (size_t)count, mOutFile);
         if (size < (size_t)count)
         {
            mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__,
                                "Error writing to file.  Write count less than expected.");
         }

         //fflush(mOutFile);
      }
      else
      {
         XERCES_STD_QUALIFIER cout.write((char *) toWrite, (int) count);
         XERCES_STD_QUALIFIER cout.flush();
      }
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::MapFormatTarget::flush()
   {
      if (mOutFile != NULL)
      {
         fflush(mOutFile);
      }
      else
      {
         XERCES_STD_QUALIFIER cout.flush();
      }
   }

   //////////////////////////////////////////////////

   MapWriter::MapWriter():
      mFormatter("UTF-8", NULL, &mFormatTarget, XMLFormatter::NoEscapes, XMLFormatter::DefaultUnRep)
   {
      mLogger = &dtUtil::Log::GetInstance(logName);
   }
   /////////////////////////////////////////////////////////////////


   MapWriter::~MapWriter()
   {
   }

   /////////////////////////////////////////////////////////////////

   template <typename VecType>
   void MapWriter::WriteVec(const VecType& vec, char* numberConversionBuffer, const size_t bufferMax)
   {
      switch (VecType::num_components) {
      case 2:
         BeginElement(MapXMLConstants::ACTOR_PROPERTY_VEC2_ELEMENT);
         break;
      case 3:
         BeginElement(MapXMLConstants::ACTOR_PROPERTY_VEC3_ELEMENT);
         break;
      case 4:
         BeginElement(MapXMLConstants::ACTOR_PROPERTY_VEC4_ELEMENT);
         break;
      default:
         //LOG error
         return;
      }

      BeginElement(MapXMLConstants::ACTOR_VEC_1_ELEMENT);
      snprintf(numberConversionBuffer, bufferMax, "%f", vec[0]);
      AddCharacters(numberConversionBuffer);
      EndElement();

      BeginElement(MapXMLConstants::ACTOR_VEC_2_ELEMENT);
      snprintf(numberConversionBuffer, bufferMax, "%f", vec[1]);
      AddCharacters(numberConversionBuffer);
      EndElement();

      if (VecType::num_components > 2)
      {
         BeginElement(MapXMLConstants::ACTOR_VEC_3_ELEMENT);
         snprintf(numberConversionBuffer, bufferMax, "%f", vec[2]);
         AddCharacters(numberConversionBuffer);
         EndElement();
      }

      if (VecType::num_components > 3)
      {
         BeginElement(MapXMLConstants::ACTOR_VEC_4_ELEMENT);
         snprintf(numberConversionBuffer, bufferMax, "%f", vec[3]);
         AddCharacters(numberConversionBuffer);
         EndElement();
      }

      EndElement();
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::Save(Map& map, const std::string& filePath)
   {
      FILE* outfile = fopen(filePath.c_str(), "w");

      if (outfile == NULL)
      {
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapSaveError, std::string("Unable to open map file \"") + filePath + "\" for writing.", __FILE__, __LINE__);
      }

      mFormatTarget.SetOutputFile(outfile);

      try {

         mFormatter << MapXMLConstants::BEGIN_XML_DECL << mFormatter.getEncodingName() << MapXMLConstants::END_XML_DECL << chLF;

         const std::string& utcTime = dtUtil::DateTime::ToString(dtUtil::DateTime(dtUtil::DateTime::TimeOrigin::LOCAL_TIME),
            dtUtil::DateTime::TimeFormat::CALENDAR_DATE_AND_TIME_FORMAT);

         BeginElement(MapXMLConstants::MAP_ELEMENT, MapXMLConstants::MAP_NAMESPACE);
         BeginElement(MapXMLConstants::HEADER_ELEMENT);
         BeginElement(MapXMLConstants::MAP_NAME_ELEMENT);
         AddCharacters(map.GetName());
         EndElement(); // End Map Name Element.
         BeginElement(MapXMLConstants::DESCRIPTION_ELEMENT);
         AddCharacters(map.GetDescription());
         EndElement(); // End Description Element.
         BeginElement(MapXMLConstants::AUTHOR_ELEMENT);
         AddCharacters(map.GetAuthor());
         EndElement(); // End Author Element.
         BeginElement(MapXMLConstants::COMMENT_ELEMENT);
         AddCharacters(map.GetComment());
         EndElement(); // End Comment Element.
         BeginElement(MapXMLConstants::COPYRIGHT_ELEMENT);
         AddCharacters(map.GetCopyright());
         EndElement(); // End Copyright Element.
         BeginElement(MapXMLConstants::CREATE_TIMESTAMP_ELEMENT);
         if (map.GetCreateDateTime().length() == 0)
         {
            map.SetCreateDateTime(utcTime);
         }
         AddCharacters(map.GetCreateDateTime());
         EndElement(); // End Create Timestamp Element.
         BeginElement(MapXMLConstants::LAST_UPDATE_TIMESTAMP_ELEMENT);
         AddCharacters(utcTime);
         EndElement(); // End Last Update Timestamp Element
         BeginElement(MapXMLConstants::EDITOR_VERSION_ELEMENT);
         AddCharacters(std::string(MapXMLConstants::EDITOR_VERSION));
         EndElement(); // End Editor Version Element.
         BeginElement(MapXMLConstants::SCHEMA_VERSION_ELEMENT);
         AddCharacters(std::string(MapXMLConstants::SCHEMA_VERSION));
         EndElement(); // End Scema Version Element.         
         EndElement(); // End Header Element.

         BeginElement(MapXMLConstants::LIBRARIES_ELEMENT);
         const std::vector<std::string>& libs = map.GetAllLibraries();
         for (std::vector<std::string>::const_iterator i = libs.begin(); i != libs.end(); ++i)
         {
            BeginElement(MapXMLConstants::LIBRARY_ELEMENT);
            BeginElement(MapXMLConstants::LIBRARY_NAME_ELEMENT);
            AddCharacters(*i);
            EndElement(); // End Library Name Element.
            BeginElement(MapXMLConstants::LIBRARY_VERSION_ELEMENT);
            AddCharacters(map.GetLibraryVersion(*i));
            EndElement(); // End Library Version Element.
            EndElement(); // End Library Element.
         }
         EndElement(); // End Libraries Element.

         std::vector<GameEvent* > events;
         map.GetEventManager().GetAllEvents(events);
         if (!events.empty())
         {
            BeginElement(MapXMLConstants::EVENTS_ELEMENT);
            for (std::vector<GameEvent* >::const_iterator i = events.begin(); i != events.end(); ++i)
            {
               BeginElement(MapXMLConstants::EVENT_ELEMENT);
               BeginElement(MapXMLConstants::EVENT_ID_ELEMENT);
               AddCharacters((*i)->GetUniqueId().ToString());
               EndElement(); // End ID Element.
               BeginElement(MapXMLConstants::EVENT_NAME_ELEMENT);
               AddCharacters((*i)->GetName());
               EndElement(); // End Event Name Element.
               BeginElement(MapXMLConstants::EVENT_DESCRIPTION_ELEMENT);
               AddCharacters((*i)->GetDescription());
               EndElement(); // End Event Description Element.
               EndElement(); // End Event Element.
            }
            EndElement(); // End Events Element.
         }

         BeginElement(MapXMLConstants::ACTORS_ELEMENT);

         if (map.GetEnvironmentActor() != NULL)
         {
            ActorProxy &proxy = *map.GetEnvironmentActor();
            BeginElement(MapXMLConstants::ACTOR_ENVIRONMENT_ACTOR_ELEMENT);
            AddCharacters(proxy.GetId().ToString());
            EndElement(); // End Actor Environment Actor Element.
         }

         const std::map<dtCore::UniqueId, dtCore::RefPtr<ActorProxy> >& proxies = map.GetAllProxies();
         for (std::map<dtCore::UniqueId, dtCore::RefPtr<ActorProxy> >::const_iterator i = proxies.begin();
              i != proxies.end(); i++)
         {
            const ActorProxy& proxy = *i->second.get();
            //printf("Proxy pointer %x\n", &proxy);
            //printf("Actor pointer %x\n", proxy.getActor());

            //ghost proxies arent saved
            //added 7/10/06 -banderegg
            if (proxy.IsGhostProxy())
               continue;

            BeginElement(MapXMLConstants::ACTOR_ELEMENT);
            BeginElement(MapXMLConstants::ACTOR_TYPE_ELEMENT);
            AddCharacters(proxy.GetActorType().GetFullName());
            EndElement(); // End Actor Type Element.
            BeginElement(MapXMLConstants::ACTOR_ID_ELEMENT);
            AddCharacters(proxy.GetId().ToString());
            EndElement(); // End Actor ID Element.
            BeginElement(MapXMLConstants::ACTOR_NAME_ELEMENT);
            AddCharacters(proxy.GetName());
            if (mLogger->IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
            {
               mLogger->LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                                   "Found Proxy Named: %s", proxy.GetName().c_str());
            }
            EndElement(); // End Actor Name Element.
            std::vector<const ActorProperty*> propList;
            proxy.GetPropertyList(propList);
            //int x = 0;
            for (std::vector<const ActorProperty*>::const_iterator i = propList.begin();
                 i != propList.end(); ++i)
            {
               //printf("Printing actor property number %d", x++);
               const ActorProperty& property = *(*i);

               // If the property is read only, skip it
               if (property.IsReadOnly())
                  continue;

               WriteProperty(property);

            }
            EndElement(); // End Actor Element.
         }
         EndElement(); // End Actors Element

         BeginElement(MapXMLConstants::ACTOR_GROUPS_ELEMENT);
         {
            int groupCount = map.GetGroupCount();
            for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
            {
               BeginElement(MapXMLConstants::ACTOR_GROUP_ELEMENT);

               int actorCount = map.GetGroupActorCount(groupIndex);
               for (int actorIndex = 0; actorIndex < actorCount; actorIndex++)
               {
                  dtDAL::ActorProxy* proxy = map.GetActorFromGroup(groupIndex, actorIndex);
                  if (proxy)
                  {
                     BeginElement(MapXMLConstants::ACTOR_GROUP_ACTOR_ELEMENT);
                     AddCharacters(proxy->GetId().ToString());
                     EndElement(); // End Groups Actor Size Element.
                  }
               }

               EndElement(); // End Group Element.
            }
         }
         EndElement(); // End Groups Element.

         BeginElement(MapXMLConstants::PRESET_CAMERAS_ELEMENT);
         {
            char numberConversionBuffer[80];

            for (int presetIndex = 0; presetIndex < 10; presetIndex++)
            {
               // Skip elements that are invalid.
               Map::PresetCameraData data = map.GetPresetCameraData(presetIndex);
               if (!data.isValid)
               {
                  continue;
               }

               BeginElement(MapXMLConstants::PRESET_CAMERA_ELEMENT);
               {
                  BeginElement(MapXMLConstants::PRESET_CAMERA_INDEX_ELEMENT);
                  snprintf(numberConversionBuffer, 80, "%d", presetIndex);
                  AddCharacters(numberConversionBuffer);
                  EndElement(); // End Preset Camera Index Element.

                  BeginElement(MapXMLConstants::PRESET_CAMERA_PERSPECTIVE_VIEW_ELEMENT);
                  {
                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_X_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.persPosition.x());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position X Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Y_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.persPosition.y());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Y Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Z_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.persPosition.z());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Z Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_ROTATION_X_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.persRotation.x());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Rotation X Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_ROTATION_Y_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.persRotation.y());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Rotation Y Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_ROTATION_Z_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.persRotation.z());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Rotation Z Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_ROTATION_W_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.persRotation.w());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Rotation W Element.
                  }
                  EndElement(); // End Preset Camera Perspective View Element.

                  BeginElement(MapXMLConstants::PRESET_CAMERA_TOP_VIEW_ELEMENT);
                  {
                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_X_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.topPosition.x());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position X Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Y_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.topPosition.y());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Y Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Z_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.topPosition.z());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Z Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_ZOOM_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.topZoom);
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Zoom Element.
                  }
                  EndElement(); // End Preset Camera Top View Element;

                  BeginElement(MapXMLConstants::PRESET_CAMERA_SIDE_VIEW_ELEMENT);
                  {
                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_X_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.sidePosition.x());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position X Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Y_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.sidePosition.y());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Y Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Z_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.sidePosition.z());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Z Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_ZOOM_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.sideZoom);
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Zoom Element.
                  }
                  EndElement(); // End Preset Camera Side View Element;

                  BeginElement(MapXMLConstants::PRESET_CAMERA_FRONT_VIEW_ELEMENT);
                  {
                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_X_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.frontPosition.x());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position X Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Y_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.frontPosition.y());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Y Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_POSITION_Z_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.frontPosition.z());
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Position Z Element.

                     BeginElement(MapXMLConstants::PRESET_CAMERA_ZOOM_ELEMENT);
                     snprintf(numberConversionBuffer, 80, "%f", data.frontZoom);
                     AddCharacters(numberConversionBuffer);
                     EndElement(); // End Preset Camera Zoom Element.
                  }
                  EndElement(); // End Preset Camera Front View Element;
               }
               EndElement(); // End Preset Camera Element.
            }
         }
         EndElement(); // End Preset Camera Element.

         EndElement(); // End Map Element.

         //closes the file.
         mFormatTarget.SetOutputFile(NULL);
      }
      catch (dtUtil::Exception& ex)
      {
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__,
                             "Caught Exception \"%s\" while attempting to save map \"%s\".",
                             ex.What().c_str(), map.GetName().c_str());
         mFormatTarget.SetOutputFile(NULL);
         throw ex;
      }
      catch (...)
      {
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__,
                             "Unknown exception while attempting to save map \"%s\".",
                             map.GetName().c_str());
         mFormatTarget.SetOutputFile(NULL);
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapSaveError, std::string("Unknown exception saving map \"") + map.GetName() + ("\"."), __FILE__, __LINE__);
      }
   }

   ////////////////////////////////////////////////////////////////////////////////
   void MapWriter::SavePrefab(const std::vector<dtCore::RefPtr<ActorProxy> > proxyList,
                              const std::string& filePath, const std::string& description,
                              const std::string& iconFile /* = "" */)
   {
      FILE* outfile = fopen(filePath.c_str(), "w");

      if (outfile == NULL)
      {
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapSaveError, std::string("Unable to open map file \"") + filePath + "\" for writing.", __FILE__, __LINE__);
      }

      mFormatTarget.SetOutputFile(outfile);

      try
      {
         mFormatter << MapXMLConstants::BEGIN_XML_DECL << mFormatter.getEncodingName() << MapXMLConstants::END_XML_DECL << chLF;

         //const std::string& utcTime = dtUtil::DateTime::ToString(dtUtil::DateTime(dtUtil::DateTime::TimeOrigin::LOCAL_TIME),
            //dtUtil::DateTime::TimeFormat::CALENDAR_DATE_AND_TIME_FORMAT);

         BeginElement(MapXMLConstants::PREFAB_ELEMENT, MapXMLConstants::PREFAB_NAMESPACE);

         BeginElement(MapXMLConstants::HEADER_ELEMENT);

         BeginElement(MapXMLConstants::DESCRIPTION_ELEMENT);
         AddCharacters(description);
         EndElement(); // End Description Element.
         BeginElement(MapXMLConstants::EDITOR_VERSION_ELEMENT);
         AddCharacters(std::string(MapXMLConstants::EDITOR_VERSION));
         EndElement(); // End Editor Version Element.
         BeginElement(MapXMLConstants::SCHEMA_VERSION_ELEMENT);
         AddCharacters(std::string(MapXMLConstants::SCHEMA_VERSION));
         EndElement(); // End Schema Version Element.
         BeginElement(MapXMLConstants::ICON_ELEMENT);
         AddCharacters(iconFile);
         EndElement(); //End Icon Element

         EndElement(); // End Header Element.

         osg::Vec3 origin;
         bool originSet = false;
         BeginElement(MapXMLConstants::ACTORS_ELEMENT);
         for (int proxyIndex = 0; proxyIndex < (int)proxyList.size(); proxyIndex++)
         {
            ActorProxy* proxy = proxyList[proxyIndex].get();

            // We can't do anything without a proxy.
            if (!proxy)
            {
               continue;
            }

            //ghost proxies arent saved
            if (proxy->IsGhostProxy())
               continue;

            // If this is the first proxy, store the translation as the origin.
            dtDAL::TransformableActorProxy* tProxy = dynamic_cast<dtDAL::TransformableActorProxy*>(proxy);
            if (tProxy)
            {
               if (!originSet)
               {
                  origin = tProxy->GetTranslation();
                  originSet = true;
               }

               tProxy->SetTranslation(tProxy->GetTranslation() - origin);
            }

            BeginElement(MapXMLConstants::ACTOR_ELEMENT);
            BeginElement(MapXMLConstants::ACTOR_TYPE_ELEMENT);
            AddCharacters(proxy->GetActorType().GetFullName());
            EndElement(); // End Actor Type Element.
            BeginElement(MapXMLConstants::ACTOR_ID_ELEMENT);
            AddCharacters(proxy->GetId().ToString());
            EndElement(); // End Actor ID Element.
            BeginElement(MapXMLConstants::ACTOR_NAME_ELEMENT);
            AddCharacters(proxy->GetName());
            if (mLogger->IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
            {
               mLogger->LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                  "Found Proxy Named: %s", proxy->GetName().c_str());
            }
            EndElement(); // End Actor Name Element.
            std::vector<const ActorProperty*> propList;
            proxy->GetPropertyList(propList);
            //int x = 0;
            for (std::vector<const ActorProperty*>::const_iterator i = propList.begin();
               i != propList.end(); ++i)
            {
               //printf("Printing actor property number %d", x++);
               const ActorProperty& property = *(*i);

               // If the property is read only, skip it
               if (property.IsReadOnly())
                  continue;

               WriteProperty(property);
            }
            EndElement(); // End Actor Element.

            // Now undo the translation.
            if (tProxy && originSet)
            {
               tProxy->SetTranslation(tProxy->GetTranslation() + origin);
            }
         }
         EndElement(); // End Actors Element

         EndElement(); // End Prefab Element.

         //closes the file.
         mFormatTarget.SetOutputFile(NULL);
      }
      catch (dtUtil::Exception& ex)
      {
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__,
            "Caught Exception \"%s\" while attempting to save prefab \"%s\".",
            ex.What().c_str(), filePath.c_str());
         mFormatTarget.SetOutputFile(NULL);
         throw ex;
      }
      catch (...)
      {
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__,
            "Unknown exception while attempting to save prefab \"%s\".",
            filePath.c_str());
         mFormatTarget.SetOutputFile(NULL);
         throw dtUtil::Exception(dtDAL::ExceptionEnum::MapSaveError, std::string("Unknown exception saving map \"") + filePath + ("\"."), __FILE__, __LINE__);
      }
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::WriteParameter(const NamedParameter& parameter)
   {
      const size_t bufferMax = 512;
      char numberConversionBuffer[bufferMax];
      const DataType& dataType = parameter.GetDataType();

      BeginElement(MapXMLConstants::ACTOR_PROPERTY_PARAMETER_ELEMENT);

      BeginElement(MapXMLConstants::ACTOR_PROPERTY_NAME_ELEMENT);
      AddCharacters(parameter.GetName());
      if (mLogger->IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
      {
         mLogger->LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                             "Found Parameter of GroupActorProperty Named: %s", parameter.GetName().c_str());
      }
      EndElement();

      switch (dataType.GetTypeId())
      {
         case DataType::FLOAT_ID:
         case DataType::DOUBLE_ID:
         case DataType::INT_ID:
         case DataType::LONGINT_ID:
         case DataType::STRING_ID:
         case DataType::BOOLEAN_ID:
         case DataType::ACTOR_ID:
         case DataType::GAMEEVENT_ID:
         case DataType::ENUMERATION_ID:
            WriteSimple(parameter);
            break;
         case DataType::VEC2_ID:
            WriteVec(static_cast<const NamedVec2Parameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC2F_ID:
            WriteVec(static_cast<const NamedVec2fParameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC2D_ID:
            WriteVec(static_cast<const NamedVec2dParameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC3_ID:
            WriteVec(static_cast<const NamedVec3Parameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC3F_ID:
            WriteVec(static_cast<const NamedVec3fParameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC3D_ID:
            WriteVec(static_cast<const NamedVec3dParameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC4_ID:
            WriteVec(static_cast<const NamedVec4Parameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC4F_ID:
            WriteVec(static_cast<const NamedVec4fParameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC4D_ID:
            WriteVec(static_cast<const NamedVec4dParameter&>(parameter).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::RGBACOLOR_ID:
            WriteColorRGBA(static_cast<const NamedRGBAColorParameter&>(parameter), numberConversionBuffer, bufferMax);
            break;
         case DataType::GROUP_ID:
         {
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_GROUP_ELEMENT);
            std::vector<const NamedParameter*> parameters;
            static_cast<const NamedGroupParameter&>(parameter).GetParameters(parameters);
            for (size_t i = 0; i < parameters.size(); ++i)
            {
               WriteParameter(*parameters[i]);
            }
            EndElement();
            break;
         }
         case DataType::ARRAY_ID:
         {
            // BeginElement(MapXMLConstants::ACTOR_PROPERTY_ARRAY_ELEMENT);
            // TODO ARRAY: Save an array that was part of a group.
            break;
         }
         case DataType::CONTAINER_ID:
         {
            //BeginElement(MapXMLConstants::ACTOR_PROPERTY_CONTAINER_ELEMENT);
            // TODO CONTAINER: Save a container that was part of a group.
            break;
         }
         default:
         {
            if (dataType.IsResource())
            {
               const NamedResourceParameter& p =
                  static_cast<const NamedResourceParameter&>(parameter);

               const ResourceDescriptor* rd = p.GetValue();

               BeginElement(MapXMLConstants::ACTOR_PROPERTY_RESOURCE_TYPE_ELEMENT);
               AddCharacters(parameter.GetDataType().GetName());
               EndElement();

               BeginElement(MapXMLConstants::ACTOR_PROPERTY_RESOURCE_DISPLAY_ELEMENT);
               if (rd != NULL)
                  AddCharacters(rd->GetDisplayName());
               EndElement();
               BeginElement(MapXMLConstants::ACTOR_PROPERTY_RESOURCE_IDENTIFIER_ELEMENT);
               if (rd != NULL)
                  AddCharacters(rd->GetResourceIdentifier());
               EndElement();
            }
            else
            {
               mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__,
                                    "Unhandled datatype in MapWriter: %s.",
                                    dataType.GetName().c_str());
            }
         }
      }
      //End the parameter element.
      EndElement();
   }

   /////////////////////////////////////////////////////////////////

   template <typename Type>
   void MapWriter::WriteColorRGBA(const Type& holder, char* numberConversionBuffer, size_t bufferMax)
   {
      osg::Vec4f val = holder.GetValue();

      BeginElement(MapXMLConstants::ACTOR_PROPERTY_COLOR_RGBA_ELEMENT);

      BeginElement(MapXMLConstants::ACTOR_COLOR_R_ELEMENT);
      snprintf(numberConversionBuffer, bufferMax, "%f", val[0]);
      AddCharacters(numberConversionBuffer);
      EndElement();

      BeginElement(MapXMLConstants::ACTOR_COLOR_G_ELEMENT);
      snprintf(numberConversionBuffer, bufferMax, "%f", val[1]);
      AddCharacters(numberConversionBuffer);
      EndElement();

      BeginElement(MapXMLConstants::ACTOR_COLOR_B_ELEMENT);
      snprintf(numberConversionBuffer, bufferMax, "%f", val[2]);
      AddCharacters(numberConversionBuffer);
      EndElement();

      BeginElement(MapXMLConstants::ACTOR_COLOR_A_ELEMENT);
      snprintf(numberConversionBuffer, bufferMax, "%f", val[3]);
      AddCharacters(numberConversionBuffer);
      EndElement();

      EndElement();
   }

   ////////////////////////////////////////////////////////////////////////////////
   void MapWriter::WriteArray(const ArrayActorPropertyBase& arrayProp, char* numberConversionBuffer, size_t bufferMax)
   {
      BeginElement(MapXMLConstants::ACTOR_PROPERTY_ARRAY_ELEMENT);

      BeginElement(MapXMLConstants::ACTOR_ARRAY_SIZE_ELEMENT);
      int arraySize = arrayProp.GetArraySize();
      snprintf(numberConversionBuffer, bufferMax, "%d", arraySize);
      AddCharacters(numberConversionBuffer);
      EndElement();

      // Save out the data for each index.
      for (int index = 0; index < arraySize; index++)
      {
         // Save out the index number.
         BeginElement(MapXMLConstants::ACTOR_ARRAY_ELEMENT);

         BeginElement(MapXMLConstants::ACTOR_ARRAY_INDEX_ELEMENT);
         snprintf(numberConversionBuffer, bufferMax, "%d", index);
         AddCharacters(numberConversionBuffer);
         EndElement();

         // Write the data for the current property.
         arrayProp.SetIndex(index);
         WriteProperty(*arrayProp.GetArrayProperty());

         EndElement();
      }

      EndElement();
   }

   ////////////////////////////////////////////////////////////////////////////////
   void MapWriter::WriteContainer(const ContainerActorProperty& arrayProp, char* numberConversionBuffer, size_t bufferMax)
   {
      BeginElement(MapXMLConstants::ACTOR_PROPERTY_CONTAINER_ELEMENT);

      // Save out the data for each index.
      for (int index = 0; index < arrayProp.GetPropertyCount(); index++)
      {
         // Write the data for the current property.
         WriteProperty(*arrayProp.GetProperty(index));
      }

      EndElement();
   }

   /////////////////////////////////////////////////////////////////
   void MapWriter::WriteSimple(const AbstractParameter& holder)
   {
      switch (holder.GetDataType().GetTypeId())
      {
         case DataType::FLOAT_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_FLOAT_ELEMENT);
            break;
         case DataType::DOUBLE_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_DOUBLE_ELEMENT);
            break;
         case DataType::INT_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_INTEGER_ELEMENT);
            break;
         case DataType::LONGINT_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_LONG_ELEMENT);
            break;
         case DataType::STRING_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_STRING_ELEMENT);
            break;
         case DataType::BOOLEAN_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_BOOLEAN_ELEMENT);
            break;
         case DataType::ACTOR_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_ACTOR_ID_ELEMENT);
            break;
         case DataType::GAMEEVENT_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_GAMEEVENT_ELEMENT);
            break;
         case DataType::ENUMERATION_ID:
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_ENUM_ELEMENT);
            break;
         default:
            //LOG ERROR
            return;
      }
      AddCharacters(holder.ToString());
      EndElement();
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::WriteProperty(const ActorProperty& property)
   {
      const size_t bufferMax = 512;
      char numberConversionBuffer[bufferMax];
      const DataType& propertyType = property.GetDataType();

      BeginElement(MapXMLConstants::ACTOR_PROPERTY_ELEMENT);

      BeginElement(MapXMLConstants::ACTOR_PROPERTY_NAME_ELEMENT);
      AddCharacters(property.GetName());
      if (mLogger->IsLevelEnabled(dtUtil::Log::LOG_DEBUG))
      {
         mLogger->LogMessage(dtUtil::Log::LOG_DEBUG, __FUNCTION__, __LINE__,
                             "Found Property Named: %s", property.GetName().c_str());
      }
      EndElement();

      switch (propertyType.GetTypeId())
      {
         case DataType::FLOAT_ID:
         case DataType::DOUBLE_ID:
         case DataType::INT_ID:
         case DataType::LONGINT_ID:
         case DataType::STRING_ID:
         case DataType::BOOLEAN_ID:
         case DataType::ACTOR_ID:
         case DataType::GAMEEVENT_ID:
         case DataType::ENUMERATION_ID:
            WriteSimple(property);
            break;
         case DataType::VEC2_ID:
            WriteVec(static_cast<const Vec2ActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC2F_ID:
            WriteVec(static_cast<const Vec2fActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC2D_ID:
            WriteVec(static_cast<const Vec2dActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC3_ID:
            WriteVec(static_cast<const Vec3ActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC3F_ID:
            WriteVec(static_cast<const Vec3fActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC3D_ID:
            WriteVec(static_cast<const Vec3dActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC4_ID:
            WriteVec(static_cast<const Vec4ActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC4F_ID:
            WriteVec(static_cast<const Vec4fActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::VEC4D_ID:
            WriteVec(static_cast<const Vec4dActorProperty&>(property).GetValue(), numberConversionBuffer, bufferMax);
            break;
         case DataType::RGBACOLOR_ID:
            WriteColorRGBA(static_cast<const ColorRgbaActorProperty&>(property), numberConversionBuffer, bufferMax);
            break;
         case DataType::GROUP_ID:
         {
            BeginElement(MapXMLConstants::ACTOR_PROPERTY_GROUP_ELEMENT);
            dtCore::RefPtr<NamedGroupParameter> gp = static_cast<const GroupActorProperty&>(property).GetValue();
            if (gp.valid())
            {
               std::vector<const NamedParameter*> parameters;
               gp->GetParameters(parameters);
               for (size_t i = 0; i < parameters.size(); ++i)
               {
                  WriteParameter(*parameters[i]);
               }
            }
            EndElement();
            break;
         }
         case DataType::ARRAY_ID:
         {
            WriteArray(static_cast<const ArrayActorPropertyBase&>(property), numberConversionBuffer, bufferMax);
            break;
         }
         case DataType::CONTAINER_ID:
         {
            WriteContainer(static_cast<const ContainerActorProperty&>(property), numberConversionBuffer, bufferMax);
            break;
         }
         default:
         {
            if (propertyType.IsResource())
            {
               const ResourceActorProperty& p =
                  static_cast<const ResourceActorProperty&>(property);

               ResourceDescriptor* rd = p.GetValue();

               BeginElement(MapXMLConstants::ACTOR_PROPERTY_RESOURCE_TYPE_ELEMENT);
               AddCharacters(property.GetDataType().GetName());
               EndElement();

               BeginElement(MapXMLConstants::ACTOR_PROPERTY_RESOURCE_DISPLAY_ELEMENT);
               if (rd != NULL)
                  AddCharacters(rd->GetDisplayName());
               EndElement();
               BeginElement(MapXMLConstants::ACTOR_PROPERTY_RESOURCE_IDENTIFIER_ELEMENT);
               if (rd != NULL)
                  AddCharacters(rd->GetResourceIdentifier());
               EndElement();
            }
            else
            {
               mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__,
                                    "Unhandled datatype in MapWriter: %s.",
                                    propertyType.GetName().c_str());
            }
         }
      }

      //end property element
      EndElement();
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::BeginElement(const XMLCh* name, const XMLCh* attributes)
   {
      xmlCharString s(name);
      mElements.push(name);
      AddIndent();

      mFormatter << chOpenAngle << name;
      if (attributes != NULL)
         mFormatter << chSpace << attributes;

      mFormatter << chCloseAngle;
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::EndElement()
   {
      const xmlCharString& name = mElements.top();
      if (mLastCharWasLF)
         AddIndent();

      mFormatter << MapXMLConstants::END_XML_ELEMENT << name.c_str() << chCloseAngle << chLF;
      mLastCharWasLF = true;
      mElements.pop();
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::AddIndent()
   {
      if (!mLastCharWasLF)
         mFormatter << chLF;

      mLastCharWasLF = false;

      size_t indentCount = mElements.size() - 1;

      if (mElements.empty())
      {
         mLogger->LogMessage(dtUtil::Log::LOG_ERROR, __FUNCTION__, __LINE__, "Invalid end element when saving a map: ending with no beginning.");

         indentCount = 0;
      }
      for (size_t x = 0; x < indentCount; x++)
      {
         for (int y = 0; y < MapWriter::indentSize; y++)
            mFormatter << chSpace;
      }
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::AddCharacters(const xmlCharString& string)
   {
      mLastCharWasLF = false;
      mFormatter << string.c_str();
   }

   /////////////////////////////////////////////////////////////////

   void MapWriter::AddCharacters(const std::string& string)
   {
      mLastCharWasLF = false;
      XMLCh * stringX = XMLString::transcode(string.c_str());
      mFormatter << stringX;
      XMLString::release(&stringX);
   }
}
