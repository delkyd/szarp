/* SZARP: SCADA software
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
 * szApp - base class for all SZARP wx applications
 * SZARP

 * Pawe� Pa�ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#include "szapp.h"
#include "cconv.h"
#include "version.h"

#include <wx/log.h>

#ifdef __WXGTK__

#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */
	extern void gtk_rc_add_default_file(const gchar *filename);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WXGTK__ */

#include <wx/dir.h>
#include <wx/config.h>
#include <wx/dialog.h>
#include <wx/xrc/xmlres.h>
#include "cconv.h"

#ifdef __WXMSW__
#include <windows.h>
#endif

#include "aboutdlg.h"
#include "szframe.h"

extern void InitCommonXmlResource();



bool szAppImpl::OnInit()
{
	wxXmlResource::Get()->InitAllHandlers();
	InitCommonXmlResource();
	return true;
}

szAppImpl::szAppImpl() : has_data_dir(false)
{
#ifdef __WXGTK__
	char* env = getenv("HOME");
	if (env == NULL) {
		return;
	}
	char* rcpath;
	asprintf(&rcpath, "%s/.szarp/gtk.rc", env);
	gtk_rc_add_default_file(rcpath);
#endif /* __WXGTK__ */

	m_releasedate = _T("2007 - 2010");
	m_version = _T(SZARP_VERSION);
}

wxString szAppImpl::GetSzarpDir()
{
#ifdef __WXMSW__
	wxDir dir;

	wxString dirpath = wxPathOnly(m_path);

	if (dirpath.IsEmpty())
		dirpath =_T("..");
	else {
		wxFileName fn(dirpath, wxEmptyString);
		fn.RemoveLastDir();
		dirpath = fn.GetFullPath();
	}

	if (!dir.Open(dirpath)) {
		wxLogError(_("Cannot access Szarp directory, cannot procees!"));
		return wxEmptyString;
	}

	wxLogInfo(_("SzarpDir: ") + dirpath);

	return dirpath + wxFileName::GetPathSeparator();
#else
	wxString ret = SC::A2S(PREFIX).c_str();
	if (ret[ret.Length() - 1] != '/') {
		ret += wxString(_T("/"));
	}

	wxLogInfo(_("SzarpDir: ") + ret);

	return ret;
#endif
}

void szAppImpl::InitSzarpDataDir()
{
	wxConfig* cfg = new wxConfig(_T("SZARPDATADIR"));
	if (cfg->Exists(_T("SZARPDATADIR"))) {
		m_szarp_data_dir = cfg->Read(_T("SZARPDATADIR"), _T(""));
		const wxChar *slash =
#ifndef __WXMSW__
						_T("/");
#else
						_T("\\");
#endif

		if (!m_szarp_data_dir.EndsWith(slash))
			m_szarp_data_dir = m_szarp_data_dir + slash;

		delete cfg;
		return;
	}
	delete cfg;

#ifndef __WXMSW__
	m_szarp_data_dir = GetSzarpDir();
#else
	OSVERSIONINFOEXW vi;	
	ZeroMemory((void*)&vi, sizeof(vi));
	vi.dwOSVersionInfoSize = sizeof(vi);

	if (GetVersionEx((OSVERSIONINFOW*) &vi)) {
		if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT
				&& vi.dwMajorVersion == 6)
			//VISTA
			m_szarp_data_dir = wxGetHomeDir() + _T("\\.szarp\\");
		else
			m_szarp_data_dir = GetSzarpDir();
	} else {
		wxLogError(_T("Failed to get windows version info"));
		m_szarp_data_dir = GetSzarpDir();
	}
#endif
}


wxString szApp::GetSzarpDir() {
	return szAppImpl::GetSzarpDir();
}

void szAppImpl::SetSzarpDataDir(wxString dir) {
	wxConfig* cfg = new wxConfig(_T("SZARPDATADIR"));
	cfg->Write(_T("SZARPDATADIR"), dir);
	delete cfg;
}

wxString szAppImpl::GetSzarpDataDir() {
	if (has_data_dir == false) {
		InitSzarpDataDir();
		has_data_dir = true;
	}

	return m_szarp_data_dir;
}

void szAppImpl::InitializeLocale(wxArrayString &catalogs, wxLocale &locale) {

		wxString lang = wxConfig::Get()->Read(_T("LANGUAGE"), _T("pl"));

	int l;
	if (lang == _T("pl"))
		l = wxLANGUAGE_POLISH;
	else if (lang == _T("fr"))
		l = wxLANGUAGE_FRENCH;
	else if (lang == _T("de"))
		l = wxLANGUAGE_GERMAN;
	else if (lang == _T("it"))
		l = wxLANGUAGE_ITALIAN;
	else if (lang == _T("sr"))
		l = wxLANGUAGE_SERBIAN;
	else if (lang == _T("hu"))
		l = wxLANGUAGE_HUNGARIAN;
	else if (lang == _T("sr"))
		l = wxLANGUAGE_SERBIAN;
	else
		l = wxLANGUAGE_ENGLISH;

	locale.Init(l);

	if (!locale.IsOk()) {
		if(locale.IsAvailable(locale.GetSystemLanguage())) {
			l = locale.GetSystemLanguage();
			locale.Init(l);
			if(!locale.IsOk()) {
				wxMessageBox(_("Default locale cannot be loaded. Exiting"), _("Information."), wxICON_ERROR);
				exit(1);
			}
			wxString Ls = L"Locale " + lang + L" not available.\nDefault system locale loaded.";
			wxMessageBox(_(s), _("Information."), wxICON_INFORMATION);
		}
		else {
			wxLogWarning(_("No locale for this system`s language."));
		}
	}

	locale.AddCatalogLookupPathPrefix(GetSzarpDir() + _T("resources/locales"));

	for(size_t i = 0; i < catalogs.Count(); i++)
		locale.AddCatalog(catalogs[i]);


#ifdef __WXGTK__
	if (l == wxLANGUAGE_POLISH)
		setenv("LC_ALL", "pl_PL", 1);
	else if (l == wxLANGUAGE_FRENCH)
		setenv("LC_ALL", "fr_FR", 1);
	else if (l == wxLANGUAGE_GERMAN)
		setenv("LC_ALL", "de_DE", 1);
	else if (l == wxLANGUAGE_ITALIAN)
		setenv("LC_ALL", "it_IT", 1);
	else if (l == wxLANGUAGE_SERBIAN)
		setenv("LC_ALL", "sr_RS", 1);
	else if (l == wxLANGUAGE_HUNGARIAN)
		setenv("LC_ALL", "hu_HU", 1);
	else
		setenv("LC_ALL", "C", 1);
#endif

}

void szAppImpl::InitializeLocale(wxString catalog, wxLocale &locale)
{
	wxArrayString catalogs;
	catalogs.Add(catalog);
	catalogs.Add(_T("common"));
	catalogs.Add(_T("wx"));
	InitializeLocale(catalogs, locale);
}

void szAppImpl::ShowAbout() {
	wxBitmap bitmap;
	wxString logoimage = GetSzarpDir();

#ifndef MINGW32
	logoimage += _T("resources/wx/images/szarp-logo.png");
#else
	logoimage += _T("resources\\wx\\images\\szarp-logo.png");
#endif

#if wxUSE_LIBPNG
	/* Activate PNG image handler. */
	wxImage::AddHandler( new wxPNGHandler );
#endif

	wxBitmap* bitmapptr = NULL;
	if(bitmap.LoadFile(logoimage, wxBITMAP_TYPE_PNG)) {
		bitmapptr = &bitmap;
	}

	wxArrayString authors;
	/* About dialog data */
	authors.Add(L"Marcin Anderson");
	authors.Add(L"Micha\u0142 Blajerski");
	authors.Add(L"Piotr Branecki");
	authors.Add(L"S\u0142awomir Chy\u0142ek");
	authors.Add(L"Krzysztof Ga\u0142\u0105zka");
	authors.Add(L"Marcin Goliszewski");
	authors.Add(L"Jaros\u0142aw Janik");
	authors.Add(L"Stanis\u0142aw K\u0142osi\u0144ski");
	authors.Add(L"Pawe\u0142 Kolega");
	authors.Add(L"Dariusz Marcinkiewicz");
	authors.Add(L"Maciej Mochol");
	authors.Add(L"Jaros\u0142aw Nowisz");
	authors.Add(L"Krzysztof O\u0142owski");
	authors.Add(L"Pawe\u0142 Pa\u0142ucha");
	authors.Add(L"Lucjan Przykorski");
	authors.Add(L"Micha\u0142 R\u00F3j");
	authors.Add(L"Stanis\u0142aw Sawa");
	authors.Add(L"Adam Smyk");
	authors.Add(L"Tomasz \u015Awiatowiec");
	authors.Add(L"Maciej Zbi\u0144kowski");
	authors.Add(wxString(L"Witold Kowalewski") + _(" (Ideas)"));
	authors.Add(wxString(L"Jadwiga Ma\u0107kow") + _(" (French translations)"));
	authors.Add(wxString(L"Violetta Rowicka") + _(" (French translations)"));
	authors.Add(wxString(L"Marcin Zalewski") + _(" (logo)"));


	wxDialog* dlg = new szAboutDlg(bitmapptr, m_progname, m_version, m_releasedate, authors);
	if (szFrame::default_icon.IsOk()) {
		dlg->SetIcon(szFrame::default_icon);
	}

	dlg->Centre();
	dlg->ShowModal();

	delete dlg;

	return;
}

szApp::szApp() : wxApp(), szAppImpl() {
}

void szApp::InitSzarpDataDir() {
	return szAppImpl::InitSzarpDataDir();
}

void szApp::SetSzarpDataDir(wxString dir) {
	szAppImpl::SetSzarpDataDir(dir);
}

bool szApp::OnInit() {
	if (!wxApp::OnInit())
		return false;
	szAppImpl::SetProgPath(argv[0]);
	szAppImpl::OnInit();
	return true;
}

wxString szApp::GetSzarpDataDir() {
	return szAppImpl::GetSzarpDataDir();
}

void szApp::InitializeLocale(wxArrayString &catalogs, wxLocale &locale)
{
	szAppImpl::InitializeLocale(catalogs, locale);
}

void szApp::InitializeLocale(wxString catalog, wxLocale &locale)
{
	szAppImpl::InitializeLocale(catalog, locale);
}

void szApp::ShowAbout()
{
	szAppImpl::ShowAbout();
}

void szApp::SetProgName(wxString str) {
	szAppImpl::SetProgName(str);
}

bool szApp::OnCmdLineError(wxCmdLineParser &parser) {
	return wxApp::OnCmdLineError(parser);
}

bool szApp::OnCmdLineHelp(wxCmdLineParser &parser) {
	return wxApp::OnCmdLineHelp(parser);
}

bool szApp::OnCmdLineParsed(wxCmdLineParser &parser){
	return wxApp::OnCmdLineParsed(parser);
}

void szApp::OnInitCmdLine(wxCmdLineParser &parser) {
	wxApp::OnInitCmdLine(parser);
	parser.AddSwitch(_("Dprefix"), wxEmptyString, _T("database prefix"));
}

