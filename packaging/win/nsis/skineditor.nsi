;-----------------------------------------------------------------------
; Skin Editor Installer Script
;-----------------------------------------------------------------------

; Properly display all languages
Unicode true

; DPI-awareness option requires NSIS 3.x!
ManifestDPIAware true

;-----------------------------------------------------------------------
; Build Locations
;-----------------------------------------------------------------------

!ifndef BASEDIR
!define BASEDIR "..\..\..\..\..\.."
!endif

!ifndef CCL_FRAMEWORK_DIR
!define CCL_FRAMEWORK_DIR "${BASEDIR}\framework"
!endif

!define EULADIR "${CCL_IDENTITIES_DIR}\ccl\eula"
!define LEGALDIR "${CCL_IDENTITIES_DIR}\ccl\legal"

;-----------------------------------------------------------------------
; Includes
;-----------------------------------------------------------------------

!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"

!include "shared.nsh"

!addplugindir "${CCL_BASEDIR}\submodules\nsis-plugins\ccltools\bin\"

;-----------------------------------------------------------------------
; Settings
;-----------------------------------------------------------------------

!define PRODUCT  		"CCL Skin Editor"
!define EXENAME			"${PRODUCT}.exe"

!define VER_MAJOR		"1"
!define VER_MINOR		"0"
!define VER_REVISION	"0"
!define PRODUCT_VERSION	"${VER_MAJOR}.${VER_MINOR}.${VER_REVISION}"

Name 			"${PRODUCT}"
InstallDir		"$PROGRAMFILES64\${PRODUCT}"
BrandingText	" "

!ifdef X64
  OutFile		"${PRODUCT} Installer.exe"
!else
!ifdef ARM64
  OutFile		"${PRODUCT} Installer Arm64.exe"
!endif
!endif

;-----------------------------------------------------------------------
; Version Information
;-----------------------------------------------------------------------

# search buildnumber.h for repository revision
!searchparse /file "${BASEDIR}\buildnumber.h" '#define BUILD_REVISION_STRING		"' BUILD_REVISION '"'

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey  "CompanyName" "${COMPANY}"
VIAddVersionKey  "FileDescription" "${PRODUCT} Installer"
VIAddVersionKey  "FileVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"
VIAddVersionKey  "InternalName" "${VENDOR_PACKAGE_DOMAIN}.skineditor"
VIAddVersionKey  "LegalCopyright" "${COPYRIGHT}"
VIAddVersionKey  "ProductName" "${PRODUCT}"
VIAddVersionKey  "ProductVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"

;-----------------------------------------------------------------------
; Definitions
;-----------------------------------------------------------------------

!define MUI_ICON   "..\resource\skineditor.ico"
!define MUI_UNICON "..\resource\skineditor.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP "resource\wizard@2x.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "resource\wizard@2x.bmp"
;!define MUI_ABORTWARNING

;!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_BITMAP "resource\header@2x.bmp"
;!define MUI_HEADERIMAGE_UNBITMAP "resource\header@2x.bmp"

;!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
;!define MUI_LANGDLL_REGISTRY_KEY "Software\${PRODUCT}" 
;!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp

;-----------------------------------------------------------------------
; Pages
;-----------------------------------------------------------------------

!insertmacro MUI_PAGE_WELCOME_CUSTOMIZED
!insertmacro MUI_PAGE_LICENSE_CUSTOMIZED $(licenseFileName)

!insertmacro MUI_PAGE_DIRECTORY_CUSTOMIZED
!insertmacro MUI_PAGE_INSTFILES_CUSTOMIZED
!insertmacro MUI_PAGE_FINISH_CUSTOMIZED

; Uninstaller
!insertmacro MUI_UNPAGE_CONFIRM_CUSTOMIZED
!insertmacro MUI_UNPAGE_INSTFILES_CUSTOMIZED

;-----------------------------------------------------------------------
; Languages
;-----------------------------------------------------------------------

!insertmacro MUI_LANGUAGE "English"
;!insertmacro MUI_LANGUAGE "German"
;!insertmacro MUI_LANGUAGE "Japanese"
;!insertmacro MUI_LANGUAGE "SimpChinese"
;!insertmacro MUI_LANGUAGE "French"
;!insertmacro MUI_LANGUAGE "Spanish"

LicenseLangString licenseFileName ${LANG_ENGLISH} "${EULADIR}\EULA.txt"
;LicenseLangString licenseFileName ${LANG_GERMAN}  "${EULADIR}\EULA-de.txt"
;LicenseLangString licenseFileName ${LANG_SIMPCHINESE}   "${EULADIR}\EULA-zh.txt"
;LicenseLangString licenseFileName ${LANG_JAPANESE} "${EULADIR}\EULA-ja.txt"
;LicenseLangString licenseFileName ${LANG_FRENCH} "${EULADIR}\EULA-fr.txt"
;LicenseLangString licenseFileName ${LANG_SPANISH} "${EULADIR}\EULA-es.txt"

; Shared Strings and font definitions
!include "${NSIS_INCLUDES_DIR}\localize.nsh"

;-----------------------------------------------------------------------
; Variables
;-----------------------------------------------------------------------

Var RunAppOnExit

;-----------------------------------------------------------------------
; Functions
;-----------------------------------------------------------------------

Function .onInit

	StrCpy $RunAppOnExit 0

	; Check for supported Windows version
	Call CheckWindowsVersion

	; Check for 64 bit version and switch registry view
	Call Check64BitVersion

	; Language selection dialog
	;!insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

Function LaunchApp
	StrCpy $RunAppOnExit 1
FunctionEnd

Function .onGUIEnd
	${if} $RunAppOnExit != 0
		ccltools::runFromExplorer "$INSTDIR\${EXENAME}"
		pop $0
	${EndIf}
FunctionEnd

;-----------------------------------------------------------------------
; Installer Sections
;-----------------------------------------------------------------------

Section "-All"

  SetOverwrite ifnewer
  
  ;-----------------------------------------------------------------------
  ; Application Files
  ;-----------------------------------------------------------------------

  SetOutPath "$INSTDIR"
  Delete "$INSTDIR\*.dll" ; remove old EXE and DLL files
  Delete "$INSTDIR\*.exe" 

  File "${BUILDDIR}\${EXENAME}"

  RMDir /r "$INSTDIR\license" ; cleanup old license files

  SetOutPath "$INSTDIR\license"
  File "${LEGALDIR}\*.txt"
  File "${EULADIR}\*.txt"

  ;-----------------------------------------------------------------------
  ; Help files
  ;-----------------------------------------------------------------------
  
  ;RMDir /r "$INSTDIR\help" ; cleanup old help files

  ;SetOutPath "$INSTDIR\help"
  ;File /r "..\..\..\assets\help\*.*"
  
  ;-----------------------------------------------------------------------
  ; Framework Files
  ;-----------------------------------------------------------------------

  SetOutPath "$INSTDIR"

  File "${BUILDDIR}\ccltext.dll"
  File "${BUILDDIR}\cclsystem.dll"
  File "${BUILDDIR}\cclgui.dll"
 
  ;-----------------------------------------------------------------------
  ; CRT
  ;-----------------------------------------------------------------------
  
  !include /NONFATAL "${CCL_BASEDIR}\submodules\vcredist\crt.nsh"

  ;-----------------------------------------------------------------------
  ; Language Packs
  ;-----------------------------------------------------------------------

  ;RMDir /r "$INSTDIR\languages" ; cleanup old language packs

  ;SetOutPath "$INSTDIR\languages"
  ;File /r "${BASEDIR}\translations\projects\xxx\languages\*.langpack"
  
  ;-----------------------------------------------------------------------
  ; PlugIns
  ;-----------------------------------------------------------------------

  Delete "$INSTDIR\Plugins\*.dll" ; cleanup plug-in folder

  SetOutPath "$INSTDIR\Plugins"

  File "${BUILDDIR}\plugins\jsengine.dll"
      
  ;-----------------------------------------------------------------------
  ; Scripts
  ;-----------------------------------------------------------------------

  RMDir /r "$INSTDIR\Scripts" ; cleanup old scripts

  SetOutPath "$INSTDIR\Scripts"
  File "..\..\..\assets\scripts\*.package"

  ;-----------------------------------------------------------------------
  ; Uninstaller
  ;-----------------------------------------------------------------------
 
  SetOutPath "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;-----------------------------------------------------------------------
  ; Shortcuts
  ;-----------------------------------------------------------------------

  CreateShortCut "$SMPROGRAMS\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}"
  CreateShortCut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}"

  ;-----------------------------------------------------------------------
  ; Register Uninstaller
  ;-----------------------------------------------------------------------

  WriteRegStr HKLM "Software\${PRODUCT}" "" $INSTDIR
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayName" "${PRODUCT}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayIcon" "$INSTDIR\${EXENAME},-1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoRepair" "1"
  
SectionEnd

;-----------------------------------------------------------------------
; Uninstaller
;-----------------------------------------------------------------------

Section "Uninstall"

  ; User Settings
  RMDir /r "$APPDATA\${PRODUCT}"

  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.dll"

  RMDir /r "$INSTDIR\license"
  ;RMDir /r "$INSTDIR\help"
  RMDir /r "$INSTDIR\Plugins"
  RMDir /r "$INSTDIR\Scripts"
  ;RMDir /r "$INSTDIR\languages"

  RMDir $INSTDIR ; should be empty now
    
  DeleteRegKey HKCU "Software\${PRODUCT}"
  DeleteRegKey HKLM "Software\${PRODUCT}"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"
  
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run\" "${PRODUCT}"

  SetShellVarContext current

  Delete "$DESKTOP\${PRODUCT}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT}.lnk"

  SetShellVarContext all

  Delete "$DESKTOP\${PRODUCT}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT}.lnk"

  SetShellVarContext current

SectionEnd

;-----------------------------------------------------------------------
; Uninstaller Functions
;-----------------------------------------------------------------------

Function un.onInit

  ; switch to x64 Registry
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}

  ; Language selection dialog
  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd