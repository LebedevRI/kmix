/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken <esken@kde.org>
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

#include "guiprofile.h"

// Qt
#include <QDir>
#include <qxml.h>
#include <QString>

// System
#include <iostream>
#include <utility>

// KDE
#include <kdebug.h>
#include <kstandarddirs.h>

// KMix
#include "mixer.h"

//QMap<Mixer*,GUIProfile*> GUIProfile::s_fallbackProfiles;
QMap<QString, GUIProfile*> GUIProfile::s_profiles;

bool SortedStringComparator::operator()(const std::string& s1, const std::string& s2) const {
    return ( s1 < s2 );
}

/**
 * Product comparator for sorting:
 * We want the comparator to sort ascending by Vendor. "Inside" the Vendors, we sort by Product Name.
 */
bool ProductComparator::operator()(const ProfProduct* p1, const ProfProduct* p2) const {
	if ( p1->vendor < p2->vendor ) {
		return ( true );
	}
	else if ( p1->vendor > p2->vendor ) {
		return ( false );
	}
	else if ( p1->productName < p2->productName ) {
		return ( true );
	}
	else if ( p1->productName > p2->productName ) {
		return ( false );
	}
	else {
		/**
		 * We reach this point, if vendor and product name is identical.
		 * Actually we don't care about the order then, so we decide that "p1" comes first.
		 *
		 * (Hint: As this is a set comparator, the return value HERE doesn't matter that
		 * much. But if we would decide later to change this Comparator to be a Map Comparator,
		 *  we must NOT return a "0" for identity - this would lead to non-insertion on insert())
		 */
		return true;
	}
}

GUIProfile::GUIProfile()
{
    _dirty = false;
}

GUIProfile::~GUIProfile()
{
    kError() << "Thou shalt not delete any GUI profile. This message is only OK, when quitting KMix"; 
    qDeleteAll(_controls);
    qDeleteAll(_tabs);
    qDeleteAll(_products);
}


void GUIProfile::setId(QString id)
{
    _id = id;
}

QString GUIProfile::getId()
{
    return _id;
}

bool GUIProfile::isDirty() {
    return _dirty;
}

/**
 * Build a profile name. Suitable to use as primary key and to build filenames.
 * @par driverName The driver name of the backend, e.g. "ALSA" or "OSS"
 * @par cardName    The card name as delivered by the driver. Use the String "any" for the standard (card-unspecific) profile
 */
QString GUIProfile::buildProfileName(QString driverName, QString mixerId, QString profileName)
{
    //QString fname(driverName);
    //fname += "." + mixerId + "." + profileName;
    //fname.replace(" ","_");
    QString fname(mixerId);
    fname += "." + profileName;
    fname.replace(" ","_");
    return fname;
}

/**
 * Finds the correct profile for the given mixer.
 * If already loaded from disk, returns the cached version.
 * Otherwise load profile from disk.
 *
 * @arg mixer         The mixer
 * @arg profileName   The profile name (e.g. "capture", "playback", "my-cool-profile", or "any"
 * @arg allowFallback If set to true, a Fallback profile will be generated if no matchibg profile could be found
 * @return GUIProfile*  The loaded GUIProfile, or 0 if no profile matched. Hint: if you use allowFallback==true, this should never return 0.
 */
GUIProfile* GUIProfile::find(Mixer* mixer, QString profileName, bool allowFallback)
{
    GUIProfile* guiprof = 0;
    QString requestedProfileName = buildProfileName(mixer->getDriverName(), mixer->id(), profileName);
    if ( s_profiles.contains(requestedProfileName) ) {
        guiprof = s_profiles.value(requestedProfileName);  // Cached
    }
    else {
        guiprof = loadProfileFromXMLfiles(mixer, profileName);  // Load from XML
        if ( guiprof != 0 ) {
            // OK, loaded
        }
        else
        {
            if ( allowFallback ) {
                guiprof = fallbackProfile(mixer);
                kDebug() << "Generate fallback profile:" << guiprof;
            }
        }
        
        if ( guiprof != 0 ) {
            guiprof->_mixerId = mixer->id();
            if ( guiprof->getId().isEmpty() ) {
                guiprof->setId(requestedProfileName);
            }
            if ( guiprof->getName().isEmpty() ) {
                guiprof->setName(profileName);
            }
            addProfile(guiprof);
        }
    }
    

    
    return guiprof;
}

/*
 * Add the profile to the internal list of profiles (Profile caching).
 * As this means that the profile is new, the "dirty" flag gets cleared.
 */
void GUIProfile::addProfile(GUIProfile* guiprof)
{
    guiprof->_dirty = false;
    s_profiles[guiprof->getId()] = guiprof;
    kDebug() << "I have added" << guiprof->getId() << "; Number of profiles is now " <<  s_profiles.size() ;
    
}




/**
 * Loads a GUI Profile from disk (xml profile file).
 * It tries to load the Soundcard specific file first (a).
 * If it doesn't exist, it will load the default profile corresponding to the soundcard driver (b).
 */
GUIProfile* GUIProfile::loadProfileFromXMLfiles(Mixer* mixer, QString profileName)
{
    GUIProfile* guiprof = 0;
    QString fileName, fileNameFQ, guiProfID;
    
    // (a) Soundcard specific profile
    guiProfID = buildProfileName(mixer->getDriverName(), mixer->id(), profileName) ;
    fileName = "profiles/" + guiProfID + ".xml";
    fileNameFQ = KStandardDirs::locate("appdata", fileName );
    kDebug() << "fileName=" <<fileName<< "fileNameFQ=" <<fileNameFQ;
    guiprof = tryLoadProfileFromXMLfile(fileNameFQ, mixer);
    
    if ( guiprof == 0 ) {
        // (b) Default profile
        guiProfID = buildProfileName(mixer->getDriverName(),"any", profileName);
        fileName = "profiles/" + guiProfID + ".xml";
        fileNameFQ = KStandardDirs::locate("appdata", fileName );
        kDebug() << "fileName=" <<fileName<< "fileNameFQ=" <<fileNameFQ;
        guiprof = tryLoadProfileFromXMLfile(fileNameFQ, mixer);
    }
    
    if ( guiprof != 0 ) {
        guiprof->setId(guiProfID);
    }
    return guiprof;
}


/**
 * Internal helper for GUIProfile::loadProfileFromXMLfiles()
 */
GUIProfile* GUIProfile::tryLoadProfileFromXMLfile(QString fname, Mixer *mixer)
{
    kDebug() << ": Read profile #" << fname;
    GUIProfile *guiprof = new GUIProfile();
    if ( guiprof->readProfile(fname) && ( guiprof->match(mixer) > 0) ) {
        // loaded
    }
    else {
        delete guiprof; // not good (e.g. Parsing error => drop this profile silently)
        guiprof = 0;
    }
    
    return guiprof;
}

GUIProfile* GUIProfile::fallbackProfile(Mixer *mixer)
{
        GUIProfile *fallback = new GUIProfile();
        
        ProfProduct* prd = new ProfProduct();
        prd->vendor         = mixer->getDriverName();
        prd->productName    = mixer->readableName();
        prd->productRelease = "1.0";
        fallback->_products.insert(prd);
        
        ProfControl* ctl = new ProfControl();
        static QString matchAll(".*");
        ctl->id          = matchAll;
        ctl->regexp      = matchAll;   // make sure id matches the regexp
        ctl->setSubcontrols(matchAll);
        ctl->show        = "simple";
        fallback->_controls.push_back(ctl);
        
        fallback->_soundcardDriver = mixer->getDriverName();
        fallback->_soundcardName   = mixer->readableName();
        
        fallback->finalizeProfile();
        return fallback;
}


/**
 * Fill the profile with the data from the given XML profile file.
 * @par  ref_fileName: Full qualified filename (with path).
 * @return bool True, if the profile was succesfully created. False if not (e.g. parsing error).
 */
bool GUIProfile::readProfile(QString& ref_fileName)
{
    QXmlSimpleReader *xmlReader = new QXmlSimpleReader();
    kDebug() << "Read profile:" << ref_fileName ;
    QFile xmlFile( ref_fileName );
    QXmlInputSource source( &xmlFile );
    GUIProfileParser* gpp = new GUIProfileParser(this);
    xmlReader->setContentHandler(gpp);
    bool ok = xmlReader->parse( source );

    //std::cout << "Raw Profile: " << *this;
    if ( ok ) {
        ok = finalizeProfile();
    } // Read OK
    else {
        // !! this error message about faulty profiles should probably be surrounded with i18n()
        kError(67100) << "ERROR: The profile '" << ref_fileName<< "' contains errors, and is not used." << endl;
    }
    delete gpp;
    delete xmlReader;
   
    return ok;
}


bool GUIProfile::writeProfile(QString& fname)
{
   bool ret = false;
   QString fileName, fileNameFQ;
   fileName = "profiles/" + getId() + ".xml";
   fileName.replace(":", ".");
   fileNameFQ = KStandardDirs::locateLocal("appdata", fileName, true );

   kDebug() << "Write profile:" << fileNameFQ ;
   QFile f(fileNameFQ);
   if ( f.open(QIODevice::WriteOnly | QFile::Truncate) )
   { 
      QTextStream out(&f);
      out << *this;
      f.close();
      ret = true;
   }

   return ret;
}

bool GUIProfile::finalizeProfile()
{
   bool ok = true;
		// Reading is OK => now make the profile consistent

		// (1) Make sure the _tabs are complete (add any missing Tabs)
		std::vector<ProfControl*>::const_iterator itEnd = _controls.end();
		for ( std::vector<ProfControl*>::const_iterator it = _controls.begin(); it != itEnd; ++it)
		{
			ProfControl* control = *it;
			QString tabnameOfControl = control->tab;
			if ( tabnameOfControl.isNull() ) {
			  tabnameOfControl = "Controls";
			  control->tab = "Controls";
			}
				// check, whether we have this Tab yet.
				//std::vector<ProfTab*>::iterator tabRef = std::find(_tabs.begin(), _tabs.end(), tabnameOfControl);
				QList<ProfTab*>::const_iterator itTEnd = _tabs.end();
                QList<ProfTab*>::const_iterator itT = _tabs.begin();
				for ( ; itT != itTEnd; ++itT) {
				    if ( (*itT)->name() == tabnameOfControl ) break;
				}
				if ( itT == itTEnd ) {
					// no such Tab yet => insert it
					ProfTab* tab = new ProfTab();
					tab->setName(tabnameOfControl);
					tab->setType("Sliders");  //  as long as we don't know better
					if ( tab->id().isNull() ) tab->setId( tab->name() );;
					_tabs.push_back(tab);
				} // tab does not exist yet => insert new tab
		} // Step (1)

		// (2) Make sure that there is at least one Tab
		if ( _tabs.size() == 0) {
			ProfTab* tab = new ProfTab();
            tab->setName("Controls"); // !! A better name should be used. What about i18n() ?
            tab->setId("Controls"); // !! A better name should be used. What about i18n() ?
            tab->setType("Sliders");  //  as long as we don't know better
			_tabs.push_back(tab);
		} // Step (2)


/*
		// (3) Assign a Tab Name to all controls that have no defined Tab Name yet.
		ProfTab* tab = _tabs.front();
		itEnd = _controls.end();		for ( std::vector<ProfControl*>::const_iterator it = _controls.begin(); it != itEnd; ++it)
		{
			ProfControl* control = *it;
			QString& tabnameOfControl = control->tab;
			if ( tabnameOfControl.isNull() ) {
				// OK, it has no TabName defined. We will assign a TabName in step (3).
				control->tab = tab->name;
			}
		} // Step (3)
		//std::cout << "Consistent Profile: " << *this;
*/

   return ok;
}

/**
 * Returns how good the given Mixer matches this GUIProfile.
 * A value between 0 (not matching at all) and MAXLONG (perfect match) is returned.
 *
 * Here is the current algorithm:
 *
 * If the driver doesn't match, 0 is returned. (OK)
 * If the card-name ...  (OK)
 *     is "*", this is worth 1 point
 *     doesn't match, 0 is returned.
 *     matches, this is worth 500 points.
 *
 * If the "card type" ...
 *     is empty, this is worth 0 points.     !!! not implemented yet
 *     doesn't match, 0 is returned.         !!! not implemented yet
 *     matches , this is worth 500 points.  !!! not implemented yet
 *
 * If the "driver version" doesn't match, 0 is returned. !!! not implemented yet
 * If the "driver version" matches, this is worth ...
 *     4000 unlimited                             <=> "*:*"
 *     6000 toLower-bound-limited                   <=> "toLower-bound:*"
 *     6000 upper-bound-limited                   <=> "*:upper-bound"
 *     8000 upper- and toLower-bound limited        <=> "toLower-bound:upper-bound"
 * or 10000 points (upper-bound=toLower-bound=bound <=> "bound:bound"
 *
 * The Profile-Generation is added to the already achieved points. (done)
 *   The maximum gain is 900 points.
 *   Thus you can create up to 900 generations (0-899) without "overriding"
 *   the points gained from the "driver version" or "card-type".
 *
 * For example:  card-name="*" (1), card-type matches (1000),
 *               driver version "*:*" (4000), Profile-Generation 4 (4).
 *         Sum:  1 + 1000 + 4000 + 4 = 5004
 *
 * @todo Implement "card type" match value
 * @todo Implement "version" match value (must be in backends as well)
 */
unsigned long GUIProfile::match(Mixer* mixer) {
	unsigned long matchValue = 0;
	if ( _soundcardDriver != mixer->getDriverName() ) {
		return 0;
	}
	if ( _soundcardName == "*" ) {
		matchValue += 1;
	}
	else if ( _soundcardName != mixer->baseName() ) {
		return 0; // card name does not match
	}
	else {
		matchValue += 500; // card name matches
	}

	// !!! we don't check current for the driver version.
	//     So we assign simply 4000 points for now.
	matchValue += 4000;
	if ( _generation < 900 ) {
		matchValue += _generation;
	}
	else {
		matchValue += 900;
	}
	return matchValue;
}

QString xmlify(QString raw);

QString xmlify(QString raw)
{
// 	kDebug() << "Before: " << raw;
	raw = raw.replace("&", "&amp;");
	raw = raw.replace("<", "&lt;");
	raw = raw.replace(">", "&gt;");
	raw = raw.replace("'", "&apos;");
	raw = raw.replace("\"", "&quot;");
// 	kDebug() << "After : " << raw;
	return raw;
}


QTextStream& operator<<(QTextStream &os, const GUIProfile& guiprof)
{
// kDebug() << "ENTER QTextStream& operator<<";
   os << "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
   os << endl << endl;

   os << "<soundcard driver=\"" << xmlify(guiprof._soundcardDriver).toUtf8().constData() << "\""
      << " version = \"" << guiprof._driverVersionMin << ":" << guiprof._driverVersionMax  << "\"" << endl
      << " name = \"" << xmlify(guiprof._soundcardName).toUtf8().constData() << "\"" << endl
      << " type = \"" << xmlify(guiprof._soundcardType).toUtf8().constData() << "\"" << endl
      << " generation = \"" << guiprof._generation << "\"" << endl
      << ">" << endl << endl ;

	for ( GUIProfile::ProductSet::const_iterator it = guiprof._products.begin(); it != guiprof._products.end(); ++it)
	{
		ProfProduct* prd = *it;
		os << "<product vendor=\"" << xmlify(prd->vendor).toUtf8().constData() << "\" name=\"" << xmlify(prd->productName).toUtf8().constData() << "\"";
		if ( ! prd->productRelease.isNull() ) {
			os << " release=\"" << xmlify(prd->productRelease).toUtf8().constData() << "\"";
		}
		if ( ! prd->comment.isNull() ) {
			os << " comment=\"" << xmlify(prd->comment).toUtf8().constData() << "\"";
		}
		os << " />" << endl;
	} // for all products
	os << endl;

	for ( std::vector<ProfControl*>::const_iterator it = guiprof._controls.begin(); it != guiprof._controls.end(); ++it)
	{
		ProfControl* profControl = *it;
		os << "<control id=\"" << xmlify(profControl->id).toUtf8().constData() << "\"" ;
		if ( !profControl->name.isNull() && profControl->name != profControl->id ) {
		 	os << " name=\"" << xmlify(profControl->name).toUtf8().constData() << "\"" ;
		}
// @todoe		os << " subcontrols=\"" << xmlify(profControl->subcontrols).toUtf8().constData() << "\"" ;
		if ( ! profControl->tab.isNull() ) {
			os << " tab=\"" << xmlify(profControl->tab).toUtf8().constData() << "\"" ;
		}
		os << " show=\"" << xmlify(profControl->show).toUtf8().constData() << "\"" ;
		os << " />" << endl;
	} // for all controls
	os << endl;

    /*
        QList<ProfTab*>::const_iterator it = guiprof.tabs().begin();
	for ( ; it != guiprof.tabs().end(); ++it) {
		ProfTab* profTab = *it;
        os << "<tab name=\"" << xmlify(profTab->name()).toUtf8().constData() << "\" id=\"" << xmlify(profTab->id()).toUtf8().constData() << "\" type=\"" << xmlify(profTab->type()).toUtf8().constData() << "\"";
                os << " />" << endl;
	} // for all tabs
	os << endl;
*/
	os << "</soundcard>" << endl;
// kDebug() << "EXIT  QTextStream& operator<<";
	return os;
}

std::ostream& operator<<(std::ostream& os, const GUIProfile& guiprof) {
	os  << "Soundcard:" << std::endl
			<< "  Driver=" << guiprof._soundcardDriver.toUtf8().constData() << std::endl
			<< "  Driver-Version min=" << guiprof._driverVersionMin
			<< " max=" << guiprof._driverVersionMax << std::endl
			<< "  Card-Name=" << guiprof._soundcardName.toUtf8().constData() << std::endl
			<< "  Card-Type=" << guiprof._soundcardType.toUtf8().constData() << std::endl
			<< "  Profile-Generation="  << guiprof._generation
			<< std::endl;

	for ( GUIProfile::ProductSet::const_iterator it = guiprof._products.begin(); it != guiprof._products.end(); ++it)
	{
		ProfProduct* prd = *it;
		os << "Product:\n  Vendor=" << prd->vendor.toUtf8().constData() << std::endl << "  Name=" << prd->productName.toUtf8().constData() << std::endl;
		if ( ! prd->productRelease.isNull() ) {
			os << "  Release=" << prd->productRelease.toUtf8().constData() << std::endl;
		}
		if ( ! prd->comment.isNull() ) {
			os << "  Comment = " << prd->comment.toUtf8().constData() << std::endl;
		}
	} // for all products

	for ( std::vector<ProfControl*>::const_iterator it = guiprof._controls.begin(); it != guiprof._controls.end(); ++it)
	{
		ProfControl* profControl = *it;
		os << "Control:\n  ID=" << profControl->id.toUtf8().constData() << std::endl;
		if ( !profControl->name.isNull() && profControl->name != profControl->id ) {
		 		os << "  Name = " << profControl->name.toUtf8().constData() << std::endl;
		}
// @todo		os << "  Subcontrols=" << profControl->subcontrols.toUtf8().constData() << std::endl;
		if ( ! profControl->tab.isNull() ) {
			os << "  Tab=" << profControl->tab.toUtf8().constData() << std::endl;
		}
		os << "  Shown-On=" << profControl->show.toUtf8().constData() << std::endl;
	} // for all controls

/*
    QList<ProfTab*>::const_iterator it = guiprof.tabs().begin();
	for ( ; it != guiprof.tabs().end(); ++it) {
		ProfTab* profTab = *it;
        os << "Tab: " << std::endl << "  ID: " << profTab->id().toUtf8().constData() << " :" << profTab->name().toUtf8().constData() << " (" << profTab->type().toUtf8().constData() << ")" << std::endl;
	} // for all tabs
*/
	return os;
}

ProfTab::ProfTab()
{
    _id = "";
    _name = "";
    _type = "";
}

ProfControl::ProfControl(){
}

ProfControl::ProfControl(const ProfControl &profControl){

		id = profControl.id;
		name = profControl.name;
		QString origSctls = profControl._subcontrols;
		setSubcontrols(origSctls); // @todo. Copy the 5 fields instead???
		name = profControl.name;
		tab = profControl.tab;
		show = profControl.show;
		backgroundColor = profControl.backgroundColor;
		switchtype = profControl.switchtype;
}


void ProfControl::setSubcontrols(QString sctls)
{
  _useSubcontrolPlayback = false;
  _useSubcontrolCapture = false;
  _useSubcontrolPlaybackSwitch = false;
  _useSubcontrolCaptureSwitch = false;
  _useSubcontrolEnum = false;

  QStringList qsl = sctls.split( ',',  QString::SkipEmptyParts, Qt::CaseInsensitive);
  QStringListIterator qslIt(qsl);
  while (qslIt.hasNext()) {
    QString sctl = qslIt.next();
       kDebug() << "setSubcontrols found: " << sctl.toLocal8Bit().constData() << endl;
       if ( sctl == "pvolume" ) _useSubcontrolPlayback = true;
       else if ( sctl == "cvolume" ) _useSubcontrolCapture = true;
       else if ( sctl == "pswitch" ) _useSubcontrolPlaybackSwitch = true;
       else if ( sctl == "cswitch" ) _useSubcontrolCaptureSwitch = true;
       else if ( sctl == "enum" ) _useSubcontrolEnum = true;
       else if ( sctl == "*" || sctl == ".*") {
	 _useSubcontrolCapture = true;
	 _useSubcontrolCaptureSwitch = true;
	 _useSubcontrolPlayback = true;
	 _useSubcontrolPlaybackSwitch = true;
	 _useSubcontrolEnum = true;
       }
       else kWarning() << "Ignoring unknown subcontrol type '" << sctl << "' in profile";
  }
  
}

// ### PARSER START ################################################


GUIProfileParser::GUIProfileParser(GUIProfile* ref_gp) : _guiProfile(ref_gp)
{
}

bool GUIProfileParser::startDocument()
{
	_scope = GUIProfileParser::NONE;  // no scope yet
	return true;
}

bool GUIProfileParser::startElement( const QString& ,
                                    const QString& ,
                                    const QString& qName,
                                    const QXmlAttributes& attributes )
{
	switch ( _scope ) {
		case GUIProfileParser::NONE:
			/** we are reading the "top level" ***************************/
			if ( qName.toLower() == "soundcard" ) {
				_scope = GUIProfileParser::SOUNDCARD;
				addSoundcard(attributes);
			}
			else {
				// skip unknown top-level nodes
				std::cerr << "Ignoring unsupported element '" << qName.toUtf8().constData() << "'" << std::endl;
			}
			// we are accepting <soundcard> only
		break;

		case GUIProfileParser::SOUNDCARD:
			if ( qName.toLower() == "product" ) {
				// Defines product names under which the chipset/hardware is sold
				addProduct(attributes);
			}
			else if ( qName.toLower() == "control" ) {
				addControl(attributes);
			}
			else if ( qName.toLower() == "tab" ) {
				addTab(attributes);
			}
			else {
				std::cerr << "Ignoring unsupported element '" << qName.toUtf8().constData() << "'" << std::endl;
			}
			// we are accepting <product>, <control> and <tab>

		break;

	} // switch()
    return true;
}

bool GUIProfileParser::endElement( const QString&, const QString&, const QString& qName )
{
	if ( qName == "soundcard" ) {
		_scope = GUIProfileParser::NONE; // should work out OK, as we don't nest soundcard entries
	}
    return true;
}

void GUIProfileParser::addSoundcard(const QXmlAttributes& attributes) {
/*
	std::cout  << "Soundcard: ";
	printAttributes(attributes);
*/
	QString driver	= attributes.value("driver");
	QString version = attributes.value("version");
	QString name	= attributes.value("name");
	QString type	= attributes.value("type");
	QString generation = attributes.value("generation");
	if ( !driver.isNull() && !name.isNull() ) {
		_guiProfile->_soundcardDriver = driver;
		_guiProfile->_soundcardName = name;
		if ( type.isNull() ) {
			_guiProfile->_soundcardType = "";
		}
		else {
			_guiProfile->_soundcardType = type;
		}
		if ( version.isNull() ) {
			_guiProfile->_driverVersionMin = 0;
			_guiProfile->_driverVersionMax = 0;
		}
		else {
			std::pair<QString,QString> versionMinMax;
			splitPair(version, versionMinMax, ':');
			_guiProfile->_driverVersionMin = versionMinMax.first.toULong();
			_guiProfile->_driverVersionMax = versionMinMax.second.toULong();
		}
		if ( type.isNull() ) { type = ""; };
		if ( generation.isNull() ) {
			_guiProfile->_generation = 0;
		}
		else {
			// Hint: If the conversion fails, _generation will be assigned 0 (which is fine)
			_guiProfile->_generation = generation.toUInt();
		}
	}

}


void GUIProfileParser::addTab(const QXmlAttributes& attributes) {
/*
	    	std::cout  << "Tab: ";
	    	printAttributes(attributes);
*/
	QString name = attributes.value("name");
    QString type    = attributes.value("type");
    QString id      = attributes.value("id");
    if ( !name.isNull() && !type.isNull() ) {
		// If you define a Tab, you must set its Type
		// It is either "Input", "Output", "Switches" or "Surround"
		// These names are case sensitive and correspond 1:1 to the View-Names.
		// This could make it possible in the (far) future to have Views as Plugins.
		ProfTab* tab = new ProfTab();
		tab->setName(name);
        if (id.isNull()) tab->setId(name); else tab->setId(id);
		tab->setType(type);

		_guiProfile->tabs().push_back(tab);
	}
}

void GUIProfileParser::addProduct(const QXmlAttributes& attributes) {
	/*
	std::cout  << "Product: ";
	printAttributes(attributes);
	*/
	QString vendor = attributes.value("vendor");
	QString name = attributes.value("name");
	QString release = attributes.value("release");
	QString comment = attributes.value("comment");
	if ( !vendor.isNull() && !name.isNull() ) {
		// Adding a product makes only sense if we have at least vendor and product name
		ProfProduct *prd = new ProfProduct();
		prd->vendor = vendor;
		prd->productName = name;
		prd->productRelease = release;
		prd->comment = comment;

		_guiProfile->_products.insert(prd);
	}
}

void GUIProfileParser::addControl(const QXmlAttributes& attributes) {
	/*
	std::cout  << "Control: ";
	printAttributes(attributes);
	*/
	QString id = attributes.value("id");
    QString subcontrols = attributes.value("subcontrols");
	QString tab = attributes.value("tab");
	QString name = attributes.value("name");
	QString regexp = attributes.value("pattern");
	QString show = attributes.value("show");
	QString background = attributes.value("background");
	QString switchtype = attributes.value("switchtype");
	if ( !id.isNull() ) {
		// We need at least an "id". We can set defaults for the rest, if undefined.
		ProfControl *profControl = new ProfControl();
		if ( subcontrols.isNull() ) {
			subcontrols = ".*";
		}
		if ( tab.isNull() ) {
			// Ignore this for the moment. We will put it on the first existing Tab at the end of parsing
		}
		if ( name.isNull() ) {
         // ignore. isNull() will be checked by all users.
		}
               if ( !background.isNull() ) {
            // ignore. isNull() will be checked by all users.
       }
              if ( !switchtype.isNull() ) {
            // ignore. isNull() will be checked by all users.
       }
      if ( regexp.isNull() ) {
         // !! should do a dictonary lookup here, and i18n(). For now, just take over "id"
         regexp = !name.isNull() ? name : id;
      }

     if ( show.isNull() ) { show = "*"; }

		profControl->id = id;
		profControl->name = name;
		profControl->setSubcontrols(subcontrols);
		profControl->name = name;
		profControl->tab = tab;
		profControl->show = show;
		profControl->backgroundColor.setNamedColor (background);
		profControl->switchtype = switchtype;
		_guiProfile->_controls.push_back(profControl);
	} // id != null
}

void GUIProfileParser::printAttributes(const QXmlAttributes& attributes) {
		    if ( attributes.length() > 0 ) {
		        for ( int i = 0 ; i < attributes.length(); i++ ) {
					std::cout << attributes.qName(i).toUtf8().constData() << ":"<< attributes.value(i).toUtf8().constData() << " , ";
		        }
			    std::cout << std::endl;
		    }
}

void GUIProfileParser::splitPair(const QString& pairString, std::pair<QString,QString>& result, char delim)
{
	int delimPos = pairString.indexOf(delim);
	if ( delimPos == -1 ) {
		// delimiter not found => use an empty String for "second"
		result.first  = pairString;
		result.second = "";
	}
	else {
		// delimiter found
		result.first  = pairString.mid(0,delimPos);
		result.second = pairString.left(delimPos+1);
	}
}
