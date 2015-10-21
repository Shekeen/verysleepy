/*=====================================================================
optionsdlg.cpp
----------------

Copyright (C) Richard Mitton

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

http://www.gnu.org/copyleft/gpl.html.
=====================================================================*/
#include "optionsdlg.h"
#include <wx/filepicker.h>
#include <wx/msw/wrapcctl.h> // include <commctrl.h> "properly"
#include <wx/valnum.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textdlg.h>

class wxPercentSlider : public wxSlider
{
public:
    wxPercentSlider(wxWindow *parent,
             wxWindowID id,
             int value,
             int minValue,
             int maxValue,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxSL_HORIZONTAL,
             const wxValidator& validator = wxDefaultValidator,
             const wxString& name = wxSliderNameStr)
    {
        Init();

        Create(parent, id, value, minValue, maxValue, pos, size, style, validator, name);
    }

protected:

	// RJM- had to modify the wxWidgets source slightly to get this to work :-/
	// Just go into include/wx/msw/slider.h, and change the definition of this
	// from 'static' to 'virtual', and add a const modifier. Then you have to do
	// a full 'make clean' and rebuild for it to take.
    virtual wxString Format(int n) const { return wxString::Format(wxT("%d%%"), n); }
};

enum OptionsId
{
	Options_UseSymServer = 1,
	Options_Throttle,
	Options_SymPath,
	Options_SymPath_Add,
	Options_SymPath_Remove,
	Options_SymPath_MoveUp,
	Options_SymPath_MoveDown,
  Options_SrvPath,
  Options_SrvPath_Add,
  Options_SrvPath_Remove,
  Options_SrvPath_MoveUp,
  Options_SrvPath_MoveDown,
	Options_SaveMinidump,
};

BEGIN_EVENT_TABLE(OptionsDlg, wxDialog)
EVT_BUTTON(wxID_OK, OptionsDlg::OnOk)
EVT_CHECKBOX(Options_UseSymServer, OptionsDlg::OnUseSymServer)
EVT_LISTBOX(Options_SymPath, OptionsDlg::OnSymPath)
EVT_BUTTON(Options_SymPath_Add, OptionsDlg::OnSymPathAdd)
EVT_BUTTON(Options_SymPath_Remove, OptionsDlg::OnSymPathRemove)
EVT_BUTTON(Options_SymPath_MoveUp, OptionsDlg::OnSymPathMoveUp)
EVT_BUTTON(Options_SymPath_MoveDown, OptionsDlg::OnSymPathMoveDown)
EVT_LISTBOX(Options_SrvPath, OptionsDlg::OnSrvPath)
EVT_BUTTON(Options_SrvPath_Add, OptionsDlg::OnSrvPathAdd)
EVT_BUTTON(Options_SrvPath_Remove, OptionsDlg::OnSrvPathRemove)
EVT_BUTTON(Options_SrvPath_MoveUp, OptionsDlg::OnSrvPathMoveUp)
EVT_BUTTON(Options_SrvPath_MoveDown, OptionsDlg::OnSrvPathMoveDown)
EVT_CHECKBOX(Options_SaveMinidump, OptionsDlg::OnSaveMinidump)
END_EVENT_TABLE()

OptionsDlg::OptionsDlg()
:	wxDialog(NULL, -1, wxString(_T("Options")),
			 wxDefaultPosition, wxDefaultSize,
			 wxDEFAULT_DIALOG_STYLE)
{
	wxBoxSizer *rootsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer *symsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Symbols");
	wxStaticBoxSizer *symdirsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Symbol search path");
	wxStaticBoxSizer *symsrvsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Symbol server");

	symPaths = new wxListBox(this, Options_SymPath, wxDefaultPosition, wxSize(-1, 75), 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB);

	wxBoxSizer *symPathSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *symPathButtonSizer = new wxBoxSizer(wxVERTICAL);

	static const struct { wxButton * OptionsDlg::* button; OptionsId id; const wchar_t *icon; const char *tip; } symPathButtons[] = {
		{ &OptionsDlg::symPathAdd     , Options_SymPath_Add     , L"button_add"   , "Browse for a directory to add" },
		{ &OptionsDlg::symPathRemove  , Options_SymPath_Remove  , L"button_remove", "Remove selected directory"     },
		{ &OptionsDlg::symPathMoveUp  , Options_SymPath_MoveUp  , L"button_up"    , "Move selected directory up"    },
		{ &OptionsDlg::symPathMoveDown, Options_SymPath_MoveDown, L"button_down"  , "Move selected directory down"  },
	};
	for (size_t n=0; n<_countof(symPathButtons); n++)
	{
		wxButton *b = this->*symPathButtons[n].button = new wxButton(
			this,
			symPathButtons[n].id,
			wxEmptyString,
			wxDefaultPosition,
			wxSize(20, 20),
			wxBU_EXACTFIT);
		b->SetBitmap(LoadPngResource(symPathButtons[n].icon));
		b->SetToolTip(symPathButtons[n].tip);
		symPathButtonSizer->Add(b, 1);
	}
	UpdateSymPathButtons();

	symPathSizer->Add(symPaths, 100, wxEXPAND);
	symPathSizer->Add(symPathButtonSizer, 1, wxSHRINK);

	useSymServer = new wxCheckBox(this, Options_UseSymServer, "Use symbol servers");
	symCacheDir = new wxDirPickerCtrl(this, -1, prefs.symCacheDir, "Select a directory to store local symbols in:",
		wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL);
  
  srvPaths = new wxListBox(this, Options_SrvPath, wxDefaultPosition, wxSize(-1, 75), 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB);

  wxBoxSizer *srvPathSizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *srvPathButtonSizer = new wxBoxSizer(wxVERTICAL);

  static const struct { wxButton * OptionsDlg::* button; OptionsId id; const wchar_t *icon; const char *tip; } srvPathButtons[] = {
    { &OptionsDlg::srvPathAdd     , Options_SrvPath_Add     , L"button_add"   , "Add a new symbol server"   },
    { &OptionsDlg::srvPathRemove  , Options_SrvPath_Remove  , L"button_remove", "Remove selected server"    },
    { &OptionsDlg::srvPathMoveUp  , Options_SrvPath_MoveUp  , L"button_up"    , "Move selected server up"   },
    { &OptionsDlg::srvPathMoveDown, Options_SrvPath_MoveDown, L"button_down"  , "Move selected server down" },
  };

  for (size_t n = 0; n < _countof(srvPathButtons); n++)
  {
    wxButton *b = this->*srvPathButtons[n].button = new wxButton(
      this,
      srvPathButtons[n].id,
      wxEmptyString,
      wxDefaultPosition,
      wxSize(20, 20),
      wxBU_EXACTFIT);
    b->SetBitmap(LoadPngResource(srvPathButtons[n].icon));
    b->SetToolTip(srvPathButtons[n].tip);
    srvPathButtonSizer->Add(b, 1);
  }
  
  srvPathSizer->Add(srvPaths, 100, wxEXPAND);
  srvPathSizer->Add(srvPathButtonSizer, 1, wxSHRINK);

	wxBoxSizer *minGwDbgHelpSizer = new wxBoxSizer(wxHORIZONTAL);
	minGwDbgHelpSizer->Add(new wxStaticText(this, -1, "MinGW DbgHelp engine:   "));

	mingwWine = new wxRadioButton(this, -1, "Wine  ");
	mingwWine->SetToolTip("Use Wine's DbgHelp implementation for MinGW symbols (dbghelpw.dll).");
	minGwDbgHelpSizer->Add(mingwWine);

	mingwDrMingw = new wxRadioButton(this, -1, "Dr. MinGW");
	mingwDrMingw->SetToolTip("Use Dr. MinGW's DbgHelp implementation for MinGW symbols (dbghelpdr.dll).");
	minGwDbgHelpSizer->Add(mingwDrMingw);

	(prefs.useWine ? mingwWine : mingwDrMingw)->SetValue(true);

	wxBoxSizer *saveMinidumpSizer = new wxBoxSizer(wxHORIZONTAL);

	saveMinidump = new wxCheckBox(this, Options_SaveMinidump, "Save minidump after ");
	saveMinidump->SetToolTip(
		"Include a minidump in saved profiling results.\n"
		"This enables the \"Load symbols from minidump\" option,\n"
		"which allows profiling an application on a user machine without symbols,\n"
		"then examining the profile results on a developer machine with symbols.");
	saveMinidumpSizer->Add(saveMinidump);

	saveMinidumpTimeValue = prefs.saveMinidump < 0 ? 0 : prefs.saveMinidump;
	saveMinidumpTime = new wxTextCtrl(
		this, -1,
		wxEmptyString, wxDefaultPosition,
		wxSize(40, -1),
		0,
		wxIntegerValidator<int>(&saveMinidumpTimeValue));
	saveMinidumpTime->SetToolTip(
		"Saving a minidump at the very start of a profiling session\n"
		"may not work as expected, as not all DLLs may be loaded yet.\n"
		"You can set a delay after which a minidump will be saved.");
	saveMinidumpTime->Enable(prefs.saveMinidump >= 0);
	saveMinidumpSizer->Add(saveMinidumpTime, 0, wxTOP, -3);
	saveMinidumpSizer->Add(new wxStaticText(this, -1, " seconds"));

  wxArrayString symSearchPaths = wxSplit(prefs.symSearchPath, ';', 0);
  for (size_t i = 0; i < symSearchPaths.GetCount(); ++i)
  {
    if (symSearchPaths[i].StartsWith(L"SRV*"))
      srvPaths->Append(wxSplit(symSearchPaths[i], '*', 0)[2]);
    else
      symPaths->Append(symSearchPaths[i]);
  }

	useSymServer->SetValue(prefs.useSymServer);
	symCacheDir->Enable(prefs.useSymServer);
	saveMinidump->SetValue(prefs.saveMinidump >= 0);

  UpdateSrvPathButtons();

  symdirsizer->Add(symPathSizer, 0, wxALL | wxEXPAND, 5);

	symsrvsizer->Add(useSymServer, 0, wxALL, 5);
  symsrvsizer->Add(srvPathSizer, 0, wxALL|wxEXPAND, 5);
	symsrvsizer->Add(new wxStaticText(this, -1, "Local cache directory:"), 0, wxLEFT|wxTOP, 5);
	symsrvsizer->Add(symCacheDir, 0, wxALL|wxEXPAND, 5);

	symsizer->Add(symdirsizer, 0, wxALL|wxEXPAND, 5);
	symsizer->Add(symsrvsizer, 0, wxALL|wxEXPAND, 5);
	symsizer->Add(minGwDbgHelpSizer, 0, wxALL, 5);
	symsizer->Add(saveMinidumpSizer, 0, wxALL, 5);

	wxStaticBoxSizer *throttlesizer = new wxStaticBoxSizer(wxVERTICAL, this, "Sample rate control");
	throttle = new wxPercentSlider(this, Options_Throttle, prefs.throttle, 1, 100, wxDefaultPosition, wxDefaultSize,
		wxSL_HORIZONTAL|wxSL_TICKS|wxSL_TOP|wxSL_LABELS);
	throttle->SetTickFreq(10);
	throttlesizer->Add(new wxStaticText(this, -1,
		"Adjusts the sample rate speed. Useful for doing longer captures\n"
		"where you wish to reduce the profiler overhead.\n"
		"Higher values increase accuracy; lower values result in better\n"
		"performance."), 0, wxALL, 5);
	throttlesizer->Add(throttle, 0, wxEXPAND|wxLEFT|wxTOP, 5);

	topsizer->Add(symsizer, 0, wxEXPAND|wxALL, 0);
	topsizer->AddSpacer(5);
	topsizer->Add(throttlesizer, 0, wxEXPAND|wxALL, 0);
	rootsizer->Add(topsizer, 1, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 10);
	rootsizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxALL, 10);
	SetSizer(rootsizer);
	rootsizer->SetSizeHints(this);
	SetAutoLayout(TRUE);

	SetSize(wxSize(400, -1));
	Centre();
}

OptionsDlg::~OptionsDlg()
{
}

void OptionsDlg::OnOk(wxCommandEvent& event)
{
	if ( Validate() && TransferDataFromWindow() )
	{
    prefs.symSearchPath = ConstructSymSearchPath();
		prefs.useSymServer = useSymServer->GetValue();
		prefs.symCacheDir = symCacheDir->GetPath();
		prefs.useWine = mingwWine->GetValue();
		prefs.saveMinidump = saveMinidump->GetValue() ? saveMinidumpTimeValue : -1;
		prefs.throttle = throttle->GetValue();
		EndModal(wxID_OK);
	}
}

void OptionsDlg::OnUseSymServer(wxCommandEvent& event)
{
	bool enabled = useSymServer->GetValue();
	symCacheDir->Enable(enabled);
  srvPaths->Enable(enabled);
  UpdateSrvPathButtons();
}

void OptionsDlg::UpdateSymPathButtons()
{
	int sel = symPaths->GetSelection();
	symPathRemove  ->Enable(sel >= 0);
	symPathMoveUp  ->Enable(sel > 0);
	symPathMoveDown->Enable(sel >= 0 && sel < (int)symPaths->GetCount()-1);
}

void OptionsDlg::OnSymPath( wxCommandEvent & event )
{
	UpdateSymPathButtons();
}

void OptionsDlg::OnSymPathAdd( wxCommandEvent & event )
{
	wxDirDialog dlg(this, "Select a symbol search path to add");
	if (dlg.ShowModal()==wxID_OK)
	{
		int sel = symPaths->Append(dlg.GetPath());
		symPaths->Select(sel);
		UpdateSymPathButtons();
	}
}

void OptionsDlg::OnSymPathRemove( wxCommandEvent & event )
{
	int sel = symPaths->GetSelection();
	symPaths->Delete(sel);
	if (sel < (int)symPaths->GetCount())
		symPaths->Select(sel);
	UpdateSymPathButtons();
}

void OptionsDlg::OnSymPathMoveUp( wxCommandEvent & event )
{
	int sel = symPaths->GetSelection();
	wxString s = symPaths->GetString(sel);
	symPaths->Delete(sel);
	symPaths->Insert(s, sel-1);
	symPaths->Select(sel-1);
	UpdateSymPathButtons();
}

void OptionsDlg::OnSymPathMoveDown( wxCommandEvent & event )
{
	int sel = symPaths->GetSelection();
	wxString s = symPaths->GetString(sel);
	symPaths->Delete(sel);
	symPaths->Insert(s, sel+1);
	symPaths->Select(sel+1);
	UpdateSymPathButtons();
}

void OptionsDlg::UpdateSrvPathButtons()
{
  if (useSymServer->IsChecked()) {
    int sel = srvPaths->GetSelection();
    srvPathAdd->Enable(true);
    srvPathRemove->Enable(sel >= 0);
    srvPathMoveUp->Enable(sel > 0);
    srvPathMoveDown->Enable(sel >= 0 && sel < (int)srvPaths->GetCount() - 1);
  }
  else {
    srvPathAdd->Enable(false);
    srvPathRemove->Enable(false);
    srvPathMoveUp->Enable(false);
    srvPathMoveDown->Enable(false);
  }
}

void OptionsDlg::OnSrvPath(wxCommandEvent & event)
{
  UpdateSrvPathButtons();
}

void OptionsDlg::OnSrvPathAdd(wxCommandEvent & event)
{
  wxTextEntryDialog dlg(this, "Enter a symbol server address.");
  if (dlg.ShowModal() == wxID_OK)
  {
    int sel = srvPaths->Append(dlg.GetValue());
    srvPaths->Select(sel);
    UpdateSrvPathButtons();
  }
}

void OptionsDlg::OnSrvPathRemove(wxCommandEvent & event)
{
  int sel = srvPaths->GetSelection();
  srvPaths->Delete(sel);
  if (sel < (int)srvPaths->GetCount())
    srvPaths->Select(sel);
  UpdateSrvPathButtons();
}

void OptionsDlg::OnSrvPathMoveUp(wxCommandEvent & event)
{
  int sel = srvPaths->GetSelection();
  wxString s = srvPaths->GetString(sel);
  srvPaths->Delete(sel);
  srvPaths->Insert(s, sel - 1);
  srvPaths->Select(sel - 1);
  UpdateSrvPathButtons();
}

void OptionsDlg::OnSrvPathMoveDown(wxCommandEvent & event)
{
  int sel = srvPaths->GetSelection();
  wxString s = srvPaths->GetString(sel);
  srvPaths->Delete(sel);
  srvPaths->Insert(s, sel + 1);
  srvPaths->Select(sel + 1);
  UpdateSrvPathButtons();
}

void OptionsDlg::OnSaveMinidump( wxCommandEvent & event )
{
	saveMinidumpTime->Enable(saveMinidump->IsChecked());
}

wxString OptionsDlg::ConstructSymSearchPath()
{
  wxString localSymSearchPath = wxJoin(symPaths->GetStrings(), ';', 0);

  if (!useSymServer->IsChecked())
    return localSymSearchPath;

  wxString symCache = symCacheDir->GetPath();
  wxArrayString serverSymSearchPaths;
  wxArrayString servers = srvPaths->GetStrings();
  for (size_t i = 0; i < servers.GetCount(); ++i)
    serverSymSearchPaths.Add(L"SRV*" + symCache + L"*" + servers[i]);
  wxString serverSymSearchPath = wxJoin(serverSymSearchPaths, ';', 0);

  if (localSymSearchPath.IsEmpty())
    return serverSymSearchPath;
  else if (serverSymSearchPath.IsEmpty())
    return localSymSearchPath;
  else
    return localSymSearchPath + L";" + serverSymSearchPath;
}