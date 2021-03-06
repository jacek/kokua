/** 
 * @file llfloaterabout.cpp
 * @author James Cook
 * @brief The about box from Help->About
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
 
#include "llviewerprecompiledheaders.h"

#include "llfloaterabout.h"

// Viewer includes
#include "kokuastreamingaudio.h"
#include "llagent.h"
#include "llappviewer.h" 
#include "llsecondlifeurls.h"
#include "llvoiceclient.h"
#include "lluictrlfactory.h"
#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "viewerinfo.h"
#include "llweb.h"

// Linden library includes
#include "llaudioengine.h"
#include "llbutton.h"
#include "llcurl.h"
#include "llglheaders.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llimagej2c.h"
#include "llsys.h"
#include "lltrans.h"
#include "lluri.h"
#include "v3dmath.h"
#include "llwindow.h"
#include "stringize.h"
#include "llsdutil_math.h"
#include "lleventapi.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

extern LLMemoryInfo gSysMemory;
extern U32 gPacketsIn;

static std::string get_viewer_release_notes_url();


///----------------------------------------------------------------------------
/// Class LLFloaterAbout
///----------------------------------------------------------------------------
class LLFloaterAbout 
	: public LLFloater
{
	friend class LLFloaterReg;
private:
	LLFloaterAbout(const LLSD& key);
	virtual ~LLFloaterAbout();

public:
	/*virtual*/ BOOL postBuild();

	/// Obtain the data used to fill out the contents string. This is
	/// separated so that we can programmatically access the same info.
	static LLSD getInfo();
	void onClickCopyToClipboard();
};


// Default constructor
LLFloaterAbout::LLFloaterAbout(const LLSD& key) 
:	LLFloater(key)
{
	
}

// Destroys the object
LLFloaterAbout::~LLFloaterAbout()
{
}

BOOL LLFloaterAbout::postBuild()
{
	center();
	LLViewerTextEditor *support_widget = 
		getChild<LLViewerTextEditor>("support_editor", true);

	LLViewerTextEditor *credits_widget = 
		getChild<LLViewerTextEditor>("credits_editor", true);

	getChild<LLUICtrl>("copy_btn")->setCommitCallback(
		boost::bind(&LLFloaterAbout::onClickCopyToClipboard, this));

#if LL_WINDOWS
	getWindow()->incBusyCount();
	getWindow()->setCursor(UI_CURSOR_ARROW);
#endif
	LLSD info(getInfo());
#if LL_WINDOWS
	getWindow()->decBusyCount();
	getWindow()->setCursor(UI_CURSOR_ARROW);
#endif

	std::ostringstream support;

	// Render the LLSD from getInfo() as a format_map_t
	LLStringUtil::format_map_t args;

	// allow the "Release Notes" URL label to be localized
	args["ReleaseNotes"] = LLTrans::getString("ReleaseNotes");

	for (LLSD::map_const_iterator ii(info.beginMap()), iend(info.endMap());
		 ii != iend; ++ii)
	{
		if (! ii->second.isArray())
		{
			// Scalar value
			if (ii->second.isUndefined())
			{
				args[ii->first] = getString("none");
			}
			else
			{
				// don't forget to render value asString()
				args[ii->first] = ii->second.asString();
			}
		}
		else
		{
			// array value: build KEY_0, KEY_1 etc. entries
			for (LLSD::Integer n(0), size(ii->second.size()); n < size; ++n)
			{
				args[STRINGIZE(ii->first << '_' << n)] = ii->second[n].asString();
			}
		}
	}

	// Now build the various pieces
	support << getString("AboutHeader", args);
	if (info.has("REGION"))
	{
		support << "\n\n" << getString("AboutPosition", args);
	}
	support << "\n\n" << getString("AboutSystem", args);
	support << "\n";
	if (info.has("GRAPHICS_DRIVER_VERSION"))
	{
		support << "\n" << getString("AboutDriver", args);
	}
	support << "\n" << getString("AboutLibs", args);
	if (info.has("COMPILER"))
	{
		support << "\n" << getString("AboutCompiler", args);
	}
	if (info.has("PACKETS_IN"))
	{
		support << '\n' << getString("AboutTraffic", args);
	}

	support_widget->appendText(support.str(), 
								FALSE, 
								LLStyle::Params()
									.color(LLUIColorTable::instance().getColor("TextFgReadOnlyColor")));
	support_widget->blockUndo();

	// Fix views
	support_widget->setEnabled(FALSE);
	support_widget->startOfDoc();

	credits_widget->setEnabled(FALSE);
	credits_widget->startOfDoc();

	return TRUE;
}

// static
LLSD LLFloaterAbout::getInfo()
{
	// The point of having one method build an LLSD info block and the other
	// construct the user-visible About string is to ensure that the same info
	// is available to a getInfo() caller as to the user opening
	// LLFloaterAbout.
	LLSD info;
	LLSD version;
	version.append(ViewerInfo::versionMajor());
	version.append(ViewerInfo::versionMinor());
	version.append(ViewerInfo::versionPatch());
	info["VIEWER_VERSION"] = version;
	info["VIEWER_VERSION_STR"] = ViewerInfo::prettyVersion();
	info["BUILD_DATE"] = __DATE__;
	info["BUILD_TIME"] = __TIME__;
	info["CHANNEL"] = ViewerInfo::viewerVariant();

	info["VIEWER_RELEASE_NOTES_URL"] = get_viewer_release_notes_url();

#if LL_MSVC
	info["COMPILER"] = "MSVC";
	info["COMPILER_VERSION"] = _MSC_VER;
#elif LL_GNUC
	info["COMPILER"] = "GCC";
	info["COMPILER_VERSION"] = GCC_VERSION;
#endif

	// Position
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		const LLVector3d &pos = gAgent.getPositionGlobal();
		info["POSITION"] = ll_sd_from_vector3d(pos);
		info["REGION"] = gAgent.getRegion()->getName();
		info["HOSTNAME"] = gAgent.getRegion()->getHost().getHostName();
		info["HOSTIP"] = gAgent.getRegion()->getHost().getString();
		info["SERVER_VERSION"] = gLastVersionChannel;
		info["SERVER_RELEASE_NOTES_URL"] = LLWeb::escapeURL(region->getCapability("ServerReleaseNotes"));
	}

	// CPU
	info["CPU"] = gSysCPU.getCPUString();
	info["MEMORY_MB"] = LLSD::Integer(gSysMemory.getPhysicalMemoryKB() / 1024);
	// Moved hack adjustment to Windows memory size into llsys.cpp
	info["OS_VERSION"] = LLAppViewer::instance()->getOSInfo().getOSString();
	info["GRAPHICS_CARD_VENDOR"] = (const char*)(glGetString(GL_VENDOR));
	info["GRAPHICS_CARD"] = (const char*)(glGetString(GL_RENDERER));

#if LL_WINDOWS
    LLSD driver_info = gDXHardware.getDisplayInfo();
    if (driver_info.has("DriverVersion"))
    {
        info["GRAPHICS_DRIVER_VERSION"] = driver_info["DriverVersion"];
    }
#endif

	info["OPENGL_VERSION"] = (const char*)(glGetString(GL_VERSION));
	info["LIBCURL_VERSION"] = LLCurl::getVersionString();
	info["J2C_VERSION"] = LLImageJ2C::getEngineInfo();
	bool want_fullname = true;
	info["AUDIO_DRIVER_VERSION"] = gAudiop ? LLSD(gAudiop->getDriverName(want_fullname)) : LLSD();
	info["STREAMING_AUDIO_DRIVER_VERSION"] = gAudioStream ? LLSD(gAudioStream->getVersion()) : LLSD();
	if(LLVoiceClient::getInstance()->voiceEnabled())
	{
		LLVoiceVersionInfo version = LLVoiceClient::getInstance()->getVersion();
		std::ostringstream version_string;
		version_string << version.serverType << " " << version.serverVersion << std::endl;
		info["VOICE_VERSION"] = version_string.str();
	}
	else 
	{
		info["VOICE_VERSION"] = LLTrans::getString("NotConnected");
	}
	
	// TODO: Implement media plugin version query
	info["QT_WEBKIT_VERSION"] = "4.6 (version number hard-coded)";

	if (gPacketsIn > 0)
	{
		info["PACKETS_LOST"] = LLViewerStats::getInstance()->mPacketsLostStat.getCurrent();
		info["PACKETS_IN"] = F32(gPacketsIn);
		info["PACKETS_PCT"] = 100.f*info["PACKETS_LOST"].asReal() / info["PACKETS_IN"].asReal();
	}

    return info;
}

static std::string get_viewer_release_notes_url()
{
	// return a URL to the release notes for this viewer, such as:
	// http://wiki.secondlife.com/wiki/Release_Notes/Second Life Beta Viewer/2.1.0
	std::string url = LLTrans::getString("RELEASE_NOTES_BASE_URL");
	if (! LLStringUtil::endsWith(url, "/"))
	{
		url += "/";
	}
	url += ViewerInfo::prettyVersion();
	return LLWeb::escapeURL(url);
}

class LLFloaterAboutListener: public LLEventAPI
{
public:
	LLFloaterAboutListener():
		LLEventAPI("LLFloaterAbout",
                   "LLFloaterAbout listener to retrieve About box info")
	{
		add("getInfo",
            "Request an LLSD::Map containing information used to populate About box",
            &LLFloaterAboutListener::getInfo,
            LLSD().with("reply", LLSD()));
	}

private:
	void getInfo(const LLSD& request) const
	{
		LLReqID reqid(request);
		LLSD reply(LLFloaterAbout::getInfo());
		reqid.stamp(reply);
		LLEventPumps::instance().obtain(request["reply"]).post(reply);
	}
};

static LLFloaterAboutListener floaterAboutListener;

void LLFloaterAbout::onClickCopyToClipboard()
{
	LLViewerTextEditor *support_widget = 
		getChild<LLViewerTextEditor>("support_editor", true);
	support_widget->selectAll();
	support_widget->copy();
	support_widget->deselect();
}

///----------------------------------------------------------------------------
/// LLFloaterAboutUtil
///----------------------------------------------------------------------------
void LLFloaterAboutUtil::registerFloater()
{
	LLFloaterReg::add("sl_about", "floater_about.xml",
		&LLFloaterReg::build<LLFloaterAbout>);

}
