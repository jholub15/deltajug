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
 * Curtiss Murphy
 */
#include <prefix/dtstageprefix-src.h>
#include <dtQt/dynamicfloatcontrol.h>
#include <dtQt/dynamicsubwidgets.h>
#include <dtDAL/actorproxy.h>
#include <dtDAL/actorproperty.h>
#include <dtDAL/datatype.h>
#include <dtDAL/enginepropertytypes.h>
#include <dtUtil/log.h>
#include <QtGui/QWidget>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>

namespace dtQt
{

   ///////////////////////////////////////////////////////////////////////////////
   DynamicFloatControl::DynamicFloatControl()
      : mTemporaryEditControl(NULL)
   {
   }

   /////////////////////////////////////////////////////////////////////////////////
   DynamicFloatControl::~DynamicFloatControl()
   {
   }


   /////////////////////////////////////////////////////////////////////////////////
   void DynamicFloatControl::InitializeData(DynamicAbstractControl* newParent,
      PropertyEditorModel* newModel, dtDAL::PropertyContainer* newPC, dtDAL::ActorProperty* newProperty)
   {
      // Note - We used to have dynamic_cast in here, but it was failing to properly cast in
      // all cases in Linux with gcc4.  So we replaced it with a static cast.
      if (newProperty != NULL && newProperty->GetDataType() == dtDAL::DataType::FLOAT)
      {
         mProperty = static_cast<dtDAL::FloatActorProperty*>(newProperty);
         DynamicAbstractControl::InitializeData(newParent, newModel, newPC, newProperty);
      }
      else
      {
         std::string propertyName = (newProperty != NULL) ? newProperty->GetName() : "NULL";
         LOG_ERROR("Cannot create dynamic control because property [" +
            propertyName + "] is not the correct type.");
      }
   }

   /////////////////////////////////////////////////////////////////////////////////
   void DynamicFloatControl::updateEditorFromModel(QWidget* widget)
   {
      if (widget != NULL && widget == mTemporaryEditControl)
      {
         SubQLineEdit* editBox = static_cast<SubQLineEdit*>(widget);

         // set the current value from our property
         float floatValue = mProperty->GetValue();
         editBox->setText(QString::number(floatValue, 'f', NUM_DECIMAL_DIGITS_FLOAT));
         editBox->selectAll();
      }
   }

   /////////////////////////////////////////////////////////////////////////////////
   bool DynamicFloatControl::updateModelFromEditor(QWidget* widget)
   {
      DynamicAbstractControl::updateModelFromEditor(widget);

      bool dataChanged = false;

      if (widget != NULL && widget == mTemporaryEditControl)
      {
         SubQLineEdit* editBox = static_cast<SubQLineEdit*>(widget);
         bool success = false;
         float result = editBox->text().toFloat(&success);

         // set our value to our object
         if (success)
         {
            // Save the data if they are different.  Note, we also need to compare the QString value,
            // else we get epsilon differences that cause the map to be marked dirty with no edits :(
            QString proxyValue = QString::number(mProperty->GetValue(), 'f', NUM_DECIMAL_DIGITS_FLOAT);
            QString newValue = editBox->text();
            if (result != mProperty->GetValue() && proxyValue != newValue)
            {
               // give undo manager the ability to create undo/redo events
               emit PropertyAboutToChange(*mPropContainer, *mProperty,
                  mProperty->ToString(), QString::number(result).toStdString());

               mProperty->SetValue(result);
               dataChanged = true;
            }
         }
         else
         {
            LOG_ERROR("updateData() failed to convert our value successfully");
         }

         // reselect all the text when we commit.
         // Gives the user visual feedback that something happened.
         editBox->selectAll();
      }

      // notify the world (mostly the viewports) that our property changed
      if (dataChanged)
      {
         emit PropertyChanged(*mPropContainer, *mProperty);
      }

      return dataChanged;
   }

   /////////////////////////////////////////////////////////////////////////////////
   QWidget *DynamicFloatControl::createEditor(QWidget* parent,
      const QStyleOptionViewItem& option, const QModelIndex& index)
   {
      // create and init the edit box
      mTemporaryEditControl = new SubQLineEdit(parent, this);
      QDoubleValidator* validator = new QDoubleValidator(mTemporaryEditControl);
      validator->setDecimals(NUM_DECIMAL_DIGITS_FLOAT);
      mTemporaryEditControl->setValidator(validator);

      if (!mInitialized)
      {
         LOG_ERROR("Tried to add itself to the parent widget before being initialized");
         return mTemporaryEditControl;
      }

      updateEditorFromModel(mTemporaryEditControl);

      mTemporaryEditControl->setToolTip(getDescription());

      return mTemporaryEditControl;
   }

   const QString DynamicFloatControl::getDisplayName()
   {
      return QString(tr(mProperty->GetLabel().c_str()));
   }

   const QString DynamicFloatControl::getDescription()
   {
      std::string tooltip = mProperty->GetDescription() + "  [Type: " +
         mProperty->GetDataType().GetName() + "]";
      return QString(tr(tooltip.c_str()));
   }

   const QString DynamicFloatControl::getValueAsString()
   {
      DynamicAbstractControl::getValueAsString();
      float floatValue = mProperty->GetValue();
      return QString::number(floatValue, 'f', NUM_DECIMAL_DIGITS_FLOAT);
   }

   bool DynamicFloatControl::isEditable()
   {
      return !mProperty->IsReadOnly();
   }

   /////////////////////////////////////////////////////////////////////////////////
   // SLOTS
   /////////////////////////////////////////////////////////////////////////////////

   bool DynamicFloatControl::updateData(QWidget* widget)
   {
      if (!mInitialized || widget == NULL)
      {
         LOG_ERROR("Tried to updateData before being initialized");
         return false;
      }

      return updateModelFromEditor(widget);
   }

   /////////////////////////////////////////////////////////////////////////////////
   void DynamicFloatControl::actorPropertyChanged(dtDAL::PropertyContainer& propCon,
            dtDAL::ActorProperty& property)
   {
      DynamicAbstractControl::actorPropertyChanged(propCon, property);

      if (mTemporaryEditControl != NULL && &propCon == mPropContainer && &property == mProperty)
      {
         updateEditorFromModel(mTemporaryEditControl);
      }
   }

} // namespace dtQt
