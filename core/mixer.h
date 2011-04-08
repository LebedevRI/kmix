//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * 1996-2000 Christian Esken <esken@kde.org>
 * Sven Fischer <herpes@kawo2.rwth-aachen.de>
 * 2002 - Helio Chissini de Castro <helio@conectiva.com.br>
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

#ifndef RANDOMPREFIX_MIXER_H
#define RANDOMPREFIX_MIXER_H

#include <QList>
#include <QObject>
#include <QString>

#include "core/volume.h"
class Mixer_Backend;
#include "core/MasterControl.h"
#include "mixset.h"
#include "core/mixdevice.h"
#include "dbus/dbusmixerwrapper.h"

class Volume;
class KConfig;

class Mixer : public QObject
{
      Q_OBJECT

public:
    enum MixerError { ERR_PERM=1, ERR_WRITE, ERR_READ,
        ERR_OPEN, ERR_LASTERR };

    Mixer( QString& ref_driverName, int device );
    virtual ~Mixer();

    static int numDrivers();
    QString getDriverName();

    MixDevice* find(const QString& devPK);
    static Mixer* findMixer( const QString& mixer_id);

    void volumeSave( KConfig *config );
    void volumeLoad( KConfig *config );

    /// Tells the number of the mixing devices
    unsigned int size() const;

    /// Returns a pointer to the mix device with the given number
    MixDevice* operator[](int val_i_num);

    /// Returns a pointer to the mix device whose type matches the value
    /// given by the parameter and the array MixerDevNames given in
    /// mixer_oss.cpp (0 is Volume, 4 is PCM, etc.)
    MixDevice *getMixdeviceById( const QString& deviceID );

    /// Open/grab the mixer for further intraction
    bool openIfValid();

    /// Returns whether the card is open/operational
    bool isOpen() const;

    /// Close/release the mixer
    virtual int close();

    /// Reads balance
    int balance() const;

    /// Returns a detailed state message after errors. Only for diagnostic purposes, no i18n.
    QString& stateMessage() const;

    /// Returns the name of the card/chip/hardware, as given by the driver. The name is NOT instance specific,
    /// so if you install two identical soundcards, two of them will deliver the same mixerName().
    /// Use this method if you need an instance-UNspecific name, e.g. for finding an appropriate
    /// mixer layout for this card, or as a prefix for constructing instance specific ID's like in id().
    virtual QString getBaseName();

    /// Wrapper to Mixer_Backend
    QString translateKernelToWhatsthis(const QString &kernelName);

    /// Return the name of the card/chip/hardware, which is suitable for humans
    virtual QString readableName();

    // Returns the name of the driver, e.g. "OSS" or "ALSA0.9"
    static QString driverName(int num);

    /// Returns an unique ID of the Mixer. It currently looks like "<soundcard_descr>::<hw_number>:<driver>"
    QString& id();
    /// The owner/creator of the Mixer can set an unique name here. This key should never displayed to
    /// the user, but can be used for referencing configuration items and such.

    void setCardInstance(int cardInstance);
    int getCardInstance() const      {          return _cardInstance;      }

    //void setID(QString& ref_id);


    /// Returns an Universal Device Identifaction of the Mixer. This is an ID that relates to the underlying operating system.
    // For OSS and ALSA this is taken from Solid (actually HAL). For Solaris this is just the device name.
    // Examples:
    // ALSA: /org/freedesktop/Hal/devices/usb_device_d8c_1_noserial_if0_sound_card_0_2_alsa_control__1
    // OSS: /org/freedesktop/Hal/devices/usb_device_d8c_1_noserial_if0_sound_card_0_2_oss_mixer__1
    // Solaris: /dev/audio
    QString& udi();

    // Returns a DBus path for this mixer
    // Used also by MixDevice to bind to this path
    const QString dbusPath();

    static QList<Mixer*> & mixers();

    /******************************************
    The KMix GLOBAL master card. Please note that KMix and KMixPanelApplet can have a
    different MasterCard's at the moment (but actually KMixPanelApplet does not read/save this yet).
    At the moment it is only used for selecting the Mixer to use in KMix's DockIcon.
    ******************************************/
    static void setGlobalMaster(QString ref_card, QString ref_control, bool preferred);
    static MixDevice* getGlobalMasterMD();
    static MixDevice* getGlobalMasterMD(bool fallbackAllowed);
    static Mixer* getGlobalMasterMixer();
    static Mixer* getGlobalMasterMixerNoFalback();
    static MasterControl& getGlobalMasterPreferred();

    /******************************************
    The recommended master of this Mixer.
    ******************************************/
    MixDevice* getLocalMasterMD();
    void setLocalMasterMD(QString&);

    /// get the actual MixSet
    MixSet getMixSet();


    /// DBUS oriented methods
    virtual void increaseVolume( const QString& mixdeviceID );
    virtual void decreaseVolume( const QString& mixdeviceID );

    /// Says if we are dynamic (e.g. widgets can come and go)
    virtual void setDynamic( bool dynamic = true );
    virtual bool isDynamic();

    virtual bool moveStream( const QString id, const QString& destId );

    void commitVolumeChange( MixDevice* md );

public slots:
    void readSetFromHWforceUpdate() const;
    virtual void setRecordSource(const QString & controlID, bool on);
    virtual void setBalance(int balance); // sets the m_balance (see there)
    
signals:
    void newBalance(Volume& );
    void controlChanged(void);
    void controlsReconfigured(const QString & mixer_ID);

protected:
    int m_balance; // from -100 (just left) to 100 (just right)
    static QList<Mixer*> s_mixers;

private:
    void setBalanceInternal(Volume& vol);
    void recreateId();


    Mixer_Backend *_mixerBackend;
    QString _id;
    QString _masterDevicePK;
    int    _cardInstance;
    static MasterControl _globalMasterCurrent;
    static MasterControl _globalMasterPreferred;

    bool m_dynamic;
};

#endif
