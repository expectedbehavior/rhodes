#include "GeoLocation.h"
#include "common/RhodesApp.h"
#include "common/RhoMutexLock.h"
#include "common/IRhoClassFactory.h"
#include "common/RhoConf.h"
#include "net/INetRequest.h"
#include <math.h>

extern "C" {
double rho_geo_latitude();
double rho_geo_longitude();
int rho_geo_known_position();	
void rho_geoimpl_settimeout(int nTimeoutSec);
}

namespace rho {
using namespace common;
namespace rubyext{

IMPLEMENT_LOGCLASS(CGeoLocation,"GeoLocation");

CGeoLocation::CGeoLocation()
{
    m_nGeoPingTimeoutSec = 0;
}

void CGeoLocation::init(common::IRhoClassFactory* pFactory)
{
    m_NetRequest = pFactory->createNetRequest();
}

void CGeoLocation::callGeoCallback(const CGeoNotification& oNotify)
{
	if (oNotify.m_strUrl.length() == 0)
		return;
	
	String strFullUrl = getNet().resolveUrl(oNotify.m_strUrl);
	String strBody = "status=ok&rho_callback=1";
	strBody += "&known_position=" + convertToStringA(rho_geo_known_position());
	strBody += "&latitude=" + convertToStringA(rho_geo_latitude());
	strBody += "&longitude=" + convertToStringA(rho_geo_longitude());

    NetRequest( getNet().pushData( strFullUrl, strBody, null ) );
}

void CGeoLocation::callGeoCallback()
{
    synchronized(m_mxNotify)
    {
        callGeoCallback(m_ViewNotify);
        callGeoCallback(m_Notify);
    }
}

void CGeoLocation::setGeoCallback(const char *url, char* params, int timeout_sec, boolean bView)
{
    if ( bView)
        m_ViewNotify = CGeoNotification(url?url:"",params?params:"");
    else
        m_Notify = CGeoNotification(url?url:"",params?params:"");

    setPingTimeoutSec(timeout_sec);
}

int CGeoLocation::getDefaultPingTimeoutSec()
{
	int nPingTimeoutSec = RHOCONF().getInt("gps_ping_timeout_sec");
	if (nPingTimeoutSec==0)
		nPingTimeoutSec = 10;
	
	return nPingTimeoutSec;
}

void CGeoLocation::setPingTimeoutSec( int nTimeout )
{
	int nNewTimeout = nTimeout;
	if (nNewTimeout == 0)
		nNewTimeout = getDefaultPingTimeoutSec();
	
	if ( nNewTimeout != m_nGeoPingTimeoutSec)
	{
		m_nGeoPingTimeoutSec = nNewTimeout;
		rho_geoimpl_settimeout(m_nGeoPingTimeoutSec);
	}
}

int CGeoLocation::getGeoTimeoutSec()
{
	if (m_nGeoPingTimeoutSec==0)
		m_nGeoPingTimeoutSec = getDefaultPingTimeoutSec();
	
	return m_nGeoPingTimeoutSec;
}

/*static*/ void CGeoLocation::callback_geolocation(void *arg, String const &/*query*/ )
{
    if (!rho_geo_known_position())
    {
        rho_http_sendresponse(arg, "Reading;Reading;Reading");
        return;
    }
    
    double latitude = rho_geo_latitude();
    double longitude = rho_geo_longitude();

    char location[256];
    sprintf(location,"%.4f\xc2\xb0 %s, %.4f\xc2\xb0 %s;%f;%f",
        fabs(latitude),latitude < 0 ? "South" : "North",
        fabs(longitude),longitude < 0 ? "West" : "East",
        latitude,longitude);

    LOGC(INFO,CRhodesApp::getLogCategory())+ "Location: " + location;
    rho_http_sendresponse(arg, location);
}

}
}

extern "C" {

void rho_geo_set_notification( const char *url, char* params, int timeout_sec)
{
    if ( url && *url )
        rho_geo_known_position();

    RHODESAPP().getGeo().setGeoCallback(url, params, timeout_sec, false);
}

void rho_geo_set_view_notification( const char *url, char* params, int timeout_sec)
{
    if ( url && *url )
        rho_geo_known_position();

    RHODESAPP().getGeo().setGeoCallback(url, params, timeout_sec, true);
}

void rho_geo_callcallback()
{
    //RHODESAPP().getGeo().callGeoCallback();
}

int rho_geo_gettimeout_sec()
{
    return RHODESAPP().getGeo().getGeoTimeoutSec();
}

}
