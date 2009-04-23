#include "RhoLogSink.h"

#include "common/RhoFile.h"
#include "common/StringConverter.h"

#if defined( OS_SYMBIAN )
#include <e32debug.h>
#endif

namespace rho {

CLogFileSink::CLogFileSink(const LogSettings& oSettings) : m_oLogConf(oSettings), m_nCirclePos(-1),
    m_pFile(0), m_pPosFile(0), m_nFileLogSize(0)
{

}

void CLogFileSink::writeLogMessage( String& strMsg ){
    int len = strMsg.length();

    if ( !m_pFile )    
        m_pFile = new general::CRhoFile();

    if ( !m_pFile->isOpened() ){
        m_pFile->open( getLogConf().getLogFilePath().c_str(), general::CRhoFile::OpenForAppend );
        m_nFileLogSize = m_pFile->size();
        loadLogPosition();
    }

    if ( getLogConf().getMaxLogFileSize() > 0 )
    {
        if ( ( m_nCirclePos >= 0 && m_nCirclePos + len > getLogConf().getMaxLogFileSize() ) || 
             ( m_nCirclePos < 0 && m_nFileLogSize + len > getLogConf().getMaxLogFileSize() ) )
        {
            m_pFile->movePosToStart();
            m_nFileLogSize = 0;
            m_nCirclePos = 0;
        }
    }

    int nWritten = m_pFile->write( strMsg.c_str(), len );
    m_pFile->flush();

    if ( m_nCirclePos >= 0 )
        m_nCirclePos += nWritten;
    else
        m_nFileLogSize += nWritten;

    saveLogPosition();
}

int CLogFileSink::getCurPos()
{
    return m_nCirclePos >= 0 ? m_nCirclePos : m_nFileLogSize;
}

void CLogFileSink::clear(){
    if ( m_pFile )    {
        delete m_pFile;
        m_pFile = NULL;
    }

    general::CRhoFile().deleteFile(getLogConf().getLogFilePath().c_str());
    String strPosPath = getLogConf().getLogFilePath() + "_pos";
    general::CRhoFile().deleteFile(strPosPath.c_str());
}

void CLogFileSink::loadLogPosition(){
    if ( !m_pPosFile )
        m_pPosFile = new general::CRhoFile();

    if ( !m_pPosFile->isOpened() ){
        String strPosPath = getLogConf().getLogFilePath() + "_pos";
        m_pPosFile->open( strPosPath.c_str(), general::CRhoFile::OpenForReadWrite );
    }

    if ( !m_pPosFile->isOpened() )
        return;

    String strPos;
    m_pPosFile->movePosToStart();
    m_pPosFile->readString(strPos);
    if ( strPos.length() == 0 )
        return;

    m_nCirclePos = atoi(strPos.c_str());
    if ( m_nCirclePos < 0 || m_nCirclePos > m_nFileLogSize )
    	m_nCirclePos = -1;
    
    if ( m_nCirclePos >= 0 )
        m_pFile->setPosTo( m_nCirclePos );
}

void CLogFileSink::saveLogPosition(){
    if ( m_nCirclePos < 0 )
        return;

    if ( m_nCirclePos > getLogConf().getMaxLogFileSize() )
    	return;
    
    String strPos = general::convertToStringA(m_nCirclePos);
    for( int i = strPos.length(); i < 10; i++ )
    	strPos += ' ';
    
    m_pPosFile->movePosToStart();
    m_pPosFile->write( strPos.c_str(), strPos.length() );
    m_pPosFile->flush();
}

void CLogOutputSink::writeLogMessage( String& strMsg )
{
    if ( strMsg.length() > 1 && strMsg[strMsg.length()-2] == '\r' )
        strMsg.erase(strMsg.length()-2,1);

    const char* szMsg = strMsg.c_str(); 
#if defined( OS_WINDOWS ) //|| defined( OS_WINCE )
        ::OutputDebugStringA(strMsg.c_str());
//        printf(strMsg.c_str());
        for( int n = 0; n < strMsg.length(); n+= 100 )
            fwrite(strMsg.c_str()+n, 1, min(100,strMsg.length()-n) , stdout );

        fflush(stdout);
        
#elif defined( OS_SYMBIAN )
        
        TPtrC8 des((const TUint8*)szMsg);
      	RDebug::RawPrint(des);
#else
        //printf(strMsg.c_str());
        for( int n = 0; n < strMsg.length(); n+= 100 )
            fwrite(strMsg.c_str()+n, 1, min(100,strMsg.length()-n) , stdout );

        fflush(stdout);
#endif

}

}