/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "viewdockareapopup.h"

// Qt
#include <QWidget>
#include <qevent.h>
#include <QLayout>
#include <qframe.h>
#include <QPushButton>
#include <QDateTime>

// KDE
#include <kdebug.h>
#include <kaction.h>
#include <kapplication.h>
#include <klocale.h>

// KMix
#include "mdwslider.h"
#include "mixer.h"
#include "kmixdockwidget.h"

// !! Do NOT remove or mask out "WType_Popup"
//    Users will not be able to close the Popup without opening the KMix main window then.
//    See Bug #93443, #96332 and #96404 for further details. -- esken
ViewDockAreaPopup::ViewDockAreaPopup(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof, KMixDockWidget *dockW )
      : ViewBase(parent, name, mixer, Qt::Popup | Qt::MSWindowsFixedSizeDialogHint , vflags, guiprof), _mdw(0), _dock(dockW)
{
    QBoxLayout *layout = new QHBoxLayout( this );
    _frame = new QFrame( this );
    layout->addWidget( _frame );

    _frame->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
    _frame->setLineWidth( 1 );

    _layoutMDW = new QGridLayout( _frame );
    _layoutMDW->setSpacing( 1 );
    _layoutMDW->setMargin( 2 );
    _layoutMDW->setObjectName( "KmixPopupLayout" );
    _hideTimer = new QTime();
    setMixSet();
}

ViewDockAreaPopup::~ViewDockAreaPopup() {
}



void ViewDockAreaPopup::mousePressEvent(QMouseEvent *)
{
    /**
       Hide the popup:
       This should work automatically, when the user clicks outside the bounds of this popup:
       Alas - it does not work.
       Why it does not work, I do not know: this->isPopup() returns "true", so Qt should
       properly take care of it in QWidget.
    */
    if ( !testAttribute(Qt::WA_UnderMouse) ) {
        _hideTimer->start();
        hide(); // needed!
    }
    return;
}

bool ViewDockAreaPopup::justHidden()
{
    return _hideTimer->elapsed() < 300;
}

void ViewDockAreaPopup::wheelEvent ( QWheelEvent * e ) {
   // Pass wheel event from "border widget" to child
   if ( _mdw != 0 ) {
      QApplication::sendEvent( _mdw, e);
   }
}

MixDevice* ViewDockAreaPopup::dockDevice()
{
    return _dockDevice;
}


void ViewDockAreaPopup::showContextMenu()
{
    // no right-button-menu on "dock area popup"
    return;
}


void ViewDockAreaPopup::setMixSet()
{
   // kDebug(67100) << "ViewDockAreaPopup::setMixSet()\n";
   _dockDevice = Mixer::getGlobalMasterMD();
   if ( _dockDevice == 0 ) {
      // If we have no dock device yet, we will take the first available mixer device
      if ( _mixer->size() > 0) {
         _dockDevice = (*_mixer)[0];
      }
   }
   if ( _dockDevice != 0 ) {
      _mixSet->append(_dockDevice);
   }
}

QWidget* ViewDockAreaPopup::add(MixDevice *md)
{
   _mdw = new MDWSlider(
      _mixer,       // the mixer for this device
      md,		  // only 1 device. This is actually _dockDevice
      true,         // Show Mute LED
      false,        // Show Record LED
                     false,        // Small
      Qt::Vertical, // Direction: only 1 device, so doesn't matter
      _frame,       // parent
      0             // Is "NULL", so that there is no RMB-popup
   );
   _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, 2 );
   _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, 0 );
   _layoutMDW->addWidget( _mdw, 0, 1 );

   // Add button to show main panel
   _showPanelBox = new QPushButton( i18n("Mixer"), _frame );
   _showPanelBox->setObjectName("MixerPanel");
   connect ( _showPanelBox, SIGNAL( clicked() ), SLOT( showPanelSlot() ) );
   _layoutMDW->addWidget( _showPanelBox, 1, 0, 1, 3 );
   
   return _mdw;
}

QSize ViewDockAreaPopup::sizeHint() const {
   //    kDebug(67100) << "ViewDockAreaPopup::sizeHint(): NewSize is " << _mdw->sizeHint() << "\n";
   return( _mdw->sizeHint() );
}

void ViewDockAreaPopup::constructionFinished() {
   //    kDebug(67100) << "ViewDockAreaPopup::constructionFinished()\n";
   if (_mdw != 0) {
      _mdw->move(0,0);
      _mdw->show();
      _mdw->resize(_mdw->sizeHint() );
   }
   resize(sizeHint());
}


void ViewDockAreaPopup::refreshVolumeLevels() {
   //    kDebug(67100) << "ViewDockAreaPopup::refreshVolumeLevels()\n";
   QWidget* mdw = _mdws.first();
   if ( mdw == 0 ) {
      kError(67100) << "ViewDockAreaPopup::refreshVolumeLevels(): mdw == 0\n";
      // sanity check (normally the lists are set up correctly)
   }
   else {
      if ( mdw->inherits("MDWSlider")) { // sanity check
            static_cast<MDWSlider*>(mdw)->update();
      }
      else {
         kError(67100) << "ViewDockAreaPopup::refreshVolumeLevels(): mdw is not slider\n";
         // no slider. Cannot happen in theory => skip it
      }
   }
}

void ViewDockAreaPopup::showPanelSlot() {
	_dock->toggleActive();
	_dock->_dockAreaPopup->hide();
}

#include "viewdockareapopup.moc"

