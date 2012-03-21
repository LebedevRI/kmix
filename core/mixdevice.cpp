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

#include "core/mixdevice.h"

#include <qregexp.h>

#include <kdebug.h>
#include <klocale.h>

#include "core/ControlPool.h"
#include "core/mixer.h"
#include "gui/guiprofile.h"
#include "core/volume.h"
#include "dbus/dbuscontrolwrapper.h"

static const QString channelTypeToIconName( MixDevice::ChannelType type )
{
    switch (type) {
        case MixDevice::AUDIO:
            return "mixer-pcm";
        case MixDevice::BASS:
        case MixDevice::SURROUND_LFE: // "LFE" SHOULD have an own icon
            return "mixer-lfe";
        case MixDevice::CD:
            return "mixer-cd";
        case MixDevice::EXTERNAL:
            return "mixer-line";
        case MixDevice::MICROPHONE:
            return "mixer-microphone";
        case MixDevice::MIDI:
            return "mixer-midi";
        case MixDevice::RECMONITOR:
            return "mixer-capture";
        case MixDevice::TREBLE:
            return "mixer-pcm-default";
        case MixDevice::UNKNOWN:
            return "mixer-front";
        case MixDevice::VOLUME:
            return "mixer-master";
        case MixDevice::VIDEO:
            return "mixer-video";
        case MixDevice::SURROUND:
        case MixDevice::SURROUND_BACK:
            return "mixer-surround";
        case MixDevice::SURROUND_CENTERFRONT:
        case MixDevice::SURROUND_CENTERBACK:
            return "mixer-surround-center";
        case MixDevice::HEADPHONE:
            return "mixer-headset";
        case MixDevice::DIGITAL:
            return "mixer-digital";
        case MixDevice::AC97:
            return "mixer-ac97";
        case MixDevice::SPEAKER:
            return "mixer-pc-speaker";
        case MixDevice::MICROPHONE_BOOST:
            return "mixer-microphone-boost";
        case MixDevice::MICROPHONE_FRONT_BOOST:
            return "mixer-microphone-front-boost";
        case MixDevice::MICROPHONE_FRONT:
            return "mixer-microphone-front";
        case MixDevice::KMIX_COMPOSITE:
            return "mixer-line";
        case MixDevice::APPLICATION_AMAROK:
            return "amarok";
        case MixDevice::APPLICATION_BANSHEE:
            return "media-player-banshee";
        case MixDevice::APPLICATION_XMM2:
            return "xmms";
        case MixDevice::APPLICATION_STREAM:
            return "mixer-pcm";
      
    }
    return "mixer-front";
}


/**
 * Constructs a MixDevice. A MixDevice represents one channel or control of
 * the mixer hardware. A MixDevice has a type (e.g. PCM), a descriptive name
 * (for example "Master" or "Headphone" or "IEC 958 Output"),
 * can have a volume level (2 when stereo), can be recordable and muted.
 * The ChannelType tells which kind of control the MixDevice is.
 */
MixDevice::MixDevice(  Mixer* mixer, const QString& id, const QString& name, ChannelType type )
{
    init(mixer, id, name, channelTypeToIconName(type), (MixSet*)0);
}

MixDevice::MixDevice(  Mixer* mixer, const QString& id, const QString& name, const QString& iconName, MixSet* moveDestinationMixSet )
{
    init(mixer, id, name, iconName, moveDestinationMixSet);
}

void MixDevice::init(  Mixer* mixer, const QString& id, const QString& name, const QString& iconName, MixSet* moveDestinationMixSet )
{
    _artificial = false;
    _applicationStream = false;
    _mixer = mixer;
    _id = id;
    mediaPlayControl = false;
    mediaNextControl = false;
    mediaPrevControl = false;
    if( name.isEmpty() )
        _name = i18n("unknown");
    else
        _name = name;
    if ( iconName.isEmpty() )
        _iconName = "mixer-front";
    else
        _iconName = iconName;
    _moveDestinationMixSet = moveDestinationMixSet;
    if ( _id.contains(' ') ) {
        // The key is used in the config file. It MUST NOT contain spaces
        kError(67100) << "MixDevice::setId(\"" << id << "\") . Invalid key - it must not contain spaces" << endl;
        _id.replace(' ', '_');
    }
    kDebug(67100) << "MixDevice::init() _id=" << _id;
}

shared_ptr<MixDevice> MixDevice::addToPool()
{
    const QString& fullyQualifiedId = getFullyQualifiedId();
    kDebug() << "MixDevice::init() id=" << fullyQualifiedId;

    shared_ptr<MixDevice> thisSharedPtr = ControlPool::instance()->add(fullyQualifiedId, this);
    new DBusControlWrapper( thisSharedPtr, dbusPath() );
	return thisSharedPtr;
}


QString MixDevice::getFullyQualifiedId()
{
	QString fqId = QString("%1@%2").arg(_id).arg(_mixer->id());
	return fqId;
}

void MixDevice::addPlaybackVolume(Volume &playbackVol)
{
   // Hint: "_playbackVolume" gets COPIED from "playbackVol", because the copy-constructor actually copies the volume levels.
   _playbackVolume = playbackVol;
   _playbackVolume.setSwitchType(Volume::PlaybackSwitch);
}

void MixDevice::addCaptureVolume (Volume &captureVol)
{
   _captureVolume = captureVol;
   _captureVolume.setSwitchType(Volume::CaptureSwitch);
}

void MixDevice::addEnums(QList<QString*>& ref_enumList)
{
   if ( ref_enumList.count() > 0 ) {
      int maxEnumId = ref_enumList.count();
      for (int i=0; i<maxEnumId; i++ ) {
            // we have an enum. Lets set the names of the enum items in the MixDevice
            // the enum names are assumed to be static!
            _enumValues.append( *(ref_enumList.at(i)) );
      }
   }
}


MixDevice::~MixDevice() {
    _enumValues.clear(); // The QString's inside will be auto-deleted, as they get unref'ed
}

Volume& MixDevice::playbackVolume()
{
    return _playbackVolume;
}

Volume& MixDevice::captureVolume()
{
    return _captureVolume;
}


void MixDevice::setEnumId(int enumId)
{
   if ( enumId < _enumValues.count() ) {
      _enumCurrentId = enumId;
   }
}

unsigned int MixDevice::enumId()
{
   return _enumCurrentId;
}

QList<QString>& MixDevice::enumValues() {
   return _enumValues;
}


const QString& MixDevice::id() const {
   return _id;
}

const QString MixDevice::dbusPath() {
	QString controlPath = _id;
	controlPath.replace(QRegExp("[^a-zA-Z0-9_]"), "_");
	return _mixer->dbusPath() + "/" + controlPath;
}


bool MixDevice::isMuted()                  { return ( _playbackVolume.hasSwitch() && ! _playbackVolume.isSwitchActivated() ); }
void MixDevice::setMuted(bool value)       { _playbackVolume.setSwitch( ! value ); }
void MixDevice::toggleMute()               { setMuted( !isMuted() ); }
bool MixDevice::isRecSource()              { return ( _captureVolume.hasSwitch() &&  _captureVolume.isSwitchActivated() ); }
bool MixDevice::isNotRecSource()           { return ( _captureVolume.hasSwitch() && !_captureVolume.isSwitchActivated() ); }
void MixDevice::setRecSource(bool value)   { _captureVolume.setSwitch( value ); }
bool MixDevice::isEnum()                   { return ( ! _enumValues.empty() ); }

int MixDevice::mediaPlay() { return mixer()->mediaPlay(_id); }
int MixDevice::mediaPrev() { return mixer()->mediaPrev(_id); }
int MixDevice::mediaNext() { return mixer()->mediaNext(_id); }

bool MixDevice::operator==(const MixDevice& other) const
{
   return ( _id == other._id );
}

void MixDevice::setControlProfile(ProfControl* control)
{
    _profControl = control;
}

ProfControl* MixDevice::controlProfile() {
    return _profControl;
}

/**
 * This method is currently only called on "kmixctrl --restore"
 *
 * Normally we have a working _volume object already, which is very important,
 * because we need to read the minimum and maximum volume levels.
 * (Another solution would be to "equip" volFromConfig with maxInt and minInt values).
 */
void MixDevice::read( KConfig *config, const QString& grp )
{
    if ( _mixer->isDynamic() || isArtificial() ) {
        kDebug(67100) << "MixDevice::read(): This MixDevice does not permit volume restoration (i.e. because it is handled lower down in the audio stack). Ignoring.";
    } else {
        QString devgrp = QString("%1.Dev%2").arg(grp).arg(_id);
        KConfigGroup cg = config->group( devgrp );
        //kDebug(67100) << "MixDevice::read() of group devgrp=" << devgrp;

        readPlaybackOrCapture(cg, false);
        readPlaybackOrCapture(cg, true );

        bool mute = cg.readEntry("is_muted", false);
        setMuted( mute );

        bool recsrc = cg.readEntry("is_recsrc", false);
        setRecSource( recsrc );

        int enumId = cg.readEntry("enum_id", -1);
        if ( enumId != -1 ) {
            setEnumId( enumId );
        }
    }
}

void MixDevice::readPlaybackOrCapture(const KConfigGroup& config, bool capture)
{
    Volume& volume = capture ? captureVolume() : playbackVolume();
    
    for ( Volume::ChannelID chid=Volume::CHIDMIN; chid<= Volume::CHIDMAX;  )
    {
      QString volstr = getVolString(chid,capture);
       if ( capture ) volstr += "Capture";
       if ( config.hasKey(volstr) ) {
          volume.setVolume(chid, config.readEntry(volstr, 0));
       } // if saved channel exists
       chid = (Volume::ChannelID)( 1 + (int)chid); // ugly
    } // for all channels    
}

/**
 *  called on "kmixctrl --save" and from the GUI's (currently only on exit)
 */
void MixDevice::write( KConfig *config, const QString& grp )
{
    if (_mixer->isDynamic() || isArtificial()) {
        kDebug(67100) << "MixDevice::write(): This MixDevice does not permit volume saving (i.e. because it is handled lower down in the audio stack). Ignoring.";
    } else {
        QString devgrp = QString("%1.Dev%2").arg(grp).arg(_id);
        KConfigGroup cg = config->group(devgrp);
        // kDebug(67100) << "MixDevice::write() of group devgrp=" << devgrp;

        writePlaybackOrCapture(cg, false);
        writePlaybackOrCapture(cg, true );

        cg.writeEntry("is_muted" , isMuted() );
        cg.writeEntry("is_recsrc", isRecSource() );
        cg.writeEntry("name", _name);
        if ( isEnum() ) {
            cg.writeEntry("enum_id", enumId() );
        }
    }
}

void MixDevice::writePlaybackOrCapture(KConfigGroup& config, bool capture)
{
    Volume& volume = capture ? captureVolume() : playbackVolume();
    foreach (VolumeChannel vc, volume.getVolumes() )
    {
       config.writeEntry(getVolString(vc.chid,capture) , (int)vc.volume);
    } // for all channels

}

QString MixDevice::getVolString(Volume::ChannelID chid, bool capture)
{
       QString volstr (Volume::ChannelNameForPersistence[chid]);
       if ( capture ) volstr += "Capture";
       return volstr;
}

#include "mixdevice.moc"

