#ifndef __PmdApplication_h_
#define __PmdApplication_h_

#include "BaseApplication.h"

class PmdApplication : public BaseApplication
{
public:
    PmdApplication(void);
    virtual ~PmdApplication(void);

protected:
    virtual void createScene(void);
};

#endif // #ifndef __PmdApplication_h_
