#ifndef monitor_h
#define monitor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "notify.h"


/*!\brief Object that can be MT-safely monitored from cradle to grave.

  Traditionally, the MVC concept was all about automatic updates. This can
  still be done, but nowadays more important seems the integrity for MT.

  The instanceCreated() will tell you when *any* Monitorable is created. To
  make your class really monitorable, use mDeclInstanceCreatedNotifierAccess
  for your subclass, too. Note that you'll need to add
  mTriggerInstanceCreatedNotifier() to the constructor, and add
  mDefineInstanceCreatedNotifierAccess(YourClassName) to a .cc.

  Similarly, you have to trigger the objectToBeDeleted() at the start of your
  destructor - if you do not have one, make one. For that, use sendDelNotif(),
  it avoids double notifications; this base class will eventually do such a
  thing but then it's too late for many purposes: the subclass part of the
  object is then already dead. Thus, at the beginning of the your destructor,
  call sendDelNotif().

  For typical usage see NamedMonitorable.

*/

mExpClass(Basic) Monitorable : public CallBacker
{
public:

					Monitorable(const Monitorable&);
    virtual				~Monitorable();
    Monitorable&			operator =(const Monitorable&);

    virtual Notifier<Monitorable>&	objectChanged()
					{ return chgnotif_; }
    virtual Notifier<Monitorable>&	objectToBeDeleted()
					{ return delnotif_; }
    mDeclInstanceCreatedNotifierAccess(	Monitorable );
					//!< defines static instanceCreated()

protected:

				Monitorable();

    mutable Threads::Lock	accesslock_;

    //!\brief makes locking easier and safer
    mExpClass(Basic) AccessLockHandler
    {
    public:
				AccessLockHandler(const Monitorable& m,
						  bool forread=true);
	bool			convertToWrite()
				{ return locker_.convertToWriteLock(); }
	Threads::Locker		locker_;
    };

    void			sendChgNotif(AccessLockHandler&);
				//!< objectChanged called with released lock
    void			sendDelNotif();

    template <class T>
    inline T			getSimple(const T&) const;
    template <class TMember,class TSetTo>
    inline void			setSimple(TMember&,TSetTo);

private:

    Notifier<Monitorable>	chgnotif_;
    Notifier<Monitorable>	delnotif_;
    bool			delalreadytriggered_;

};

//! For use in subclasses of Monitorable
#define mLock4Read() AccessLockHandler accesslockhandler_( *this )
#define mLock4Write() AccessLockHandler accesslockhandler_( *this, false )
#define mLock2Write() accesslockhandler_.convertToWrite()
#define mUnlockAllAccess() accesslockhandler_.locker_.unlockNow()
#define mSendChgNotif() sendChgNotif( accesslockhandler_ )


template <class T>
inline T Monitorable::getSimple( const T& memb ) const
{
    mLock4Read();
    return memb;
}

template <class TMember,class TSetTo>
inline void Monitorable::setSimple( TMember& memb, TSetTo setto )
{
    mLock4Read();
    if ( memb == setto )
	return;
    if ( mLock2Write() || !(memb == setto) )
    {
	memb = setto;
	mSendChgNotif();
    }
}


#define mImplSimpleMonitoredGet(fnnm,typ,memb) \
    typ fnnm() const { return getSimple( memb ); }
#define mImplSimpleMonitoredSet(fnnm,typ,memb) \
    void fnnm( typ _set_to_ ) { setSimple( memb, _set_to_ ); }
#define mImplSimpleMonitoredGetSet(pfx,fnnmget,fnnmset,typ,memb) \
    pfx mImplSimpleMonitoredGet(fnnmget,typ,memb) \
    pfx mImplSimpleMonitoredSet(fnnmset,const typ&,memb)


#endif