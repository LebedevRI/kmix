/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KMIXERWIDGET_H
#define KMIXERWIDGET_H

#include <qwidget.h>
#include <qptrlist.h>
class QString;
class QGridLayout;

#include <kpanelapplet.h>
class KPopupMenu;

#include "mixer.h"
#include "mixdevicewidget.h"

// QT
class QSlider;


// KDE
class KActionCollection;
class KActionMenu;
class KConfig;
class KTabWidget;

// KMix
class Mixer;
class ViewInput;
class ViewOutput;
class ViewSwitches;


class KMixerWidget : public QWidget  
{
   Q_OBJECT

  public:
   KMixerWidget( int _id, Mixer *mixer, const QString &mixerName, int mixerNum,
                 MixDevice::DeviceCategory categoryMask = MixDevice::ALL ,
                 QWidget *parent=0, const char *name=0, bool menuInitallyVisible=true );
   ~KMixerWidget();
	
   enum KMixerWidgetIO { OUTPUT=0, INPUT };

   //   void addActionToPopup( KAction *action );

   //QString name() const { return m_name; };
   //void setName( const QString &name ) { m_name = name; };

   const Mixer *mixer() const { return _mixer; };
   //QString mixerName()  const { return m_mixerName; };
   int mixerNum() const { return m_mixerNum; };

   int id() const { return m_id; };

   KActionCollection* getActionCollection() const { return 0; /* m_actions; */ }
	
  signals:
   void masterMuted( bool );
   void newMasterVolume(Volume vol);
   void toggleMenuBar();

  public slots:
   void setTicks( bool on );
   void setLabels( bool on );
   void setIcons( bool on );
   void toggleMenuBarSlot();

   void saveConfig( KConfig *config, const QString &grp );
   void loadConfig( KConfig *config, const QString &grp );

  private slots:
   void updateBalance();
  //   void slotToggleMixerDevice(int id);

  private:
   Mixer *_mixer;
   QSlider *m_balanceSlider;
   QVBoxLayout *m_topLayout; // contains the Card selector, TabWidget and balance slider

   KTabWidget* m_ioTab;

   ViewOutput*    _oWidget;
   ViewInput*     _iWidget;
   ViewSwitches*  _swWidget;

   //QString m_name;
   //QString m_mixerName;
   int m_mixerNum;
   int m_id;

   //KActionCollection *m_actions;
   KActionMenu *m_toggleMixerChannels;

   bool _iconsEnabled;
   bool _labelsEnabled;
   bool _ticksEnabled;
   MixDevice::DeviceCategory m_categoryMask;

   void createLayout(bool menuInitallyVisible);
   //   void createMasterVolWidget(KPanelApplet::Direction);
};

#endif
