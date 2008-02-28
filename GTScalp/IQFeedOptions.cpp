#include "StdAfx.h"
#include "IQFeedOptions.h"

// TODO:  convert OnNewResponse over to IQFeedRetrial

CIQFeedOptions::CIQFeedOptions(const char *szSymbol) {
  OpenPort();
  string s;
  s = "OEA,";
  s += szSymbol;
  s += ";";
  bLookingForDetail = true;
  //bs.Send( s.c_str() );
  m_pPort->SendToSocket( s.c_str() );
}

CIQFeedOptions::~CIQFeedOptions(void) {

  typedef string* pString;

  while ( !m_vOptionSymbols.empty() ) {
    pString &s = m_vOptionSymbols.back();
    delete s;
    m_vOptionSymbols.pop_back();
  }
}

void CIQFeedOptions::AddOptionSymbol( const char *s, unsigned short cnt ) {
  string *_s = new string( s, cnt );
  m_vOptionSymbols.push_back( _s );
}

void CIQFeedOptions::OnNewResponse( const char *szLine ) {
  if ( !bLookingForDetail ) {
    //CIQFeedRetrieval::OnNewResponse( szLine );
  }
  else {
    char *szSubStr = (char*) szLine;
    char *ixLine = (char*) szLine;
    unsigned short cnt = 0;

    while ( 0 != *ixLine ) {
      if ( ':' == *ixLine ) {
        if ( 0 != cnt ) {
          AddOptionSymbol( szSubStr, cnt );
          cnt = 0;
        }
        // switch from calls to puts
      }
      else {
        if ( ',' == *ixLine ) {
          if ( 0 != cnt ) {
            AddOptionSymbol( szSubStr, cnt );
            cnt = 0;
          }
        }
        else {
          // add to outstanding string
          if ( 0 == cnt ) {
            szSubStr = ixLine;
          }
          cnt++;
        }
      }
      ixLine++;
    }
    if ( 0 != cnt ) {
      if ( ' ' != *szSubStr ) {
        AddOptionSymbol( szSubStr, cnt );
        cnt = 0;
      }
    }
    if ( NULL != OnSymbolListReceived ) OnSymbolListReceived();
    ClosePort();
    bLookingForDetail = false;
  }
}
