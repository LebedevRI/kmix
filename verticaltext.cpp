/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2003 Christian Esken <esken@kde.org>
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

#include "verticaltext.h"
#include <qpainter.h>
#include <kdebug.h>


VerticalText::VerticalText(QWidget * parent, const char * name, WFlags f) : QWidget(parent,name,f)
{
	resize(20,100 /*parent->height() */ );
	//setFixedWidth(20);
	setMinimumSize(20,10); // neccesary for smooth integration into layouts (we only care for the widths).
}

VerticalText::~VerticalText() {
}


void VerticalText::paintEvent ( QPaintEvent * /*event*/ ) {
	//kdDebug() << "paintEvent(). height()=" <<  height() << "\n";
	QPainter paint(this);
	paint.rotate(270);
	paint.drawText(-height()+2,width(),name());
}
