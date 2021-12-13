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

#include "gui/hotmic.h"

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStandardItemModel>
#include <QToolTip>
#include <QVBoxLayout>

#include <klocalizedstring.h>

#include "core/ControlManager.h"
#include "core/mixer.h"
#include "gui/kmixtoolbox.h"

HotMic::HotMic(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);

    HUD = new QPushButton(this);
    connect(HUD, &QPushButton::released, this, &HotMic::toggleMute);

    updateHUD();

    setCentralWidget(HUD);

    ControlManager::instance().addListener(
        QString(), // All mixers (as the global master mixer might change)
        ControlManager::Volume,
        this,
        QString("HotMic"));
}

HotMic::~HotMic()
{
    ControlManager::instance().removeListener(this);

    delete HUD;
}

void HotMic::updateHUD()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if (md.get() != nullptr) {
        if (!md->isMuted()) {
            HUD->setStyleSheet("background-color: red;");
            HUD->setText("!!! LIVE !!!");
        } else {
            HUD->setStyleSheet("background-color: green;");
            HUD->setText("offline");
        }
    }
}

void HotMic::controlsChange(ControlManager::ChangeType changeType)
{
    switch (changeType) {
    case ControlManager::MasterChanged:
        Q_FALLTHROUGH();

    case ControlManager::Volume:
        updateHUD();
        break;

    default:
        ControlManager::warnUnexpectedChangeType(changeType, this);
        break;
    }
}

void HotMic::toggleMute()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if (md.get() != nullptr) {
        md->toggleMute();
        md->mixer()->commitVolumeChange(md);
    }
}
