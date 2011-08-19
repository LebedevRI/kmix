/**
 * KMix -- MPRIS2 backend
 *
 * Copyright (C) 2011 Christian Esken <esken@kde.org>
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
 *
 */

#ifndef Mixer_MPRIS2_H
#define Mixer_MPRIS2_H

#include <QString>
#include <QMainWindow>
#include <QtDBus>
#include <QDBusInterface>
#include <QMap>

#include "mixer_backend.h"

class MPrisAppdata : public QObject
{
  Q_OBJECT 
public:
  MPrisAppdata();
  ~MPrisAppdata();
  QString id;
  QDBusInterface* propertyIfc;
  QDBusInterface* playerIfc;

public slots:
//  void volumeChangedIncoming(QString ifc, QList<QVariant> msg);
  void volumeChangedIncoming(QString,QVariantMap,QStringList);
  
signals:
  void volumeChanged(MPrisAppdata* mad, double);
};

class Mixer_MPRIS2 : public Mixer_Backend
{
  Q_OBJECT
public:
  explicit Mixer_MPRIS2(Mixer *mixer, int device = -1 );
    virtual ~Mixer_MPRIS2();
    void getMprisControl(QDBusConnection& conn, QString arg1);
    QString getDriverName();

  virtual int open();
  virtual int close();
  virtual int readVolumeFromHW( const QString& id, MixDevice * );
  virtual int writeVolumeToHW( const QString& id, MixDevice * );
  virtual void setEnumIdHW(const QString& id, unsigned int);
  virtual unsigned int enumIdHW(const QString& id);
  virtual bool moveStream( const QString& id, const QString& destId );
  virtual bool needsPolling() { return false; }

  virtual int mediaPlay(QString id);
  virtual int mediaPrev(QString id);
  virtual int mediaNext(QString id);
  virtual int mediaControl(QString id, QString command);

public slots:
  void volumeChanged(MPrisAppdata* mad, double);

private:
  int run();
//  static char MPRIS_IFC2[40];
  static QString MPRIS_IFC2;
  
  static QString getBusDestination(const QString& id);
  
  QMap<QString,MPrisAppdata*> apps;
};

#endif

