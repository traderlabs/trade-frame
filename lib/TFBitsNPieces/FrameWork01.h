/************************************************************************
 * Copyright(c) 2011, One Unified. All rights reserved.                 *
 *                                                                      *
 * This file is provided as is WITHOUT ANY WARRANTY                     *
 *  without even the implied warranty of                                *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
 *                                                                      *
 * This software may not be used nor distributed without proper license *
 * agreement.                                                           *
 *                                                                      *
 * See the file LICENSE.txt for redistribution information.             *
 ************************************************************************/

#pragma once

#include <TFInteractiveBrokers/IBTWS.h>
#include <TFIQFeed/IQFeedProvider.h>
#include <TFSimulation/SimulationProvider.h>

#include <TFVuTrading/PanelProviderControl.h>

namespace ou { // One Unified
namespace tf { // TradeFrame
  
template<typename CRTP>
class FrameWork01 {
public:

  enum Mode_t {
    EModeUnknown,
    EModeSimulation,
    EModeLive
  } m_eMode;

  FrameWork01(void);
  ~FrameWork01(void);

  Mode_t Mode( void ) const { return m_mode; };

protected:

  typedef ou::tf::CProviderInterfaceBase::pProvider_t pProvider_t;
  typedef ou::tf::eProviderState_t eProviderState_t;

  typedef ou::tf::CIBTWS::pProvider_t pProviderIBTWS_t;
  typedef ou::tf::CIQFeedProvider::pProvider_t pProviderIQFeed_t;
  typedef ou::tf::CSimulationProvider::pProvider_t pProviderSim_t;

  Mode_t m_mode;

  pProviderIQFeed_t m_iqfeed;
  bool m_bIQFeedConnected;

  pProviderIBTWS_t m_tws;
  bool m_bIBConnected;

  pProviderSim_t m_sim;
  bool m_bSimConnected;

  pProvider_t m_pData1Provider;
  bool m_bData1Connected;
  pProvider_t m_pData2Provider;
  bool m_bData2Connected;
  pProvider_t m_pExecutionProvider;
  bool m_bExecConnected;

  PanelProviderControl* m_pPanelProviderControl;

  void LinkToPanelProviderControl( void );
  void DelinkFromPanelProviderControl( void );

private:

  void SetMode( void );

  void HandleStateChangeRequest( ou::tf::eProviderState_t, bool&, pProvider_t );

  void HandleProviderStateChangeIQFeed( ou::tf::eProviderState_t );
  void HandleProviderStateChangeIB( ou::tf::eProviderState_t );
  void HandleProviderStateChangeSimulation( ou::tf::eProviderState_t ); 

  void HandleIBConnected( int );
  void HandleIQFeedConnected( int );
  void HandleSimulatorConnected( int );

  void HandleIBDisConnected( int );
  void HandleIQFeedDisConnected( int );
  void HandleSimulatorDisConnected( int );

  void HandleOnData1Connected( int );
  void HandleOnData1Disconnected( int );

  void HandleOnData2Connected( int );
  void HandleOnData2Disconnected( int );

  void HandleOnExecConnected( int );  // need to test for connection failure, when ib is not running
  void HandleOnExecDisconnected( int );

  void HandleProviderSelectD1( ou::tf::PanelProviderControl::Provider_t );
  void HandleProviderSelectD2( ou::tf::PanelProviderControl::Provider_t );
  void HandleProviderSelectX( ou::tf::PanelProviderControl::Provider_t );

  // for CRTP
  void OnIQFeedConnected( int ) {};
  void OnIBConnected( int ) {}; 
  void OnSimulatorConnected( int ) {};
  void OnIQFeedDisconnected( int ) {};
  void OnIBDisconnected( int ) {};
  void OnSimulatorDisconnected( int ) {};

  // for CRTP
  void OnData1Connected( int ) {};
  void OnData2Connected( int ) {};
  void OnExecConnected( int ) {};
  void OnData1Disconnected( int ) {};
  void OnData2Disconnteted( int ) {};
  void OnExecDisconnected( int ) {};

};

template<typename CRTP>
FrameWork01<CRTP>::FrameWork01( void ) :
  m_mode( EModeUnknown ),
  m_pPanelProviderControl( 0 ),
  m_tws( new ou::tf::CIBTWS( "U000000" ) ), m_bIBConnected( false ), 
  m_iqfeed( new ou::tf::CIQFeedProvider() ), m_bIQFeedConnected( false ),
  m_sim( new ou::tf::CSimulationProvider() ), m_bSimConnected( false )

{
  // this is where we select which provider we will be working with on this run
  // providers need to be registered in order for portfolio/position loading to function properly
  // key needs to match to account
  // ensure providers have been initialized above first
  CProviderManager::Instance().Register( "iq01", static_cast<pProvider_t>( m_iqfeed ) );
  CProviderManager::Instance().Register( "ib01", static_cast<pProvider_t>( m_tws ) );
  CProviderManager::Instance().Register( "sim01", static_cast<pProvider_t>( m_sim ) );

  m_iqfeed->OnConnected.Add( MakeDelegate( this, &FrameWork01::HandleIQFeedConnected ) );
  m_iqfeed->OnDisconnected.Add( MakeDelegate( this, &FrameWork01::HandleIQFeedDisConnected ) );

  m_tws->OnConnected.Add( MakeDelegate( this, &FrameWork01::HandleIBConnected ) );
  m_tws->OnDisconnected.Add( MakeDelegate( this, &FrameWork01::HandleIBDisConnected ) );

  m_sim->OnConnected.Add( MakeDelegate( this, &FrameWork01::HandleSimulatorConnected ) );
  m_sim->OnDisconnected.Add( MakeDelegate( this, &FrameWork01::HandleSimulatorDisConnected ) );
}

template<typename CRTP>
FrameWork01<CRTP>::~FrameWork01( void ) {

  m_iqfeed->OnConnected.Remove( MakeDelegate( this, &FrameWork01::HandleIQFeedConnected ) );
  m_iqfeed->OnDisconnected.Remove( MakeDelegate( this, &FrameWork01::HandleIQFeedDisConnected ) );

  m_tws->OnConnected.Remove( MakeDelegate( this, &FrameWork01::HandleIBConnected ) );
  m_tws->OnDisconnected.Remove( MakeDelegate( this, &FrameWork01::HandleIBDisConnected ) );

  m_sim->OnConnected.Remove( MakeDelegate( this, &FrameWork01::HandleSimulatorConnected ) );
  m_sim->OnDisconnected.Remove( MakeDelegate( this, &FrameWork01::HandleSimulatorDisConnected ) );

  ou::tf::CProviderManager::Instance().Release( "iq01" );
  ou::tf::CProviderManager::Instance().Release( "ib01" );
  ou::tf::CProviderManager::Instance().Release( "sim01" );

}

template<typename CRTP>
void FrameWork01<CRTP>::LinkToPanelProviderControl( void ) {
  m_pPanelProviderControl->SetOnIQFeedStateChangeHandler( MakeDelegate( this, &FrameWork01<CRTP>::HandleProviderStateChangeIQFeed ) );
  m_pPanelProviderControl->SetOnIBStateChangeHandler( MakeDelegate( this, &FrameWork01<CRTP>::HandleProviderStateChangeIB ) );
  m_pPanelProviderControl->SetOnSimulatorStateChangeHandler( MakeDelegate( this, &FrameWork01<CRTP>::HandleProviderStateChangeSimulation ) );

  m_pPanelProviderControl->SetOnProviderSelectD1Handler( MakeDelegate( this, &FrameWork01<CRTP>::HandleProviderSelectD1 ) );
  m_pPanelProviderControl->SetOnProviderSelectD2Handler( MakeDelegate( this, &FrameWork01<CRTP>::HandleProviderSelectD2 ) );
  m_pPanelProviderControl->SetOnProviderSelectXHandler( MakeDelegate( this, &FrameWork01<CRTP>::HandleProviderSelectX ) );
}

template<typename CRTP>
void FrameWork01<CRTP>::DelinkFromPanelProviderControl( void ) {
  m_pPanelProviderControl->SetOnProviderSelectD1Handler( 0 );
  m_pPanelProviderControl->SetOnProviderSelectD2Handler( 0 );
  m_pPanelProviderControl->SetOnProviderSelectXHandler( 0 );

  m_pPanelProviderControl->SetOnIQFeedStateChangeHandler( 0 );
  m_pPanelProviderControl->SetOnIBStateChangeHandler( 0 );
  m_pPanelProviderControl->SetOnSimulatorStateChangeHandler( 0 );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleProviderStateChangeIB( ou::tf::eProviderState_t state ) {
  HandleStateChangeRequest( state, m_bIBConnected, m_tws );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleProviderStateChangeIQFeed( ou::tf::eProviderState_t state ) {
  HandleStateChangeRequest( state, m_bIQFeedConnected, m_iqfeed );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleProviderStateChangeSimulation( ou::tf::eProviderState_t state ) {
  HandleStateChangeRequest( state, m_bSimConnected, m_sim );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleStateChangeRequest( eProviderState_t state, bool& flag, pProvider_t p ) {
  switch ( state ) {
  case eProviderState_t::ProviderOff:
    assert( !flag );
    break;
  case eProviderState_t::ProviderGoingOn:
    if ( !flag ) {
      {
        std::stringstream ss;
        ss.str( "" );
        ss << ou::CTimeSource::Instance().Internal();
//        m_sTSDataStreamOpened = "/app/semiauto/" + ss.str();  // will need to make this generic if need some for multiple providers.
      }
      p->Connect();
    }
    break;
  case eProviderState_t::ProviderOn:
    assert( flag );
    break;
  case eProviderState_t::ProviderGoingOff:
    if ( flag ) {
      p->Disconnect();
    }
    break;
  }
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleIQFeedConnected( int e ) {  // cross thread event
  m_bIQFeedConnected = true;
  if ( 0 != m_pPanelProviderControl )
    m_pPanelProviderControl->QueueEvent( new UpdateProviderStatusEvent( EVT_ProviderIQFeed, eProviderState_t::ProviderOn ) );
//  for ( vInstrumentData_t::iterator iter = m_vInstruments.begin(); iter != m_vInstruments.end(); ++iter ) {
//    iter->AddQuoteHandler( m_iqfeed );
//    iter->AddTradeHandler( m_iqfeed );
//  }
  static_cast<CRTP*>(this)->OnIQFeedConnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleIBConnected( int e ) { // cross thread event
  m_bIBConnected = true;
  if ( 0 != m_pPanelProviderControl )
    m_pPanelProviderControl->QueueEvent( new UpdateProviderStatusEvent( EVT_ProviderIB, eProviderState_t::ProviderOn ) );
  static_cast<CRTP*>(this)->OnIBConnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleSimulatorConnected( int e ) { // cross thread event
  m_bSimConnected = true;
  if ( 0 != m_pPanelProviderControl )
    m_pPanelProviderControl->QueueEvent( new UpdateProviderStatusEvent( EVT_ProviderSimulator, eProviderState_t::ProviderOn ) );
  static_cast<CRTP*>(this)->OnSimulatorConnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleIQFeedDisConnected( int e ) { // cross thread event
//  for ( vInstrumentData_t::iterator iter = m_vInstruments.begin(); iter != m_vInstruments.end(); ++iter ) {
//    iter->RemoveQuoteHandler( m_iqfeed );
//    iter->RemoveTradeHandler( m_iqfeed );
//  }
  m_bIQFeedConnected = false;
  if ( 0 != m_pPanelProviderControl )
    m_pPanelProviderControl->QueueEvent( new UpdateProviderStatusEvent( EVT_ProviderIQFeed, eProviderState_t::ProviderOff ) );
//  if ( EQuiescent != m_stateAcquisition ) {
//    m_stateAcquisition = EWriteData;
//  }
  static_cast<CRTP*>(this)->OnIQFeedDisconnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleIBDisConnected( int e ) {  // cross thread event
  m_bIBConnected = false;
  if ( 0 != m_pPanelProviderControl )
    m_pPanelProviderControl->QueueEvent( new UpdateProviderStatusEvent( EVT_ProviderIB, eProviderState_t::ProviderOff ) );
  static_cast<CRTP*>(this)->OnIBDisconnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleSimulatorDisConnected( int e ) {  // cross thread event
  m_bSimConnected = false;
  if ( 0 != m_pPanelProviderControl )
    m_pPanelProviderControl->QueueEvent( new UpdateProviderStatusEvent( EVT_ProviderSimulator, eProviderState_t::ProviderOff ) );
  static_cast<CRTP*>(this)->OnSimulatorDisconnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleOnData1Connected(int e) {
  m_bData1Connected = true;
//  HandleOnConnected(e);
//  CIQFeedHistoryQuery<CProcess>::Connect();  
  static_cast<CRTP*>(this)->OnData1Connected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleOnData1Disconnected(int e) {
  m_bData1Connected = false;
//  HandleOnConnected(e);
  static_cast<CRTP*>(this)->OnData1Disconnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleOnData2Connected(int e) {
//  CIQFeedHistoryQuery<CProcess>::Connect();  
  m_bData2Connected = true;
//  HandleOnConnected( e );
  static_cast<CRTP*>(this)->OnData2Connected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleOnData2Disconnected(int e) {
  m_bData2Connected = false;
//  HandleOnConnected( e );
  static_cast<CRTP*>(this)->OnData2Disconnteted( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleOnExecConnected(int e) {
  m_bExecConnected = true;
  static_cast<CRTP*>(this)->OnExecConnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleOnExecDisconnected(int e) {
  m_bExecConnected = false;
  static_cast<CRTP*>(this)->OnExecDisconnected( e );
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleProviderSelectD1( ou::tf::PanelProviderControl::Provider_t provider) {
  if ( 0 != m_pData1Provider.use_count() ) {
    m_pData1Provider->OnConnected.Remove( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData1Connected ) );
    m_pData1Provider->OnDisconnected.Remove( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData1Disconnected ) );
  }
  switch ( provider ) {
  case ou::tf::PanelProviderControl::Provider_t::EIQFeed:
    m_pData1Provider = m_iqfeed;
    break;
  case ou::tf::PanelProviderControl::Provider_t::EIB:
    m_pData1Provider = m_tws;
    break;
  case ou::tf::PanelProviderControl::Provider_t::ESim:
    m_pData1Provider = m_sim;
    break;
  }
  m_pData1Provider->OnConnected.Add( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData1Connected ) );
  m_pData1Provider->OnDisconnected.Add( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData1Disconnected ) );
  SetMode();
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleProviderSelectD2( ou::tf::PanelProviderControl::Provider_t provider ) {
  if ( 0 != m_pData2Provider.use_count() ) {
    m_pData2Provider->OnConnected.Remove( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData2Connected ) );
    m_pData2Provider->OnDisconnected.Remove( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData2Disconnected ) );
  }
  switch ( provider ) {
  case ou::tf::PanelProviderControl::Provider_t::EIQFeed:
    m_pData2Provider = m_iqfeed;
    break;
  case ou::tf::PanelProviderControl::Provider_t::EIB:
    m_pData2Provider = m_tws;
    break;
  case ou::tf::PanelProviderControl::Provider_t::ESim:
    m_pData2Provider = m_sim;
    break;
  }
  m_pData2Provider->OnConnected.Add( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData2Connected ) );
  m_pData2Provider->OnDisconnected.Add( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnData2Disconnected ) );
  SetMode();
}

template<typename CRTP>
void FrameWork01<CRTP>::HandleProviderSelectX( ou::tf::PanelProviderControl::Provider_t provider ) {
  if ( 0 != m_pExecutionProvider.use_count() ) {
    m_pExecutionProvider->OnConnected.Remove( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnExecConnected ) );
    m_pExecutionProvider->OnDisconnected.Remove( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnExecDisconnected ) );
  }
  switch ( provider ) {
  case ou::tf::PanelProviderControl::Provider_t::EIQFeed:
    m_pExecutionProvider = m_iqfeed;
    break;
  case ou::tf::PanelProviderControl::Provider_t::EIB:
    m_pExecutionProvider = m_tws;
    break;
  case ou::tf::PanelProviderControl::Provider_t::ESim:
    m_pExecutionProvider = m_sim;
    break;
  }
  m_pExecutionProvider->OnConnected.Add( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnExecConnected ) );
  m_pExecutionProvider->OnDisconnected.Add( MakeDelegate( this, &FrameWork01<CRTP>::HandleOnExecDisconnected ) );
  SetMode();
}

template<typename CRTP>
void FrameWork01<CRTP>::SetMode( void ) {
  m_mode = EModeUnknown;
  if ( ( 0 != m_pData1Provider.use_count() ) && ( 0 != m_pExecutionProvider.use_count() ) ) {
    if ( ( ou::tf::keytypes::EProviderSimulator == m_pData1Provider->ID() ) && ( ou::tf::keytypes::EProviderSimulator == m_pExecutionProvider->ID() ) ) {
      m_mode = EModeSimulation;
    }
    if ( 
      ( ( ou::tf::keytypes::EProviderIQF == m_pData1Provider->ID() ) || ( ou::tf::keytypes::EProviderIB == m_pData1Provider->ID() ) )
      && ( ou::tf::keytypes::EProviderIB == m_pExecutionProvider->ID() ) )
    {
      m_mode = EModeLive;
    }
  }
  
}

} // namespace tf
} // namespace ou

