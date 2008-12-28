#include "StdAfx.h"
#include "IBTWS.h"

#include "TWS\Contract.h"
#include "TWS\Order.h"
#include "TWS\OrderState.h"
#include "TWS\Execution.h"

#include <iostream>
#include <stdexcept>
#include <limits>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CIBTWS::CIBTWS( const string &acctCode, const string &address, UINT port ): 
  CProviderInterface(), 
    pTWS( NULL ),
    m_sAccountCode( acctCode ), m_sIPAddress( address ), m_nPort( port ), m_curTickerId( 0 )
{
  m_sName = "IB";
  m_nID = EProviderIB;
  CIBSymbol *p = NULL;
  m_vTickerToSymbol.push_back( p );  // first ticker is 1, so preload at position 0
}

CIBTWS::~CIBTWS(void) {
  Disconnect();
  m_vTickerToSymbol.clear();  // the provider class takes care of deleting CIBSymbol.
}

void CIBTWS::Connect() {
  if ( NULL == pTWS ) {
    pTWS = new EClientSocket( this );
    pTWS->eConnect( m_sIPAddress.c_str(), m_nPort );
    m_bConnected = true;
    OnConnected( 0 );
    pTWS->reqCurrentTime();
    pTWS->reqNewsBulletins( true );
    pTWS->reqOpenOrders();
    //ExecutionFilter filter;
    //pTWS->reqExecutions( filter );
    pTWS->reqAccountUpdates( true, "" );
  }
}

void CIBTWS::Disconnect() {
  // check to see if there are any watches happening, and get them disconnected
  if ( NULL != pTWS ) {
    m_bConnected = false;
    pTWS->eDisconnect();
    delete pTWS;
    pTWS = NULL;
    OnDisconnected( 0 );
    std::cout << "IB Disconnected " << std::endl;
  }
}

CSymbol *CIBTWS::NewCSymbol( const std::string &sSymbolName ) {
  TickerId ticker = ++m_curTickerId;
  CIBSymbol *pSymbol = new CIBSymbol( sSymbolName, ticker );
  m_vTickerToSymbol.push_back( pSymbol );
  return pSymbol;
}

void CIBTWS::StartQuoteWatch(CSymbol *pSymbol) {  // overridden from base class
  StartQuoteTradeWatch( pSymbol );
}

void CIBTWS::StopQuoteWatch(CSymbol *pSymbol) {  // overridden from base class
  StopQuoteTradeWatch( pSymbol );
}

void CIBTWS::StartTradeWatch(CSymbol *pSymbol) {  // overridden from base class
  StartQuoteTradeWatch( pSymbol );
}

void CIBTWS::StopTradeWatch(CSymbol *pSymbol) {  // overridden from base class
  StopQuoteTradeWatch( pSymbol );
}

void CIBTWS::StartQuoteTradeWatch( CSymbol *pSymbol ) {
  CIBSymbol *pIBSymbol = (CIBSymbol *) pSymbol;
  if ( !pIBSymbol->GetQuoteTradeWatchInProgress() ) {
    // start watch
    pIBSymbol->SetQuoteTradeWatchInProgress();
    Contract contract;
    contract.symbol = pSymbol->Name().c_str();
    contract.currency = "USD";
    contract.exchange = "SMART";
    contract.secType = "STK";
    //pTWS->reqMktData( pIBSymbol->GetTickerId(), contract, "100,101,104,165,221,225,236", false );
    pTWS->reqMktData( pIBSymbol->GetTickerId(), contract, "", false );
  }
}

void CIBTWS::StopQuoteTradeWatch( CSymbol *pSymbol ) {
  CIBSymbol *pIBSymbol = (CIBSymbol *) pSymbol;
  if ( pIBSymbol->QuoteWatchNeeded() || pIBSymbol->TradeWatchNeeded() ) {
    // don't do anything if either a quote or trade watch still in progress
  }
  else {
    // stop watch
    pTWS->cancelMktData( pIBSymbol->GetTickerId() );
    pIBSymbol->ResetQuoteTradeWatchInProgress();
  }
}

void CIBTWS::StartDepthWatch(CSymbol *pSymbol) {  // overridden from base class
  CIBSymbol *pIBSymbol = (CIBSymbol *) pSymbol;
  if ( !pIBSymbol->GetDepthWatchInProgress() ) {
    // start watch
    pIBSymbol->SetDepthWatchInProgress();
  }
}

void CIBTWS::StopDepthWatch(CSymbol *pSymbol) {  // overridden from base class
  CIBSymbol *pIBSymbol = (CIBSymbol *) pSymbol;
  if ( pIBSymbol->DepthWatchNeeded() ) {
  }
  else {
    // stop watch
    pIBSymbol->ResetDepthWatchInProgress();
  }
}

// indexed with InstrumentType::enumInstrumentTypes
const char *CIBTWS::szSecurityType[] = { "NULL", "STK", "OPT", "FUT", "FOP", "CASH", "IND" };
const char *CIBTWS::szOrderType[] = { "UNKN", "MKT", "LMT", "STP", "STPLMT", "NULL", 
                   "TRAIL", "TRAILLIMIT", "MKTCLS", "LMTCLS", "SCALE" };
//long CIBTWS::nOrderId = 1;

void CIBTWS::PlaceOrder( COrder *pOrder ) {
  CProviderInterface::PlaceOrder( pOrder ); // any underlying initialization
  Order twsorder;
  twsorder.orderId = pOrder->GetOrderId();

  //Contract contract2;
  //contract2.conId = 44678227;
  //pTWS->reqContractDetails( contract2 );

  Contract contract;
  contract.symbol = pOrder->GetInstrument()->GetSymbolName().c_str();
  contract.currency = pOrder->GetInstrument()->GetCurrencyName();
  contract.exchange = (*(pOrder->GetInstrument()->GetExchangeName())).c_str();
  contract.secType = szSecurityType[ pOrder->GetInstrument()->GetInstrumentType() ];
  CString s;
  switch ( pOrder->GetInstrument()->GetInstrumentType() ) {
    case InstrumentType::Stock:
      contract.exchange = "SMART";
      break;
    case InstrumentType::Future:
      s.Format( "%04d%02d", pOrder->GetInstrument()->GetExpiryYear(), pOrder->GetInstrument()->GetExpiryMonth() );
      contract.expiry = s;
      if ( "CBOT" == contract.exchange ) contract.exchange = "ECBOT";
      break;
  }
  twsorder.action = pOrder->GetOrderSideName();
  twsorder.totalQuantity = pOrder->GetQuantity();
  twsorder.orderType = szOrderType[ pOrder->GetOrderType() ];
  twsorder.tif = "DAY";
  //twsorder.goodAfterTime = "20080625 09:30:00";
  //twsorder.goodTillDate = "20080625 16:00:00";
  switch ( pOrder->GetOrderType() ) {
    case OrderType::Limit:
      twsorder.lmtPrice = pOrder->GetPrice1();
      twsorder.auxPrice = 0;
      break;
    case OrderType::Stop:
      twsorder.auxPrice = pOrder->GetPrice1();
      twsorder.auxPrice = 0;
      break;
    case OrderType::StopLimit:
      twsorder.lmtPrice = pOrder->GetPrice1();
      twsorder.auxPrice = pOrder->GetPrice2();
      break;
    default:
      twsorder.lmtPrice = 0;
      twsorder.auxPrice = 0;
  }
  twsorder.transmit = true;
  twsorder.outsideRth = pOrder->GetOutsideRTH();
  //twsorder.whatIf = true;
  pTWS->placeOrder( twsorder.orderId, contract, twsorder );
}

void CIBTWS::CancelOrder( COrder *pOrder ) {
  CProviderInterface::CancelOrder( pOrder );
  pTWS->cancelOrder( pOrder->GetOrderId() );
}

void CIBTWS::tickPrice( TickerId tickerId, TickType tickType, double price, int canAutoExecute) {
  CIBSymbol *pSym = m_vTickerToSymbol[ tickerId ];
  //std::cout << "tickPrice " << pSym->Name() << ", " << TickTypeStrings[tickType] << ", " << price << std::endl;
  pSym->AcceptTickPrice( tickType, price );
}

void CIBTWS::tickSize( TickerId tickerId, TickType tickType, int size) {
  CIBSymbol *pSym = m_vTickerToSymbol[ tickerId ];
  //std::cout << "tickSize " << pSym->Name() << ", " << TickTypeStrings[tickType] << ", " << size << std::endl;
  pSym->AcceptTickSize( tickType, size );
}

void CIBTWS::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
                                   double modelPrice, double pvDividend) {
  std::cout << "tickOptionComputation" << ", " << TickTypeStrings[tickType] << std::endl; 
}

void CIBTWS::tickGeneric(TickerId tickerId, TickType tickType, double value) {
  std::cout << "tickGeneric " << m_vTickerToSymbol[ tickerId ]->Name() << ", " << TickTypeStrings[tickType] << ", " << value << std::endl;
}

void CIBTWS::tickString(TickerId tickerId, TickType tickType, const CString& value) {
  CIBSymbol *pSym = m_vTickerToSymbol[ tickerId ];
  //std::cout << "tickString " << pSym->Name() << ", " 
  //  << TickTypeStrings[tickType] << ", " << value;
  //std::cout << std::endl;
  pSym->AcceptTickString( tickType, value );
}

void CIBTWS::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const CString& formattedBasisPoints,
  double totalDividends, int holdDays, const CString& futureExpiry, double dividendImpact, double dividendsToExpiry ) {
  std::cout << "tickEFP" << std::endl;
}

void CIBTWS::orderStatus( OrderId orderId, const CString &status, int filled,
                         int remaining, double avgFillPrice, int permId, int parentId,
                         double lastFillPrice, int clientId, const CString& whyHeld) 
{
  if ( true ) {
    std::cout 
      << "OrderStatus: ordid=" << orderId 
      << ", stat=" << status 
      << ", fild=" << filled 
      << ", rem=" << remaining 
      << ", avgfillprc=" << avgFillPrice 
      << ", permid=" << permId 
      //<< ", parentid=" << parentId 
      << ", lfp=" << lastFillPrice 
      //<< ", clid=" << clientId 
      //<< ", yh=" << whyHeld 
      << std::endl;
  }
}

void CIBTWS::openOrder( OrderId orderId, const Contract& contract, const Order& order, const OrderState& state) {
  if ( order.whatIf ) {
    std::cout << "WhatIf:  ordid=" << orderId << ", cont.sym=" << contract.symbol
      << ", state.commission=" << state.commission
      << " " << state.commissionCurrency
      << ", state.equitywithloan=" << state.equityWithLoan 
      << ", state.initmarg=" << state.initMargin
      << ", state.maintmarg=" << state.maintMargin
      << ", state.maxcom=" << state.maxCommission
      << ", state.mincom=" << state.minCommission 
      << std::endl;
  }
  else { 
    std::cout 
      << "OpenOrder: ordid=" << orderId 
      << ", state.stat=" << state.status 
      << ", cont.sym=" << contract.symbol 
      << ", order.action=" << order.action 
      << ", state.commission=" << state.commission
      << " " << state.commissionCurrency
      //<< ", ord.id=" << order.orderId 
      //<< ", ord.ref=" << order.orderRef 
      << ", state.warning=" << state.warningText 
      << std::endl; 
    //if ( std::numeric_limits<double>::max(0) != state.commission ) 
    if ( 1e308 > state.commission ) 
      m_OrderManager.ReportCommission( orderId, state.commission ); 
  }
  if ( state.warningText != "" ) std::cout << "Open Order Warning: " << state.warningText << std::endl;
}

void CIBTWS::execDetails( OrderId orderId, const Contract& contract, const Execution& execution) {

  std::cout 
    << "execDetails: " 
    << "  sym=" << contract.symbol 
    << ", oid=" << orderId 
    //<< ", ex.oid=" << execution.orderId 
    << ", ex.pr=" << execution.price 
    << ", ex.sh=" << execution.shares 
    << ", ex.sd=" << execution.side 
    << ", ex.ti=" << execution.time 
    << ", ex.ex=" << execution.exchange
    //<< ", ex.liq=" << execution.liquidation
    << ", ex.pid=" << execution.permId
    << ", ex.acct=" << execution.acctNumber
    //<< ", ex.clid=" << execution.clientId
    << ", ex.xid=" << execution.execId
    << std::endl;

  OrderSide::enumOrderSide side = OrderSide::Unknown;
  if ( "BOT" == execution.side ) side = OrderSide::Buy;  // could try just first character for fast comparison
  if ( "SLD" == execution.side ) side = OrderSide::Sell;
  if ( OrderSide::Unknown == side ) std::cout << "Unknown execution side: " << execution.side << std::endl;
  else {
    CExecution exec( orderId, execution.price, execution.shares, side, 
      LPCTSTR( execution.exchange ), LPCTSTR( execution.execId ) );
    m_OrderManager.ReportExecution( exec );
  }
}

/*

OrderStatus: ordid=1057, stat=Filled, fild=200, rem=0, avgfillprc=117, permid=2147126208, parentid=0, lfp=117, clid=0, yh=
OpenOrder: ordid=1057, cont.sym=ICE, ord.id=1057, ord.ref=, state.stat=Filled, state.warning=, order.action=BUY, state.commission=1 USD
OrderStatus: ordid=1057, stat=Filled, fild=200, rem=0, avgfillprc=117, permid=2147126208, parentid=0, lfp=117, clid=0, yh=
OpenOrder: ordid=1057, cont.sym=ICE, ord.id=1057, ord.ref=, state.stat=Filled, state.warning=, order.action=BUY, state.commission=1 USD
execDetails:   sym=ICE, oid=1057, ex.oid=1057, ex.pr=117, ex.sh=100, ex.sd=BOT, ex.ti=20080607  12:39:55, ex.ex=NYSE, ex.liq=0, ex.pid=2147126208, ex.acct=DU15067, ex.clid=0, ex.xid=0000ea6a.44f438e4.01.01
OrderStatus: ordid=1057, stat=Submitted, fild=100, rem=100, avgfillprc=117, permid=2147126208, parentid=0, lfp=117, clid=0, yh=
OpenOrder: ordid=1057, cont.sym=ICE, ord.id=1057, ord.ref=, state.stat=Submitted, state.warning=, order.action=BUY, state.commission=1 USD
OrderStatus: ordid=1057, stat=Submitted, fild=100, rem=100, avgfillprc=117, permid=2147126208, parentid=0, lfp=117, clid=0, yh=
OpenOrder: ordid=1057, cont.sym=ICE, ord.id=1057, ord.ref=, state.stat=Submitted, state.warning=, order.action=BUY, state.commission=1.79769e+308 USD
execDetails:   sym=ICE, oid=1057, ex.oid=1057, ex.pr=117, ex.sh=100, ex.sd=BOT, ex.ti=20080607  12:39:14, ex.ex=NYSE, ex.liq=0, ex.pid=2147126208, ex.acct=DU15067, ex.clid=0, ex.xid=0000ea6a.44f438d5.01.01
current time 1212851947
*/

void CIBTWS::error(const int id, const int errorCode, const CString errorString) {
  std::cout << "error " << id << ", " << errorCode << ", " << errorString << std::endl;
  switch ( errorCode ) {
    case 1102: // Connectivity has been restored
      pTWS->reqAccountUpdates( true, "" );
      break;
  }
}

void CIBTWS::winError( const CString &str, int lastError) {
  std::cout << "winerror " << str << ", " << lastError << std::endl;
}

void CIBTWS::updateNewsBulletin(int msgId, int msgType, const CString& newsMessage, const CString& originExch) {
  std::cout << "news bulletin " << msgId << ", " << msgType << ", " << newsMessage << ", " << originExch << std::endl;
}

void CIBTWS::currentTime(long time) {
  std::cout << "current time " << time << endl;
  m_time = time;
}

void CIBTWS::updateAccountTime(const CString& timeStamp) {
}

void CIBTWS::contractDetails( const ContractDetails& contractDetails) {
  std::cout << "contract Details " << 
    contractDetails.orderTypes << ", " << contractDetails.minTick << std::endl;
}

void CIBTWS::bondContractDetails( const ContractDetails& contractDetails) {
}

void CIBTWS::nextValidId( OrderId orderId) {
  std::cout << "next valid id " << orderId << std::endl;
}

void CIBTWS::updatePortfolio( const Contract& contract, int position,
      double marketPrice, double marketValue, double averageCost,
      double unrealizedPNL, double realizedPNL, const CString& accountName) {
  if ( true ) {
    std::cout << "portfolio " 
      << contract.symbol 
      << ", type=" << contract.secType 
      << ", strike=" << contract.strike
      << ", expire=" << contract.expiry
      << ", right=" << contract.right
      << ", pos=" << position
      << ", price=" << marketPrice
      << ", val=" << marketValue
      << ", cost=" << averageCost
      << ", uPL=" << unrealizedPNL
      << ", rPL=" << realizedPNL
      //<< ", " << accountName 
      << std::endl;
  }
}

// todo: use the keyword lookup to make this faster
//   key, bool, double, string
void CIBTWS::updateAccountValue(const CString& key, const CString& val,
                                const CString& currency, const CString& accountName) {
  bool bEmit = false;
  if ( "AccountCode" == key ) bEmit = true;
  if ( "AccountReady" == key ) bEmit = true;
  if ( "AccountType" == key ) bEmit = true;
  if ( "AvailableFunds" == key ) bEmit = true;
  if ( "BuyingPower" == key ) {
    m_dblBuyingPower = atof( (LPCTSTR) val );
    bEmit = true;
    //std::cout << "**Buying Power " << m_dblBuyingPower << std::endl;
  }
  if ( "CashBalance" == key ) bEmit = true;
  if ( "Cushion" == key ) bEmit = true;
  if ( "GrossPositionValue" == key ) bEmit = true;
  if ( "PNL" == key ) bEmit = true;
  if ( "UnrealizedPnL" == key ) bEmit = true;
  if ( "SMA" == key ) bEmit = true;
  if ( "AvailableFunds" == key ) {
    m_dblAvailableFunds = atof( (LPCTSTR) val );
    bEmit = true;
  }
  if ( "MaintMarginReq" == key ) bEmit = true;
  if ( "InitMarginReq" == key ) bEmit = true;
  if ( bEmit ) {
    //std::cout << "account value " << key << ", " << val << ", " << currency << ", " << accountName << std::endl;
  }
}

void CIBTWS::connectionClosed() {
  std::cout << "connection closed" << std::endl;
}

void CIBTWS::updateMktDepth(TickerId id, int position, int operation, int side,
                            double price, int size) {
}

void CIBTWS::updateMktDepthL2(TickerId id, int position, CString marketMaker, int operation,
                              int side, double price, int size) {
}

void CIBTWS::managedAccounts( const CString& accountsList) {
}

void CIBTWS::receiveFA(faDataType pFaDataType, const CString& cxml) {
}

void CIBTWS::historicalData(TickerId reqId, const CString& date, double open, double high, 
                            double low, double close, int volume, int barCount, double WAP, int hasGaps) {
}

void CIBTWS::scannerParameters(const CString &xml) {
}

void CIBTWS::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
                         const CString &distance, const CString &benchmark, const CString &projection,
                         const CString &legsStr) {
}

void CIBTWS::scannerDataEnd(int reqId) {
}

void CIBTWS::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
                         long volume, double wap, int count) {
}

// From EWrapper.h
char *CIBTWS::TickTypeStrings[] = {
  "BID_SIZE", "BID", "ASK", "ASK_SIZE", "LAST", "LAST_SIZE",
				"HIGH", "LOW", "VOLUME", "CLOSE",
				"BID_OPTION_COMPUTATION", 
				"ASK_OPTION_COMPUTATION", 
				"LAST_OPTION_COMPUTATION",
				"MODEL_OPTION",
				"OPEN",
				"LOW_13_WEEK",
				"HIGH_13_WEEK",
				"LOW_26_WEEK",
				"HIGH_26_WEEK",
				"LOW_52_WEEK",
				"HIGH_52_WEEK",
				"AVG_VOLUME",
				"OPEN_INTEREST",
				"OPTION_HISTORICAL_VOL",
				"OPTION_IMPLIED_VOL",
				"OPTION_BID_EXCH",
				"OPTION_ASK_EXCH",
				"OPTION_CALL_OPEN_INTEREST",
				"OPTION_PUT_OPEN_INTEREST",
				"OPTION_CALL_VOLUME",
				"OPTION_PUT_VOLUME",
				"INDEX_FUTURE_PREMIUM",
				"BID_EXCH",
				"ASK_EXCH",
				"AUCTION_VOLUME",
				"AUCTION_PRICE",
				"AUCTION_IMBALANCE",
				"MARK_PRICE",
				"BID_EFP_COMPUTATION",
				"ASK_EFP_COMPUTATION",
				"LAST_EFP_COMPUTATION",
				"OPEN_EFP_COMPUTATION",
				"HIGH_EFP_COMPUTATION",
				"LOW_EFP_COMPUTATION",
				"CLOSE_EFP_COMPUTATION",
				"LAST_TIMESTAMP",
				"SHORTABLE",
				"NOT_SET"
};