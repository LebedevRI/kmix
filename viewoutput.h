#ifndef ViewOutput_h
#define ViewOutput_h

#include "viewsliders.h"
class QWidget;
class Mixer;

class ViewOutput : public ViewSliders
{
    Q_OBJECT
public:
    ViewOutput(QWidget* parent, const char* name, Mixer* mixer, bool menuInitallyVisible);
    ~ViewOutput();

    virtual void setMixSet(MixSet *mixset);
};

#endif

