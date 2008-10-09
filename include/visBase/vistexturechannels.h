#ifndef vistexturechannels_h
#define vistexturechannels_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		Jun 2008
 RCS:		$Id: vistexturechannels.h,v 1.2 2008-10-09 21:27:27 cvskris Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoTextureChannelSet;
class SoSwitch;
class SbImage;

template <class T> class Array2D;

namespace ColTab { class MapperSetup; };

namespace visBase
{

class TextureChannel2RGBA;
class ChannelInfo;

class TextureChannels : public DataObject
{
public:
    void			setSize(int,int,int=1);
    int				getSize(int dim) const;

    virtual bool		turnOn(bool yn);
    virtual bool		isOn() const;

    int				nrChannels() const;
    int				addChannel();
    int				insertChannel(int);
    void			removeChannel(int);
    void			swapChannels(int,int);

    void			setColTabMapperSetup(int channel,
	    					const ColTab::MapperSetup&);
    				//!<Will not trigger a remap, use reMapData
    const ColTab::MapperSetup&	getColTabMapperSetup(int channel) const;
    void			reMapData(int channel);

    void			setNrVersions(int channel,int nrvers);
    int				nrVersions(int channel) const;
    int				currentVersion(int channel) const;
    void			setCurrentVersion(int channel,int version);

    enum			CachePolicy { None, CacheCopy, Cache, TakeOver};

    bool			setUnMappedData(int channel,int version,
	    				        const float*, CachePolicy);
    bool			setMappedData(int channel,int version,
	    				      unsigned char*, CachePolicy);

    bool			setChannels2RGBA(TextureChannel2RGBA*);

    SoNode*			getInventorNode();
    const SbImage*		getChannels() const;

protected:
    friend			class ChannelInfo;
    void			update( int channel );
    				TextureChannels();
    				~TextureChannels();

    int				size_[3];
    ObjectSet<ChannelInfo>	channelinfo_;
    SoTextureChannelSet*	tc_;
    SoSwitch*			onoff_;
    TextureChannel2RGBA*	tc2rgba_;

};


}; // Namespace

#endif
