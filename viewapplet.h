#ifndef ViewApplet_h
#define ViewApplet_h

#include "viewbase.h"
#include <kpanelapplet.h>

class QBoxLayout;
class QHBox;
class Mixer;

class ViewApplet : public ViewBase
{
    Q_OBJECT
public:
    ViewApplet(QWidget* parent, const char* name, Mixer* mixer, KPanelApplet::Direction direction);
    ~ViewApplet();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();

    QSize sizeHint();
    virtual void resizeEvent(QResizeEvent*);

public slots:
   virtual void refreshVolumeLevels();

private:
    QBoxLayout*   _layoutMDW;
    KPanelApplet::Direction _direction;
};

#endif

