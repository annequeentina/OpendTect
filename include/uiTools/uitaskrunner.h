#ifndef uitaskrunner_h
#define uitaskrunner_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2007
 RCS:           $Id: uitaskrunner.h,v 1.7 2008-06-03 03:20:38 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "thread.h"
#include "task.h"

class uiProgressBar;
class Timer;

class uiTaskRunner : public uiDialog
		   , public TaskRunner
{ 	
public:
			uiTaskRunner(uiParent*);
			~uiTaskRunner();

    bool		execute(Task& t);
    const char*		lastMsg() const		{ return prevmessage_.buf(); }
    int			getState() const	{ return state_; }

protected:

    uiProgressBar*	progbar_;

    Timer&		tim_;
    BufferString	execnm_;

    Task*		task_;
    Threads::Mutex	dispinfomutex_;
    int			prevtotalnr_;
    int			prevnrdone_;
    int			prevpercentage_;
    BufferString	prevmessage_;
    BufferString	prevnrdonetext_;

    Threads::Mutex&	statemutex_;	
    int			state_; //-1 finished in error
    				// 0 finished without error
				// 1 running
    Threads::Thread*	thread_;
    Threads::Mutex	uitaskrunnerthreadmutex_;
    void		doWork(CallBacker*);	//!< Method with work thread

    void		updateFields();
    void		onFinalise(CallBacker*);
    void		timerTick(CallBacker*);
    virtual bool        acceptOK(CallBacker*);

    virtual bool        rejectOK(CallBacker*);

    void		init();
};

#endif
