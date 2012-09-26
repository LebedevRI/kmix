/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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

#include "gui/kmixdockwidget.h"

#include <kaction.h>
#include <klocale.h>
#include <kmenu.h>
#include <kdebug.h>
#include <kwindowsystem.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDesktopWidget>
#include <QApplication>

#include "apps/kmix.h"
#include "core/ControlManager.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "gui/dialogselectmaster.h"
#include "gui/mixdevicewidget.h"
#include "gui/viewdockareapopup.h"


#define FEATURE_UNITY_POPUP true

void MetaMixer::reset()
{
  // TOOD remove the MetaMixer
    m_mixer = Mixer::getGlobalMasterMixer();
    // after changing the master device, make sure to re-read (otherwise no "changed()" signals might get sent by the Mixer
    m_mixer->readSetFromHWforceUpdate();
}

KMixDockWidget::KMixDockWidget(KMixWindow* parent, bool volumePopup)
    : KStatusNotifierItem(parent)
    , _oldToolTipValue(-1)
    , _oldPixmapType('-')
    , _volumePopup(volumePopup)
    , _kmixMainWindow(parent)
    , _contextMenuWasOpen(false)
{
    setToolTipIconByName("kmix");
    setTitle(i18n( "Volume Control"));
    setCategory(Hardware);
    setStatus(Active);

    m_metaMixer.reset();
//     createMasterVolWidget();
    createActions();

    connect(this, SIGNAL(scrollRequested(int,Qt::Orientation)), this, SLOT(trayWheelEvent(int,Qt::Orientation)));
    connect(this, SIGNAL(secondaryActivateRequested(QPoint)), this, SLOT(dockMute()));

    connect(contextMenu(), SIGNAL(aboutToShow()), this, SLOT(contextMenuAboutToShow()));

    if (_volumePopup)
    {
        kDebug() << "Construct the ViewDockAreaPopup and actions";
        _referenceWidget = new KMenu(parent);
        _referenceWidget2 = new ViewDockAreaPopup(_referenceWidget, "dockArea", 0, QString("no-guiprofile-yet-in-dock"), parent);

        _volWA = new QWidgetAction(_referenceWidget);
        _volWA->setDefaultWidget(_referenceWidget2);
        _referenceWidget->addAction(_volWA);
    }
    else
    {
        _volWA = 0;
        _referenceWidget = 0;
    }


  	ControlManager::instance().addListener(
	  QString(), // All mixers (as the Global master Mixer might change)
	ControlChangeType::Volume,
	this,
	QString("KMixDockWidget")	  
	);
	
	 ControlManager::instance().addListener(
	  QString(), // All mixers (as the Global master Mixer might change)
	ControlChangeType::MasterChanged,
	this,
	QString("KMixDockWidget")	  
	);
	 
	      // Refresh in all cases. When there is no Golbal Master we still need
     // to initialize correctly (e.g. for showin 0% or hiding it)
     refreshVolumeLevels();
}

KMixDockWidget::~KMixDockWidget()
{
  ControlManager::instance().removeListener(this);
    // Note: deleting _volWA also deletes its associated ViewDockAreaPopup (_referenceWidget) and prevents the
    //       action to be left with a dangling pointer.
    //       cesken: I adapted the patch from https://bugs.kde.org/show_bug.cgi?id=220621#c27 to branch /branches/work/kmix 
    delete _volWA;
}

void KMixDockWidget::controlsChange(int changeType)
{
  ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
  switch (type )
  {
    case  ControlChangeType::MasterChanged:
      m_metaMixer.reset();
      // Notify the main window, as it might need to update the visibiliy of the dock icon.
      _kmixMainWindow->updateDocking();
      _kmixMainWindow->saveConfig();
      refreshVolumeLevels();
      actionCollection()->action(QLatin1String("select_master"))->setEnabled(m_metaMixer.hasMixer());
      break;

    case ControlChangeType::Volume:
      refreshVolumeLevels();
      break;

    default:
      ControlManager::warnUnexpectedChangeType(type, this);
  }
}

void KMixDockWidget::refreshVolumeLevels()
{
  setVolumeTip();
  updatePixmap();
}

void KMixDockWidget::createActions()
{
    QMenu *menu = contextMenu();

    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if ( md.get() != 0 && md->hasMuteSwitch() ) {
        // Put "Mute" selector in context menu
        KToggleAction *action = actionCollection()->add<KToggleAction>( "dock_mute" );
        action->setText( i18n( "M&ute" ) );
        connect(action, SIGNAL(triggered(bool)), SLOT(dockMute()));
        menu->addAction( action );
    }

    // Put "Select Master Channel" dialog in context menu
    QAction *action = actionCollection()->addAction( "select_master" );
    action->setText( i18n("Select Master Channel...") );
    action->setEnabled(m_metaMixer.hasMixer());
    connect(action, SIGNAL(triggered(bool)), SLOT(selectMaster()));
    menu->addAction( action );

    //Context menu entry to access phonon settings
    menu->addAction(_kmixMainWindow->actionCollection()->action("launch_kdesoundsetup"));
}

void KMixDockWidget::selectMaster()
{
   DialogSelectMaster* dsm = new DialogSelectMaster(m_metaMixer.mixer());
   dsm->setAttribute(Qt::WA_DeleteOnClose, true);
   dsm->show();
}

/**
 * Returns the playback volume level in percent. If the volume is muted, 0 is returned.
 * If the given MixDevice contains no playback volume, the capture volume isd used
 * instead, and 0 is returned if capturing is disabled for the given MixDevice.
 * 
 * @returns The volume level in percent
 */
int KMixDockWidget::getUserfriendlyVolumeLevel(const shared_ptr<MixDevice>& md)
{
	bool usePlayback = md->playbackVolume().hasVolume();
	Volume& vol = usePlayback ? md->playbackVolume() : md->captureVolume();
	bool isActive = usePlayback ? !md->isMuted() : md->isRecSource();
	int val = isActive ? vol.getAvgVolumePercent(Volume::MALL) : 0;
	return val;
}

void
KMixDockWidget::setVolumeTip()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    QString tip = "";
    int virtualToolTipValue = 0;

    if ( md.get() == 0 )
    {
        tip = i18n("Mixer cannot be found"); // !! text could be reworked
        virtualToolTipValue = -2;
    }
    else
    {
        // Playback volume will be used for the DockIcon if available.
        // This heuristic is "good enough" for the DockIcon for now.
		int val = getUserfriendlyVolumeLevel(md);
       	tip = i18n( "Volume at %1%", val );
        if ( md->isMuted() )
        	tip += i18n( " (Muted)" );

        // create a new "virtual" value. With that we see "volume changes" as well as "muted changes"
        virtualToolTipValue = val;
        if ( md->isMuted() )
        	virtualToolTipValue += 10000;
    }

    // The actual updating is only done when the "toolTipValue" was changed (to avoid flicker)
    if ( virtualToolTipValue != _oldToolTipValue )
    {
        // changed (or completely new tooltip)
        setToolTipTitle(tip);
    }
    _oldToolTipValue = virtualToolTipValue;
}

// void KMixDockWidget::updateDockPopup()
// {
//   kDebug() << "KMixDockWidget::updateDockPopup";
//   _referenceWidget2->createDeviceWidgets();
// }

void
KMixDockWidget::updatePixmap()
{
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();

    char newPixmapType;
    if ( md == 0 )
    {
        newPixmapType = 'e';
    }
    else
    {
    	int percentage = getUserfriendlyVolumeLevel(md);
		if      ( percentage <= 0 ) newPixmapType = '0';  // Hint: also negative-values
		else if ( percentage < 25 ) newPixmapType = '1';
		else if ( percentage < 75 ) newPixmapType = '2';
		else                        newPixmapType = '3';
    }

   if ( newPixmapType != _oldPixmapType ) {
      // Pixmap must be changed => do so
      switch ( newPixmapType ) {
         case 'e': setIconByName( "kmixdocked_error" ); break;
         case 'm': 
         case '0': setIconByName( "audio-volume-muted"  ); break;
         case '1': setIconByName( "audio-volume-low"  ); break;
         case '2': setIconByName( "audio-volume-medium" ); break;
         case '3': setIconByName( "audio-volume-high" ); break;
      }
   }

   _oldPixmapType = newPixmapType;
}

void KMixDockWidget::activate(const QPoint &pos)
{
    kDebug() << "Activate at " << pos;

    bool showHideMainWindow = false;
    showHideMainWindow |= (_referenceWidget == 0);
    showHideMainWindow |= (pos.x() == 0  && pos.y() == 0);  // HACK. When the action comes from the context menu, the pos is (0,0)

    if ( showHideMainWindow ) {
        // Use default KStatusNotifierItem behavior if we are not using the dockAreaPopup
        kDebug() << "Use default KStatusNotifierItem behavior";
        setAssociatedWidget(_kmixMainWindow);
//        KStatusNotifierItem::activate(pos);
        KStatusNotifierItem::activate();
        return;
    }

    KMenu* dockAreaPopup =_referenceWidget; // TODO Refactor to use _referenceWidget directly
    kDebug() << "Skip default KStatusNotifierItem behavior";
    if ( dockAreaPopup->isVisible() ) {
        dockAreaPopup->hide();
        kDebug() << "dap is visible => hide and return";
        return;
    }

//    if (dockAreaPopup->isVisible()) {
//        contextMenu()->hide();
//        setAssociatedWidget(_kmixMainWindow);
//        KStatusNotifierItem::activate(pos);
//        kDebug() << "cm is visible => setAssociatedWidget(_kmixMainWindow)";
//        return;
//    }
    if ( false ) {}
    else {
        setAssociatedWidget(_referenceWidget);
        kDebug() << "cm is NOT visible => setAssociatedWidget(_referenceWidget)";

        dockAreaPopup->adjustSize();
        int h = dockAreaPopup->height();
        int x = pos.x() - dockAreaPopup->width()/2;
        int y = pos.y() - h;

        // kDebug() << "h="<<h<< " x="<<x << " y="<<y<< "gx="<< geometry().x() << "gy="<< geometry().y();

        if ( y < 0 ) {
            y = pos.y();
        }

        dockAreaPopup->move(x, y);  // so that the mouse is outside of the widget
        kDebug() << "moving to" << dockAreaPopup->size() << x << y;

        dockAreaPopup->show();

        // Now handle Multihead displays. And also make sure that the dialog is not
        // moved out-of-the screen on the right (see Bug 101742).
        const QDesktopWidget* vdesktop = QApplication::desktop();
        const QRect& vScreenSize = vdesktop->screenGeometry(dockAreaPopup);
        //const QRect screenGeometry(const QWidget *widget) const
        if ( (x+dockAreaPopup->width()) > (vScreenSize.width() + vScreenSize.x()) ) {
            // move horizontally, so that it is completely visible
            dockAreaPopup->move(vScreenSize.width() + vScreenSize.x() - dockAreaPopup->width() -1 , y);
            kDebug() << "Multihead: (case 1) moving to" << vScreenSize.x() << "," << vScreenSize.y();
        }
        else if ( x < vScreenSize.x() ) {
            // horizontally out-of bound
            dockAreaPopup->move(vScreenSize.x(), y);
            kDebug() << "Multihead: (case 2) moving to" << vScreenSize.x() << "," << vScreenSize.y();
        }
        // the above stuff could also be implemented vertically

        KWindowSystem::setState( dockAreaPopup->winId(), NET::StaysOnTop | NET::SkipTaskbar | NET::SkipPager );
    }
}


void
KMixDockWidget::trayWheelEvent(int delta,Qt::Orientation wheelOrientation)
{
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
	if ( md.get() == 0 )
		return;


      Volume &vol = ( md->playbackVolume().hasVolume() ) ?  md->playbackVolume() : md->captureVolume();
      int inc = vol.volumeSpan() / Mixer::VOLUME_STEP_DIVISOR;

    if ( inc < 1 ) inc = 1;

    if (wheelOrientation == Qt::Horizontal) // Reverse horizontal scroll: bko228780 
    	delta = -delta;

    long int cv = inc * (delta / 120 );
//    kDebug() << "twe: " << cv << " : " << vol;
    bool isInactive =  vol.isCapture() ? !md->isRecSource() : md->isMuted();
    kDebug() << "Operating on capture=" << vol.isCapture() << ", isInactive=" << isInactive;
	if ( cv > 0 && isInactive)
	{   // increasing from muted state: unmute and start with a low volume level
		if ( vol.isCapture())
			md->setRecSource(true);
		else
			md->setMuted(false);
		vol.setAllVolumes(cv);
	}
	else
	    vol.changeAllVolumes(cv);

//	kDebug() << "twe: " << cv << " : " << vol;

    md->mixer()->commitVolumeChange(md);
    refreshVolumeLevels();
}


void
KMixDockWidget::dockMute()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if ( md )
    {
        md->toggleMute();
        md->mixer()->commitVolumeChange( md );
	refreshVolumeLevels();
    }
}

void
KMixDockWidget::contextMenuAboutToShow()
{
  // TODO Unity / Gnome only support one type of activation (left-click == right-click)
  //      So we should show here the ViewDockAreaPopup instead of the menu:
  // Unity: Detect, by finding DBUS Service com.canonical.Unity.Panel.Service
  // Gnome2: ?
  // Gnome3: ?
  
  QDBusConnection connection = QDBusConnection::sessionBus();
#ifdef FEATURE_UNITY_POPUP
    if (connection.interface()->isServiceRegistered("com.canonical.Unity.Panel.Service"))
    {
      // This is the wrong place (aboutToShow will definitely ALSO show the menu)
      QPoint popupPos = QPoint(100,100);
      activate(popupPos);
      kError() << "LOOKS LIKE KMix IS RUNNING UNDER Unity ... should show ViewDockAreaPopup instead";
      return;      
    }
#endif

    // Enable/Disable "Muted" menu item
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    KToggleAction *dockMuteAction = static_cast<KToggleAction*>(actionCollection()->action("dock_mute"));
    //kDebug(67100) << "---> md=" << md << "dockMuteAction=" << dockMuteAction << "isMuted=" << md->isMuted();
    if ( md != 0 && dockMuteAction != 0 )
    {
    	Volume& vol = md->playbackVolume().hasVolume() ? md->playbackVolume() : md->captureVolume();
    	bool isInactive =  vol.isCapture() ? md->isMuted() : !md->isRecSource();
        bool hasSwitch = vol.isCapture() ? vol.hasSwitch() : md->hasMuteSwitch();
        dockMuteAction->setEnabled( hasSwitch );
        dockMuteAction->setChecked( hasSwitch && !isInactive );
    }
    _contextMenuWasOpen = true;
}

#include "kmixdockwidget.moc"
