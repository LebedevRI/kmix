// Linux stuff, by Christian Esken
#if defined(linux)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>
// FreeBSD section, according to Sebestyen Zoltan
#elif defined(__FreeBSD__)
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include "machine/soundcard.h"
// NetBSD section, according to  Lennart Augustsson <augustss@cs.chalmers.se>
#elif defined(__NetBSD__)
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include <soundcard.h>
// BSDI section, according to <tom@foo.toetag.com>
#elif defined()
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/soundcard.h> 
// UnixWare includes
#elif defined(_UNIXWARE)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>
#endif

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_OSS( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}


Mixer_OSS::Mixer_OSS() : Mixer() { }
Mixer_OSS::Mixer_OSS(int devnum, int SetNum) : Mixer(devnum, SetNum) { }

void Mixer_OSS::setDevNumName_I(int devnum)
{
  switch (devnum) {
  case 0:
  case 1:
    devname = "/dev/mixer";
    break;
  default:
    devname = "/dev/mixer";
    devname += ('0'+devnum-1);
    break;
  }
}

QString Mixer_OSS::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n("kmix: You have no permission to access the mixer device.\n" \
			"Login as root and do a 'chmod a+rw /dev/mixer*' to allow the access.");
      break;
    case ERR_OPEN:
      l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
			"Please check that the soundcard is installed and the\n" \
			"soundcard driver is loaded.\n" \
			"On Linux you might need to use 'insmod' to load the driver.\n" \
			"Use 'soundon' when using commercial OSS.");
      break;
    default:
      l_s_errmsg = Mixer::errorText(mixer_error);
    }
  return l_s_errmsg;
}

void Mixer_OSS::readVolumeFromHW( int devnum, int *VolLeft, int *VolRight )
{
  int Volume;
  if (ioctl(fd, MIXER_READ( devnum ), &Volume) == -1)
    /* Oops, can't read mixer */
    errormsg(Mixer::ERR_READ);

  *VolLeft  = (Volume & 0x7f);
  *VolRight = ((Volume>>8) & 0x7f);
}


