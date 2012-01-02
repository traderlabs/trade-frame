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

#include <OUCommon/FastDelegate.h>
using namespace fastdelegate;

#define SYMBOL_PANELSIMULATIONCONTROL_STYLE wxTAB_TRAVERSAL
#define SYMBOL_PANELSIMULATIONCONTROL_TITLE _("Simulation Control")
#define SYMBOL_PANELSIMULATIONCONTROL_IDNAME ID_PANELSIMULATIONCONTROL
#define SYMBOL_PANELSIMULATIONCONTROL_SIZE wxSize(400, 300)
#define SYMBOL_PANELSIMULATIONCONTROL_POSITION wxDefaultPosition

class PanelSimulationControl: public wxPanel
{
public:

  PanelSimulationControl();
  PanelSimulationControl( 
    wxWindow* parent, 
    wxWindowID id = SYMBOL_PANELSIMULATIONCONTROL_IDNAME, 
    const wxPoint& pos = SYMBOL_PANELSIMULATIONCONTROL_POSITION, 
    const wxSize& size = SYMBOL_PANELSIMULATIONCONTROL_SIZE, 
    long style = SYMBOL_PANELSIMULATIONCONTROL_STYLE );
  ~PanelSimulationControl();

  bool Create( 
    wxWindow* parent, 
    wxWindowID id = SYMBOL_PANELSIMULATIONCONTROL_IDNAME, 
    const wxPoint& pos = SYMBOL_PANELSIMULATIONCONTROL_POSITION, 
    const wxSize& size = SYMBOL_PANELSIMULATIONCONTROL_SIZE, 
    long style = SYMBOL_PANELSIMULATIONCONTROL_STYLE );
  
  void Init();
  void CreateControls();

  static bool ShowToolTips() { return true; };
  wxBitmap GetBitmapResource( const wxString& name );
  wxIcon GetIconResource( const wxString& name );

  std::string GetInstrumentName( void) const { return m_txtInstrumentName->GetLineText( 0 ); };
  std::string GetGroupDirectory( void ) const { return m_txtGroupDirectory->GetLineText( 0 ); };

  typedef FastDelegate0<> OnStartSimulation_t;
  void SetOnStartSimulation( OnStartSimulation_t function ) {
    m_OnStartSimulation = function;
  }

  typedef FastDelegate0<> OnDrawChart_t;
  void SetOnDrawChart( OnDrawChart_t function ) {
    m_OnDrawChart = function;
  }

protected:
private:
  enum { ID_Null=wxID_HIGHEST, ID_PANELSIMULATIONCONTROL, 
    ID_TEXT_INSTRUMENTNAME, ID_TEXT_GROUPDIRECTORY, ID_BTN_STARTSIM, ID_BTN_DRAWCHART,
    ID_STATIC_RESULT, ID_GAUGE_PROGRESS
  };

  wxTextCtrl* m_txtInstrumentName;
  wxTextCtrl* m_txtGroupDirectory;
  wxStaticText* m_staticResult;
  wxGauge* m_gaugeProgress;

  OnStartSimulation_t m_OnStartSimulation;
  OnDrawChart_t m_OnDrawChart;

  void OnBtnStartSimulationClicked( wxCommandEvent& event );
  void OnBtnDrawChartClicked( wxCommandEvent& event );

};

