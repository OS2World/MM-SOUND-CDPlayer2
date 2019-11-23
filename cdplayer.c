/*************************************************************************
 * Dateiname    :  cdplayer.c    
 *
 * Beschreibung :  Beispielprogramm eines CD-Players unter Verwendung des
 *                 Multimedia Toolkits und der MCI String Command 
 *                 Schnittstelle.
 *
 * Inhalt       :  Dieses Beispielprogramm zeigt auf, wie einfach eine 
 *                 Multimediaanwendung unter Verwendung der Textschnitt-
 *                 stelle entwickelt werden kann.
 *
 *                 Eine Audio-CD wird gelesen und kann abgespielt werden.
 *                 ZusÑtzlich kînnen die Titel der CD in eine Datenbank
 *                 eingetragen werden. Die Datenbank ist als INI-Datei
 *                 organisiert (der Einfachheit wegen).
 *
 * MMPM/2 API's :  Folgende MMPM/2 API's werden verwendet
 *
 *
 * Alle Dateien :  cdplayer.c     Source Code.
 *                 cdplayer.h     Include Datei.
 *                 cdplayer.rc    Resource Datei.
 *                 cdplayer.dlg   Dialog Definition.
 *                 cdplayer.mak   Datei fÅr NMAKE.
 *                 cdplayer.def   Datei fÅr LINK386.
 *                 cdplayer.ico   CD-Player Icon.
 *
 * Copyright (C) 1993 Axel Salomon, fÅr Inside OS/2 Ausgabe November '93 
 *************************************************************************/

#define  INCL_WIN                   /* Win  APIs einbinden.              */
#define  INCL_PM                    /* PM   APIs einbinden.              */
#define  INCL_OS2MM                 /* Multimedia APIs einbinden         */
#define  INCL_SW                    /* fÅr 'secondary windows'           */

#include <os2.h>
#include <os2me.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sw.h>

#include "cdplayer.h"

#define MCI_STRING_LENGTH           256
#define MCI_ERROR_STRING_LENGTH     128
#define MCI_RETURN_STRING_LENGTH    128

/*
 * Prototyping.
 */

MRESULT EXPENTRY CdplayerDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY MehrDlgProc    ( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

INT     main( INT argc, CHAR *argv[] );
VOID    Beenden( VOID );
VOID    Initialisierung( VOID );
ULONG   SendStringToMCI( PSZ pszCmd, HWND hwndNotify, USHORT usUserParam );
VOID    CdpInitContainer( VOID );
VOID    CdpSetupAudioCDInfo( VOID );
VOID    CdpClearAudioCDInfo( VOID );
VOID    CdpUpdateTrack( LONG lTrack, LONG lFlag );
VOID    CdpUpdateAktuell( VOID );
VOID    CdpEnableObjects( BOOL bFlag );

/*
 * Globale Variablen.
 */

/* Handle der verschiedenen Objekte */
HAB    hab;                            /* Handle Anchor Block       */
HMQ    hmq;                            /* Handle Message Queue      */
HWND   hwndFrame,hwndFrame2;           /* Handle des framewindows   */
HWND   hwndCdplayer;                   /* Handle des Dialoges       */
HWND   hwndCdmehr;                     /* Handle des MehrDialoges   */
HWND   hwndContainer;                  /* Containerhandle           */
HWND   hwndSpinVolume;                 /* Handle fÅr die LautstÑrke */
HWND   hwndSpinbutton;                 /* Handle fÅr die Titelwahl  */
HWND   hwndScrollbar;                  /* Handle fÅr die Zeitansteuerung */
HWND   hwndCheckbox;                   /* Handle fÅr Musikkopplung  */
HWND   hwndEFCDTitel;                  /* Eingabefeldhandle ...     */
HWND   hwndEFCDID;
HWND   hwndEFAutor;
HWND   hwndEFTitel;
HWND   hwndEFLaenge;
HWND   hwndEFAktuell;
HWND   hwndEFMusik;                    /* Handles fÅr den Mehr ... Dialog */
HWND   hwndEFText;
HWND   hwndEFRichtung;
HWND   hwndEFTakte;
HWND   hwndEFAusgabe;
HWND   hwndMLEBemerk;  
HINI   hiniCdplayer;                   /* Handle der CD-Player INI-Datei */
HPOINTER  hptrWait, hptrArrow;         /* Handle fÅr die Zeiger Uhr und Pfeil */

/* Container Variable */
CNRINFO cnri;                          /* Container Info Struktur */
ULONG   cnriFlags;

FIELDINFOINSERT fii;                   /* Struktur fÅr das AnfÅgen von Spalten */

#define CNR_COLUMN_NUM 10              /* Anzahl der Spalten */

UCHAR achColumnTitle[CNR_COLUMN_NUM][20] = { "Nr.", "Titel", "Autor", "LÑnge", "Musik von", "Text von", "Musikrichtung", "Erscheinungsdatum", "Bemerkungen", "Millisekunden" };
ULONG aulColumnData [CNR_COLUMN_NUM]     = {CFA_ULONG, CFA_STRING, CFA_STRING, CFA_STRING, CFA_STRING, CFA_STRING, CFA_STRING, CFA_STRING, CFA_STRING, CFA_ULONG };

/* Textspeicher */ 
UCHAR achMciSnd[MCI_STRING_LENGTH];         /* Kommando      fÅr mciSendString() */
UCHAR achReturn[MCI_RETURN_STRING_LENGTH];  /* RÅckgabe */
UCHAR achError [MCI_ERROR_STRING_LENGTH];   /* Fehler   */

CHAR acBuffer[128];                         /* temporÑrer Textspeicher */

/* CD Daten */
UCHAR acHexID[19];                     /* CD-ID in HEX Format             */
UCHAR acHex[17];

UCHAR acCdDevice[20];                  /* Parameter 1 des CDPLAYER's      */
  
UCHAR acCDTitel[128];                  
LONG  lNumOfTracks;                    /* Anzahl der Titel                */
LONG  lCurrTrack, lUnlinkedTrack;      /* Steuerung des Titelwahl         */
LONG  lVolume = 75;                    /* LautstÑrke: 75%                 */
BOOL  bIsPlaying;                      /* Schalter: Musik lÑuft           */
BOOL  bIsMediaPres;                    /*           Audio CD eingelegt    */
BOOL  bIsLinked;                       /*           Musik/Titel verbunden */
BOOL  bIsMehr = FALSE;                 /*           "Mehr" Dialog aktiv   */

/* Titeldaten */
typedef struct _CDTITELINFO { /* FÅr Eingabefelder */
        UCHAR  acAutor[128];
        UCHAR  acTitel[128];
        UCHAR  acLaenge[16];
        UCHAR  acMusik[128];
        UCHAR  acText[128];
        UCHAR  acRichtung[128];
        UCHAR  acTakte[128];
        UCHAR  acAusgabe[30];
        UCHAR  acBemerkung[4096];
        LONG   lMillisec;
        PRECORDCORE prcCnrDaten;
      } CDTITELINFO;

typedef union _DATEN { /* FÅr Containerrecords */
        LONG     lWert;
        PSZ      pszString;
        PVOID    pOther;
        CDATE    date;
        CTIME    time;
        HBITMAP  bitmap;
        HPOINTER icon;
      } DATEN;

#define MAX_CD_TITEL 99 

CDTITELINFO aCDTitel[MAX_CD_TITEL];

/* Definitionen fÅr CD-Datenaktualisierung */
#define CDP_KEEPTIME       0x0001   /* Musikposition beibehalten     */
#define CDP_NOCNRUPDATE    0x0002   /* Container nicht aktualisieren */
#define CDP_UPDATETEXTONLY 0x0004   /* Musik nicht aktualisieren     */

/*
 * Main.
 */

INT main( INT argc, CHAR *argv[] )
{
   QMSG    qmsg;

/* Der 1. Parameter von CDPLAYER kann ein cdaudio-GerÑt in der Form 
   cdaudio01, cdaudio02, cdaudio03, etc. sein */

   if( argc >= 2 )
       strcpy( acCdDevice, argv[1] );
   else
      strcpy( acCdDevice, "cdaudio" );

   Initialisierung();

   while ( WinGetMsg( hab, (PQMSG) &qmsg, (HWND) NULL, 0, 0) )
      WinDispatchMsg( hab, (PQMSG) &qmsg );

   Beenden();

   return( TRUE);
} 


/*
 * Initialisierung.
 */

VOID Initialisierung( VOID)
{
   hab       = WinInitialize( 0);
   hmq       = WinCreateMsgQueue( hab, 0);

   /* Profile îffnen */
   hiniCdplayer = PrfOpenProfile( hab, "CDPLAYER.INI" );

   /* ID-konvertierung vorbereiten */
   strcpy( acHex, "0123456789ABCDEF" );
   strcpy( acHexID, "0x00000000" );

   /* Handle der Zeiger erfragen und auf Warten setzen */
   hptrArrow = WinQuerySysPointer ( HWND_DESKTOP, SPTR_ARROW, FALSE );
   hptrWait  = WinQuerySysPointer ( HWND_DESKTOP, SPTR_WAIT,  FALSE );

   WinSetPointer ( HWND_DESKTOP, hptrWait );

   /* Dialog laden */
   hwndFrame =                    /* Handle des Framewindows               */
       WinLoadSecondaryWindow(
          HWND_DESKTOP,           /* Elternteil des Dialoges               */
          HWND_DESKTOP,           /* EigentÅmer des Dialoges               */
          (PFNWP) CdplayerDlgProc,/* Prozedur des Dailoges                 */
          (HMODULE) NULL,         /* Dialog befindet sich in der EXE-Datei */
          ID_DLG_CDPLAYER,        /* Dialog ID                             */
          (PVOID) NULL);          /* Parameter fÅr den Dialog              */

   /* "Standardgrî·e" zum SystemmenÅ hinzufÅgen */
   WinInsertDefaultSize(hwndFrame, "~Standardgrî·e");

   /* Handle des Dialogs erfragen */
   hwndCdplayer = WinQuerySecondaryHWND(hwndFrame, QS_DIALOG);

   /* Dialog anzeigen */
   WinShowWindow( hwndFrame, TRUE );        /* Dialog anzeigen             */

   /* Zeiger wieder zurÅcksetzen */
   WinSetPointer ( HWND_DESKTOP, hptrArrow );

}


/*
 * Beenden.
 */

VOID Beenden( VOID )
{
   /* Ini-Datei schlie·en */
   PrfCloseProfile( hiniCdplayer );

   WinDestroySecondaryWindow( hwndFrame ); 
   WinDestroyMsgQueue( hmq );
   WinTerminate( hab );
}  


/*
 * CdplayerDlgProc.
 */

MRESULT EXPENTRY CdplayerDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  NOTIFYRECORDEMPHASIS *nre;      /* Struktur fÅr die Aktivierung von ContainersÑtzen */
  HPOINTER hpPrgIcon;             /* Handle fÅr das Programmicon */
  HACCEL   hAcc;                  /* Kurztasten Handle */
  PSZ      pszReturnString;       /* Zeiger auf ein RÅckgabetext */
  USHORT   id;                    /* id des Objektes, da· ein Mitteilung (WM_CONTROL) bekommen hat */
  LONG     lMinuten, lSekunden, lTrack, lMaxMin, lMaxSec; /* Zeitwerte */
  ULONG    ni, ulRet;             /* ni = notify information; RÅckgabewert von Funktionen */
  SHORT    nc, slpos, cmd, sSec;  /* nc = notify command; slpos Musikposition / Sliderposition in msec. */
  PVOID    pUser;                 /* Zeiger auf private Daten eines Containerdatensatzes */
  DATEN   *pDaten;                /* Zeiger auf ein Array von Daten eines Containerdatensatzes */
  static BOOL bIsInit = TRUE;     /* Initialisierung aktiv ? */

   switch( msg ) {
     case WM_INITDLG :
          /* ICON fÅr den Dialog laden und anzeigen */
          hpPrgIcon = WinLoadPointer( HWND_DESKTOP, (HMODULE) NULL, ID_IC_CDPLAYER );
          WinDefSecondaryWindowProc( hwndFrame, WM_SETICON, (MPARAM) hpPrgIcon, (MPARAM) 0 );
          hAcc = WinLoadAccelTable( hab, (HMODULE) NULL, ID_AC_CDPLAYER );
          WinSetAccelTable( hab, hAcc, hwndFrame );

          /* Handle der Objekte sichern */
          hwndCdplayer   = hwnd;
          hwndContainer  = WinWindowFromID( hwnd, ID_CNR_TITELANZEIGE  );
          hwndSpinbutton = WinWindowFromID( hwnd, ID_SB_LIEDNR         );
          hwndSpinVolume = WinWindowFromID( hwnd, ID_SB_VOLUME         );
          hwndScrollbar  = WinWindowFromID( hwnd, ID_SCR_ANZEIGE       );
          hwndCheckbox   = WinWindowFromID( hwnd, ID_CKB_TRENNEN       );
          hwndEFCDTitel  = WinWindowFromID( hwnd, ID_EF_CDTITEL        );
          hwndEFCDID     = WinWindowFromID( hwnd, ID_EF_CDID           );
          hwndEFAutor    = WinWindowFromID( hwnd, ID_EF_AUTOR          );
          hwndEFTitel    = WinWindowFromID( hwnd, ID_EF_TITEL          );
          hwndEFLaenge   = WinWindowFromID( hwnd, ID_EF_LAENGE         );
          hwndEFAktuell  = WinWindowFromID( hwnd, ID_EF_ZEITAKTUELL    );

          /* EingabefeldlÑngen sichern */
          WinSendMsg( hwndEFCDTitel, EM_SETTEXTLIMIT, (MPARAM) 128, (MPARAM) NULL );
          WinSendMsg( hwndEFAutor  , EM_SETTEXTLIMIT, (MPARAM) 128, (MPARAM) NULL );
          WinSendMsg( hwndEFTitel  , EM_SETTEXTLIMIT, (MPARAM) 128, (MPARAM) NULL );

          /* Container initialisieren, Titel, Spalten, etc. */
          CdpInitContainer();

          /* Audio CD GerÑt îffnen und CD prÅfen */
          sprintf( achMciSnd, "open %s alias cd wait", acCdDevice );
          SendStringToMCI( achMciSnd, hwnd, 0 );
          SendStringToMCI( "status cd media present wait", hwnd, 0 );

          /* Ist eine Audio CD eingelegt, Daten auswerten, sonst Daten zurÅcksetzen */
          if( strcmp( achReturn, "TRUE" ) == 0 )
          {
              CdpSetupAudioCDInfo();
          }
          else
          {
              CdpClearAudioCDInfo();
          }

          /* Schalter setzen */
          bIsPlaying = FALSE;
          bIsInit = FALSE;

          /* Timer starten, um Anzeige zu aktualisieren und Medium zu prÅfen */
          WinStartTimer( hab, hwnd, 1, 1000 );

          return( (MRESULT) 0);

     case WM_TIMER: /* Eine Sekunde ist vorbei ... */
          if( mp1 != (MPARAM) 1)
              return (MRESULT) 0;

          /* Timer anhalten, falls eine CD ausgelesen worden ist, wÅrde dies zu Lange dauern
             und es wÅrden weitere WM_TIMER - Msg. auflaufen */
          WinStopTimer( hab, hwnd, 1 );

          /* Medium prÅfen */
          SendStringToMCI( "status cd media present wait", hwnd, 0 );

          if( strcmp( achReturn, "FALSE" ) == 0 && bIsMediaPres )       /* Die CD wurde herausgenommen */
          {
              WinSetPointer ( HWND_DESKTOP, hptrWait );
              CdpClearAudioCDInfo();
              WinSetPointer ( HWND_DESKTOP, hptrArrow );
          }
          else if( strcmp( achReturn, "TRUE" ) == 0 && ! bIsMediaPres ) /* Eine CD wurde eingelegt */
          {
              WinSetPointer ( HWND_DESKTOP, hptrWait );
              CdpSetupAudioCDInfo();
              WinSetPointer ( HWND_DESKTOP, hptrArrow );
          }
          else /* Anzeige aktuallisieren */
          {
             if( bIsMediaPres && bIsPlaying)
                 CdpUpdateAktuell();
          }

          /* Timer wieder starten */
          WinStartTimer( hab, hwnd, 1, 1000 );
          return (MRESULT) 0;

     case WM_CLOSE : /* Der Dialog wird geschlossen */
          /* Timer stoppen */
          WinStopTimer( hab, hwnd, 1 );

          /* Audio CD GerÑt schlie·en */
          SendStringToMCI( "close cd wait", hwnd, 0 );

          /* "Mehr" Dialog schlie·en, falls noch aktiv ... */
          if( bIsMehr )
              WinSendMsg( hwndCdmehr, WM_CLOSE, (MPARAM) NULL, (MPARAM) NULL );

          return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2 ) );

     case WM_CONTROL: /* Ein Objekt hat ein Mitteilung verschickt */
          /* WÑhrend der Initialisierung oder wenn keine CD eingelegt wurde, haben wir nichts zu tun */
          if( bIsInit || ! bIsMediaPres )
              return (MRESULT) 0;

          id = (USHORT) SHORT1FROMMP( mp1 ); /* Objekt-Id speichern  */
          nc = (USHORT) SHORT2FROMMP( mp1 ); /* Notify Cmd speichern */
          ni = (ULONG) mp2;                  /* Weitere Informationen zu dieser Mitteilung */

          switch( id ){
            case ID_SB_LIEDNR: /* Spinbutton Titelauswahl */
                 switch( nc ){
                   case SPBN_CHANGE:
                   case SPBN_ENDSPIN:
                        WinSendMsg( hwndSpinbutton, SPBM_QUERYVALUE, &lTrack, MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ) );
                        break;
                   default:
                        return (MRESULT) 0;
                 }
                 /* Aktuallisiere Eingabefelder, Scrollbar und Audio-CD */
                 if( (bIsLinked && lTrack != lCurrTrack) || (!bIsLinked && lTrack != lUnlinkedTrack) )
                     CdpUpdateTrack( lTrack, (bIsLinked ? 0 : CDP_UPDATETEXTONLY) );
                 return (MRESULT) 0;

            case ID_SB_VOLUME: /* Spinbutton LautstÑrke */
                 switch( nc ){
                   case SPBN_CHANGE:
                   case SPBN_ENDSPIN:
                        WinSendMsg( hwndSpinVolume, SPBM_QUERYVALUE, &lVolume, MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ) );
                        break;
                   default:
                        return (MRESULT) 0;
                 }
                 /* Aktuallisiere LautstÑrke */
                 sprintf( acBuffer, "set cd audio volume %i wait", lVolume );
                 SendStringToMci( acBuffer, hwndCdplayer, 0 );
                 return (MRESULT) 0;

            case ID_EF_CDTITEL: /* Eingabefeld CD-Titel */
                 switch( nc ){
                   case EN_CHANGE:
                        WinQueryWindowText( hwndEFCDTitel, 128, acBuffer );
                        if( strcmp( acBuffer, acCDTitel ) )  /* Titel bei énderung aktualisieren */
                        {
                            strcpy( acCDTitel, acBuffer );   /* sichern */ /* In INI-Datei speichern */
                            PrfWriteProfileString( hiniCdplayer, acHexID, "CD-Titel", acCDTitel );
                            memset( &cnri, 0, sizeof(cnri)); /* Container aktualisieren */
                            cnri.pszCnrTitle = acCDTitel; 
                            cnriFlags        = CMA_CNRTITLE;
                            WinSendMsg( hwndContainer, CM_SETCNRINFO, (MPARAM) &cnri, (MPARAM) cnriFlags );
                        }
                        break;
                   default:
                        return (MRESULT) 0;
                 }
                 return (MRESULT) 0;

            case ID_EF_AUTOR: /* Eingabefeld Autor */
                 switch( nc ){
                   case EN_CHANGE:
                        /* aktuelle TitelNr abfragen, Autor in der INI-Datei speichern, Container aktualisieren */
                        WinSendMsg( hwndSpinbutton, SPBM_QUERYVALUE, &lTrack, MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ) );
                        WinQueryWindowText( hwndEFAutor, 128, acBuffer );
                        if( strcmp( aCDTitel[lTrack-1].acAutor, acBuffer ) )
                        {
                            strcpy( aCDTitel[lTrack-1].acAutor, acBuffer );
                            sprintf( acBuffer, "AUTOR%i", lTrack );
                            PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lTrack-1].acAutor );
                            WinSendMsg( hwndContainer, CM_INVALIDATERECORD, (MPARAM) &aCDTitel[lTrack-1].prcCnrDaten, MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );
                        }
                        break;
                   default:
                        return (MRESULT) 0;
                 }
                 return (MRESULT) 0;

            case ID_EF_TITEL: /* Eingabefeld Titel */
                 switch( nc ){
                   case EN_CHANGE:
                        /* aktuelle TitelNr abfragen, Titel in der INI-Datei speichern, Container aktualisieren */
                        WinSendMsg( hwndSpinbutton, SPBM_QUERYVALUE, &lTrack, MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ) );
                        WinQueryWindowText( hwndEFTitel, 128, acBuffer );
                        if( strcmp( aCDTitel[lTrack-1].acTitel, acBuffer ) )
                        {
                            strcpy( aCDTitel[lTrack-1].acTitel, acBuffer );
                            sprintf( acBuffer, "TITEL%i", lTrack );
                            PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lTrack-1].acTitel );
                            WinSendMsg( hwndContainer, CM_INVALIDATERECORD, (MPARAM) &aCDTitel[lTrack-1].prcCnrDaten, MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );
                        }
                        break;
                   default:
                        return (MRESULT) 0;
                 }
                 return (MRESULT) 0;

            case ID_CNR_TITELANZEIGE: /* Container */
                 switch(nc) {
                   case CN_ENTER: /* Auswahl ... ok */
                        break;

                   case CN_EMPHASIS: /* Hervorhebung : nur wenn ein Datensatz ausgewÑhlt worden ist */
                        nre = (NOTIFYRECORDEMPHASIS *)ni;
                        if( !( nre->fEmphasisMask & CRA_SELECTED ) || !( nre->pRecord->flRecordAttr & CRA_SELECTED ) )
                            return (MRESULT) 0;
                        break;
                   default:
                        return (MRESULT) 0;
                 }

                 /* Datensatz erfragen und Zeiger setzen */
                 pUser = (PVOID) WinSendMsg( hwndContainer, CM_QUERYRECORDEMPHASIS, (MPARAM) CMA_FIRST, (MPARAM) CRA_SELECTED );
                 if( pUser == (PVOID) NULL || (LONG) pUser < 0 )
                     return (MRESULT) NULL;
                 pDaten = (DATEN *)(((PBYTE) pUser)+sizeof(RECORDCORE));
                 pDaten--; /* Irgendetwas stimmt mit dem Beginn der User-Daten nicht, daher 4 Bytes zurÅck (warum ?) */

                 /* Titel erfragen und Dialog aktualisieren (etwas kompliziert, sorry, wegen der Entkopplung) */
                 lTrack = pDaten[0].lWert; 
                 if( lTrack > 0 && lTrack <= lNumOfTracks )
                     if( (bIsLinked && lTrack != lCurrTrack) || (!bIsLinked && lTrack != lUnlinkedTrack) )
                         CdpUpdateTrack( lTrack, CDP_NOCNRUPDATE | (bIsLinked ? 0 : CDP_UPDATETEXTONLY) );
                 return (MRESULT) 0;

            case ID_CKB_TRENNEN: /* Checkbox Titelwahl und Musik trennen */
                 switch( nc ){
                   case BN_CLICKED:
                   case BN_DBLCLICKED: /* Status abfragen */
                        bIsLinked = (BOOL)WinSendMsg( hwndCheckbox, BM_QUERYCHECK, (MPARAM) NULL, (MPARAM) NULL );
                        break;
                   default:
                        return (MRESULT) 0;
                 }
                 /* Aktuallisiere Eingabefelder, Scrollbar und Audio-CD */
                 WinSendMsg( hwndSpinbutton, SPBM_QUERYVALUE, &lTrack, MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ) );
                 if( lTrack != lCurrTrack )
                     CdpUpdateTrack( lTrack, (bIsLinked ? 0 : CDP_UPDATETEXTONLY) );
                 return (MRESULT) 0;

            default:
                 return (MRESULT) 0;
          }
          return( (MRESULT) 0);

     case WM_HSCROLL: /* Die Scrollbar wurde neu positioniert */
          if( bIsInit || ! bIsMediaPres )
              return (MRESULT) 0;

          id      = (USHORT) mp1;          /* Objekt Id */
          cmd     = SHORT2FROMMP( mp2 );   /* Scrollbar-Befehl */ /* Aktuelle Position */
          slpos   = SHORT1FROMMP( mp2 );
          if( slpos == 0 )
              slpos   = (SHORT)WinSendMsg( hwndScrollbar, SBM_QUERYPOS, (MPARAM) NULL, (MPARAM) NULL );

          /* Minuten und Sekunden berechnen */
          lMinuten  = (LONG)slpos / 60;
          lSekunden = (LONG)slpos % 60;

          switch( cmd ){
            case SB_LINELEFT: /* Eine Sekunde zurÅck */
                 slpos --;
                 if( slpos < 0 )
                     slpos = 0;
                 WinSendMsg( hwndScrollbar, SBM_SETPOS, MPFROMSHORT(slpos), (MPARAM) NULL );
                 break;
            case SB_LINERIGHT: /* Ein Sekunde vor */
                 slpos ++;
                 if( (LONG)slpos * 1000 > aCDTitel[lCurrTrack-1].lMillisec )
                     slpos = (SHORT) (aCDTitel[lCurrTrack-1].lMillisec / 1000);
                 WinSendMsg( hwndScrollbar, SBM_SETPOS, MPFROMSHORT(slpos), (MPARAM) NULL );
                 break;
            case SB_PAGELEFT: /* Eine Minute zurÅck */
                 slpos -= 60;
                 if( slpos < 0 )
                     slpos = 0;
                 WinSendMsg( hwndScrollbar, SBM_SETPOS, MPFROMSHORT(slpos), (MPARAM) NULL );
                 break;
            case SB_PAGERIGHT: /* Eine Minute vor */
                 slpos += 60;
                 if( (LONG)slpos * 1000 > aCDTitel[lCurrTrack-1].lMillisec )
                     slpos = (SHORT) (aCDTitel[lCurrTrack-1].lMillisec / 1000);
                 WinSendMsg( hwndScrollbar, SBM_SETPOS, MPFROMSHORT(slpos), (MPARAM) NULL );
                 break;
            case SB_SLIDERPOSITION: /* Position hat sich durch "tracking" geÑndert */
                 break;

            case SB_SLIDERTRACK: /* Der Slider wird bewegt, Achtung! Timer ist noch aktiv: öberschneidung. */
                 sprintf( acBuffer, "%02i:%02i:%02i:00", lCurrTrack, lMinuten, lSekunden );
                 WinSetWindowText( hwndEFAktuell, acBuffer );
                 return (MRESULT) 0;

            default:
                 return (MRESULT) 0;
          }

          /* Musik neu positionieren */
          SendStringToMci( "set cd time format tmsf wait", hwnd, 0 );
          sprintf( achMciSnd, "seek cd to %i:%i:%i:00 wait", lCurrTrack, lMinuten, lSekunden );
          SendStringToMci( achMciSnd, hwnd, 0 );
          SendStringToMci( "status cd position wait", hwnd, 0 );

          /* Anzeige aktuallisieren */
          WinSetWindowText( hwndEFAktuell, achReturn );

          /* Wurde die Musik gespielt, weiter speilen (sollte eigendlich automatisch gehen)  */
          if( bIsPlaying )
              SendStringToMci( "play cd", hwnd, 0 );

          return( (MRESULT) 0);

     case WM_COMMAND : /* Ein Psuhbutton wurde gedrÅckt */
          if( bIsInit || ! bIsMediaPres )
              return (MRESULT) 0;

          switch (SHORT1FROMMP(mp1)){
            case ID_PB_PLAY: /* Musik abspielen */
                 if( bIsPlaying )
                     break;
                 bIsPlaying = TRUE;
                 SendStringToMci( "play cd", hwnd, 0 );
                 break;
            case ID_PB_PAUSE: /* Musik anhalten */
                 if( ! bIsPlaying )
                     break;
                 bIsPlaying = FALSE;
                 SendStringToMci( "pause cd wait", hwnd, 0 );
                 break;
            case ID_PB_STOP: /* Musik anhalten */
                 bIsPlaying = FALSE;
                 SendStringToMci( "stop cd wait", hwnd, 0 );
                 break;
            case ID_PB_START: /* Zum ersten Titel der CD springen, Dialog aktuallisieren */
                 lCurrTrack = 1;
                 CdpUpdateTrack( lCurrTrack, 0 );
                 break;
            case ID_PB_REWIND: /* Ein Titel zurÅck, Dialog aktuallisieren */
                 lCurrTrack --;
                 if( lCurrTrack < 1 )
                     lCurrTrack = lNumOfTracks;
                 CdpUpdateTrack( lCurrTrack, 0 );
                 break;
            case ID_PB_SECBACKWARD: /* 10 Sekunden des aktuellen Titels zurÅck, Dialog aktuallisieren */
                 SendStringToMci( "status cd position wait", hwnd, 0 );
                 lTrack    = atol( achReturn   );
                 lMinuten  = atol( achReturn+3 );
                 lSekunden = atol( achReturn+6 );

                 lSekunden -= 10;
                 if( lSekunden < 0 )
                 {
                     lSekunden += 60;
                     lMinuten--;
                     if( lMinuten < 0 )
                     {
                         lTrack--;
                         if( lTrack < 1 )
                             lTrack = lNumOfTracks;
                         lMinuten  = atol( aCDTitel[lTrack-1].acLaenge   );
                         lSekunden = atol( aCDTitel[lTrack-1].acLaenge+3 );
                     }
                 }
                 SendStringToMci( "set cd time format tmsf wait", hwnd, 0 );
                 sprintf( achMciSnd, "seek cd to %i:%i:%i:00 wait", lTrack, lMinuten, lSekunden );
                 SendStringToMci( achMciSnd, hwnd, 0 );
                 if( bIsPlaying )
                     SendStringToMci( "play cd", hwndCdplayer, 0 );
                 CdpUpdateAktuell();
                 break;
            case ID_PB_SECFORWARD: /* 10 Sekunden des aktuellen Titels weiter, Dialog aktuallisieren */
                 SendStringToMci( "status cd position wait", hwnd, 0 );
                 lTrack    = atol( achReturn   );
                 lMinuten  = atol( achReturn+3 );
                 lSekunden = atol( achReturn+6 );
                 lMaxMin   = atol( aCDTitel[lTrack-1].acLaenge   );
                 lMaxSec   = atol( aCDTitel[lTrack-1].acLaenge+3 );

                 lSekunden += 10;
                 if( lSekunden > 60 || (lSekunden > lMaxSec && lMinuten == lMaxMin) )
                 {
                     if( lSekunden > 60 )
                         lSekunden -= 60;

                     lMinuten++;

                     if( lMinuten > lMaxMin )
                     {
                         lTrack++;
                         if( lTrack > lNumOfTracks )
                             lTrack = 1;
                         lMinuten  = 0;
                         lSekunden = 0;
                     }
                 }
                 SendStringToMci( "set cd time format tmsf wait", hwnd, 0 );
                 sprintf( achMciSnd, "seek cd to %i:%i:%i:00 wait", lTrack, lMinuten, lSekunden );
                 SendStringToMci( achMciSnd, hwnd, 0 );
                 if( bIsPlaying )
                     SendStringToMci( "play cd", hwndCdplayer, 0 );
                 CdpUpdateAktuell();
                 break;
            case ID_PB_FORWARD: /* Zum nÑchsten Titel springen, Dialog aktuallisieren */ 
                 lCurrTrack ++;
                 if( lCurrTrack > lNumOfTracks )
                     lCurrTrack = 1;
                 CdpUpdateTrack( lCurrTrack, 0 );
                 break;
            case ID_PB_END: /* Zum letzten Titel springen, Dialog aktuallisieren */
                 lCurrTrack = lNumOfTracks;
                 CdpUpdateTrack( lCurrTrack, 0 );
                 break;
            case ID_PB_EJECT: /* CD auswerfen */
                 SendStringToMci( "set cd door open wait", hwnd, 0 );
                 break;
            case ID_PB_MEHR: /* ZusÑtzliche Daten zum Titel erfassen */
                 /* Mehr-Pushbutton ausschalten und Uhr als Zeiger setzen */
                 WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_MEHR ), FALSE );
                 WinSetPointer ( HWND_DESKTOP, hptrWait );

                 /* Aktuelle TitelNr erfragen */
                 WinSendMsg( hwndSpinbutton, SPBM_QUERYVALUE, &lTrack, MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ) );
 
                 /* Dialog laden, Standardgrî·e in SystemmenÅ setzen, Handle erfragen, Dialog anzeigen */
                 bIsMehr = TRUE;
                 hwndFrame2 = WinLoadSecondaryWindow( HWND_DESKTOP, HWND_DESKTOP, (PFNWP) MehrDlgProc, (HMODULE) NULL, ID_DLG_MEHR, (PVOID) lTrack);
                 WinInsertDefaultSize(hwndFrame2, "~Standardgrî·e");
                 hwndCdmehr = WinQuerySecondaryHWND(hwndFrame2, QS_DIALOG);
                 WinShowWindow( hwndFrame2, TRUE );

                 /* Dialog abarbeiten und RÅckgabewert auswerten */
                 ulRet = WinProcessSecondaryWindow( hwndFrame2 );
                 bIsMehr = FALSE;

                 /* Mehr-Pushbutton wieder aktivieren, falls CD noch vorhanden, und Zeiger zurÅcksetzen */
                 WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_MEHR ), bIsMediaPres );
                 WinSetPointer ( HWND_DESKTOP, hptrArrow );

                 /* Wurden die Daten gespeichert, Container aktualisieren */
                 if( ulRet )
                     WinSendMsg( hwndContainer, CM_INVALIDATERECORD, (MPARAM) &aCDTitel[ulRet-1].prcCnrDaten, MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );                    
                 break;
 
            default:
                 break;
          }  
          return( (MRESULT) 0);
     default:
          return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2));
   }
}

/*
 * CdpInitContainer.
 */

VOID CdpInitContainer( VOID )
{
  FIELDINFO *fi1st, *filocal;
  MRESULT rc;
  INT j;

  /* Containerinfo setzen */

  memset( &cnri, 0, sizeof(cnri) );

  cnri.cb               = sizeof(cnri);
  cnri.flWindowAttr     = CV_DETAIL | CA_CONTAINERTITLE | CA_TITLEREADONLY | CA_TITLESEPARATOR | CA_TITLECENTER | CA_DETAILSVIEWTITLES;
  cnri.xVertSplitbar    = -1;
  cnri.pFieldInfoLast   = (PFIELDINFO) NULL;       
  cnri.pFieldInfoObject = (PFIELDINFO) NULL;      
  cnri.cFields          = 0;

  cnriFlags          = 0x7FFF;

  WinSendMsg( hwndContainer, CM_SETCNRINFO, (MPARAM) &cnri, (MPARAM) cnriFlags );

  /* Spalten definieren */
  fi1st = (FIELDINFO *) WinSendMsg( hwndContainer, CM_ALLOCDETAILFIELDINFO, (MPARAM) CNR_COLUMN_NUM, (MPARAM) NULL );

  filocal = fi1st;

  for( j=0; j<CNR_COLUMN_NUM; j++ )
  {
       /* Initialisierung */
       if( j == CNR_COLUMN_NUM-1 )
           filocal->flData = CFA_LEFT | CFA_VCENTER | CFA_FIREADONLY | aulColumnData[j];
       else
           filocal->flData = CFA_LEFT | CFA_VCENTER | CFA_FIREADONLY | CFA_SEPARATOR | aulColumnData[j];

       filocal->flTitle = CFA_CENTER | CFA_VCENTER | CFA_STRING | CFA_FITITLEREADONLY | CFA_HORZSEPARATOR;
       filocal->pTitleData = (PVOID)achColumnTitle[j];
       filocal->offStruct = sizeof(RECORDCORE)+(j-1)*sizeof(PVOID);
       filocal = filocal->pNextFieldInfo;
  }

  /* Erzeugung */
  memset( &fii, 0, sizeof(fii) );
  fii.cb                   = sizeof( fii );
  fii.pFieldInfoOrder      = (PFIELDINFO) CMA_FIRST;
  fii.cFieldInfoInsert     = (SHORT) CNR_COLUMN_NUM;
  fii.fInvalidateFieldInfo = TRUE;

  rc = WinSendMsg( hwndContainer, CM_INSERTDETAILFIELDINFO, MPFROMP( fi1st ), MPFROMP( &fii ) );

}


/*
 * CdpSetupAudioCDInfo.
 */

VOID CdpSetupAudioCDInfo( VOID )
{
  INT          i,j;
  LONG         lStart, lWert;
  SHORT        sSec, sStartSec;
  ULONG        ulPrfLen;
  RECORDCORE  *prc, *rcfirst;
  DATEN        *pri;
  RECORDINSERT rii;
  APIRET       ret;

  /* "Mehr" Dialog schlie·en, falls noch aktiv ... */
  if( bIsMehr )
      WinSendMsg( hwndCdmehr, WM_CLOSE, (MPARAM) NULL, (MPARAM) NULL );

  WinEnableWindowUpdate( hwndCdplayer, FALSE );
  CdpEnableObjects( TRUE );

  lCurrTrack = 0;

  /* CD-ID ermitteln */
  SendStringToMCI( "info cd id", (HWND) NULL, 0 );

  for( i=0; i<8; i++ )
  {
       acHexID[2*(i+1)  ] = acHex[ achReturn[i] / 16 ];
       acHexID[2*(i+1)+1] = acHex[ achReturn[i] % 16 ];
  }

  acHexID[18] = '\0';
  WinSetWindowText( hwndEFCDID, acHexID );

  /* Anzahl der Lieder ermitteln */
  SendStringToMci( "status cd number of tracks wait" );

  lNumOfTracks = atol( achReturn );
        
  /* Profile abfragen, ob CD-ID schon vorhanden, falls ja, Daten auslesen, sonst Daten anlegen */
  memset( acCDTitel, 0, 128 );
  ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, "CD-Titel", (PSZ) NULL, acCDTitel, 128 );
  if( strlen( acCDTitel ) == 0 )
  {
      strcpy( acCDTitel, "< unbekannt >" );
      for( i=0; i<lNumOfTracks; i++ )
      {
           memset( aCDTitel[i].acAutor, 0, 128);
           memset( aCDTitel[i].acTitel, 0, 128);
      }
  }
  else
  {
      for( i=0; i<lNumOfTracks; i++ )
      {
           sprintf( acBuffer, "AUTOR%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acAutor, 128 );
           sprintf( acBuffer, "TITEL%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acTitel, 128 );
           sprintf( acBuffer, "MUSIK%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acMusik, 128 );
           sprintf( acBuffer, "TEXT%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acText, 128 );
           sprintf( acBuffer, "RICHTUNG%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acRichtung, 128 );
           sprintf( acBuffer, "TAKTE%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acTakte, 128 );
           sprintf( acBuffer, "AUSGABE%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acAusgabe, 30 );
           sprintf( acBuffer, "BEMERKUNG%i", i+1);
           ulPrfLen = PrfQueryProfileString( hiniCdplayer, acHexID, acBuffer, (PSZ) NULL, aCDTitel[i].acBemerkung, 4096 );
      }
  }

  /* CD auf das erste Lied positionieren */
  SendStringToMci( "seek cd to start wait" );
  lCurrTrack = 1;

  WinSetWindowText( hwndEFCDTitel, acCDTitel );
  WinSetWindowText( hwndEFAutor, aCDTitel[0].acAutor );
  WinSetWindowText( hwndEFTitel, aCDTitel[0].acTitel );

  /* Spinbutton initialisieren */
  WinSendMsg( hwndSpinbutton, SPBM_SETLIMITS, (MPARAM) lNumOfTracks, (MPARAM) 1 );
  WinSendMsg( hwndSpinbutton, SPBM_SETCURRENTVALUE, (MPARAM) 1, (MPARAM) NULL );

  WinSendMsg( hwndSpinVolume, SPBM_SETLIMITS, (MPARAM) 100, (MPARAM) 0 );
  WinSendMsg( hwndSpinVolume, SPBM_SETCURRENTVALUE, (MPARAM) lVolume, (MPARAM) NULL );

  sprintf( acBuffer, "set cd audio volume %i wait", lVolume );
  SendStringToMci( acBuffer, hwndCdplayer, 0 );

  /* Die LÑngen der einzelnen Tracks erfragen und in CDTITELINFO ablegen */
  for( i=0; i<lNumOfTracks; i++ )
  {
       SendStringToMci( "set cd time format milliseconds wait" );
       sprintf( achMciSnd, "status cd length track %i wait", i+1 );
       SendStringToMci( achMciSnd, (HWND) NULL, 0 );
       aCDTitel[i].lMillisec = atol( achReturn );

       SendStringToMci( "set cd time format msf wait" );
       sprintf( achMciSnd, "status cd length track %i wait", i+1 );
       SendStringToMci( achMciSnd, (HWND) NULL, 0 );
       strcpy(aCDTitel[i].acLaenge, achReturn );
  }

  SendStringToMci( "set cd time format msf wait" );

  /* Container fÅllen */
  for( i=0; i<lNumOfTracks; i++ )
  {
    rcfirst = (PRECORDCORE) WinSendMsg( hwndContainer, CM_ALLOCRECORD, (MPARAM) ((CNR_COLUMN_NUM) * sizeof(DATEN)), (MPARAM) 1 );

    aCDTitel[i].prcCnrDaten = rcfirst;

    pri = (DATEN *)(((BYTE *) rcfirst) + sizeof(RECORDCORE));
    pri--;

    pri[0].lWert     = i+1;
    pri[1].pszString = aCDTitel[i].acTitel;
    pri[2].pszString = aCDTitel[i].acAutor;
    pri[3].pszString = aCDTitel[i].acLaenge;
    pri[4].pszString = aCDTitel[i].acMusik;
    pri[5].pszString = aCDTitel[i].acText;
    pri[6].pszString = aCDTitel[i].acRichtung;
    pri[7].pszString = aCDTitel[i].acAusgabe;
    pri[8].pszString = aCDTitel[i].acBemerkung;
    pri[9].lWert     = aCDTitel[i].lMillisec;

    memset( &rii, 0, sizeof(rii) );
    rii.cb                = sizeof(RECORDINSERT);
    rii.pRecordOrder      = (PRECORDCORE) CMA_END;
    rii.pRecordParent     = (PRECORDCORE) NULL;
    rii.zOrder            = CMA_TOP;
    rii.cRecordsInsert    = 1;
    rii.fInvalidateRecord = TRUE;
   
    WinSendMsg( hwndContainer, CM_INSERTRECORD, (MPARAM) rcfirst, (MPARAM) &rii );

  }

  /* Containertitel aktualisieren */
  memset( &cnri, 0, sizeof(cnri));
  cnri.pszCnrTitle = acCDTitel;
  cnriFlags        = CMA_CNRTITLE;
  WinSendMsg( hwndContainer, CM_SETCNRINFO, (MPARAM) &cnri, (MPARAM) cnriFlags );

  /* Spielzeit des ersten Liedes in Eingabefeld Åbertragen */
  WinSetWindowText( hwndEFLaenge, aCDTitel[0].acLaenge );
  WinSetWindowText( hwndEFAktuell, "01:00:00:00" );

  /* Scrollbar setzten */
  sSec = (SHORT)(aCDTitel[0].lMillisec / 1000);
  WinSendMsg( hwndScrollbar, SBM_SETSCROLLBAR, (MPARAM) 0, MPFROM2SHORT( 0, sSec ) );
  WinSendMsg( hwndScrollbar, SBM_SETTHUMBSIZE, MPFROM2SHORT(1, sSec), (MPARAM) NULL );

  /* Checkbutton setzten */
  bIsLinked = TRUE;
  WinSendMsg( hwndCheckbox, BM_SETCHECK, (MPARAM) bIsLinked, (MPARAM) NULL );

  bIsMediaPres = TRUE;
  WinEnableWindowUpdate( hwndCdplayer, TRUE );
  WinShowWindow( hwndCdplayer, TRUE );

}


/*
 * CdpClearAudioCDInfo.
 */

VOID CdpClearAudioCDInfo( VOID )
{
  bIsMediaPres = FALSE;

  /* "Mehr" Dialog schlie·en, falls noch aktiv ... */
  if( bIsMehr )
      WinSendMsg( hwndCdmehr, WM_CLOSE, (MPARAM) NULL, (MPARAM) NULL );

  WinEnableWindowUpdate( hwndCdplayer, FALSE );

  /* Eingabefeldinhalt lîschen */
  WinSetWindowText( hwndEFCDTitel, "" );
  WinSetWindowText( hwndEFCDID   , "" );
  WinSetWindowText( hwndEFAutor  , "" );
  WinSetWindowText( hwndEFTitel  , "" );
  WinSetWindowText( hwndEFLaenge , "" );
  WinSetWindowText( hwndEFAktuell, "" );

  /* Scrollbar zurÅcksetzten  */
  WinSendMsg( hwndScrollbar, SBM_SETSCROLLBAR, (MPARAM) 0, (MPARAM) MPFROM2SHORT( 0, 0 ) );
  WinSendMsg( hwndScrollbar, SBM_SETTHUMBSIZE, MPFROM2SHORT(0, 0), (MPARAM) NULL );

  /* Spinbutton zurÅcksetzten */
  WinSendMsg( hwndSpinbutton, SPBM_SETLIMITS, (MPARAM) 0, (MPARAM) 0 );

  /* Checkbutton zurÅcksetzten */
  bIsLinked = FALSE;
  WinSendMsg( hwndCheckbox, BM_SETCHECK, (MPARAM) bIsLinked, (MPARAM) NULL );

  /* Container zurÅcksetzten  */
  memset( &cnri, 0, sizeof(cnri));
  cnri.pszCnrTitle = "Legen Sie bitte eine Audio-CD ein ..."; 
  cnriFlags        = CMA_CNRTITLE;

  lNumOfTracks = 0;

  WinSendMsg( hwndContainer, CM_SETCNRINFO, (MPARAM) &cnri, (MPARAM) cnriFlags );
  WinSendMsg( hwndContainer, CM_REMOVERECORD, (MPARAM) NULL, MPFROM2SHORT(0, (CMA_FREE | CMA_INVALIDATE) ) );

  CdpEnableObjects( FALSE );
  WinEnableWindowUpdate( hwndCdplayer, TRUE );
  WinShowWindow( hwndCdplayer, TRUE );
}


/*
 * CdpEnableObjects.
 */

VOID CdpEnableObjects( BOOL bFlag )
{
  WinEnableWindow( hwndContainer , bFlag );
  WinEnableWindow( hwndSpinVolume, bFlag );
  WinEnableWindow( hwndSpinbutton, bFlag );
  WinEnableWindow( hwndScrollbar , bFlag );
  WinEnableWindow( hwndCheckbox  , bFlag );
  WinEnableWindow( hwndEFCDTitel , bFlag );
  WinEnableWindow( hwndEFCDID    , bFlag );
  WinEnableWindow( hwndEFAutor   , bFlag );
  WinEnableWindow( hwndEFTitel   , bFlag );
  WinEnableWindow( hwndEFLaenge  , bFlag );
  WinEnableWindow( hwndEFAktuell , bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_PLAY        ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_PAUSE       ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_STOP        ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_START       ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_REWIND      ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_SECBACKWARD ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_SECFORWARD  ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_FORWARD     ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_END         ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_EJECT       ), bFlag );
  WinEnableWindow( WinWindowFromID( hwndCdplayer, ID_PB_MEHR        ), bFlag );
}


/*
 * SendStringToMCI.
 */

ULONG SendStringToMCI( PSZ pszCmd, HWND hwndNotify, USHORT usUserParam )
{
  USHORT usResult;
  ULONG ulRet, lErr;

  memset( achReturn, 0, MCI_RETURN_STRING_LENGTH );
  memset( achError , 0, MCI_ERROR_STRING_LENGTH );

  while( ulRet = mciSendString( pszCmd, achReturn, MCI_RETURN_STRING_LENGTH, hwndNotify, usUserParam ) )
  {
    lErr = mciGetErrorString( ulRet, achError, MCI_ERROR_STRING_LENGTH );

    if( lErr != MCIERR_SUCCESS ) /* Fehler bei Fehlerauswertung */
    {
        strcpy( achError, "Es ist ein Fehler bei der Fehlerauswertung aufgetreten." );
    }
    usResult = WinMessageBox( HWND_DESKTOP, hwndNotify, achError, pszCmd, 1000, MB_ABORTRETRYIGNORE | MB_MOVEABLE | MB_APPLMODAL | MB_INFORMATION );
    switch( usResult ) {
      case MBID_ABORT:
           WinSendMsg( hwndCdplayer, WM_CLOSE, (MPARAM) NULL, (MPARAM) NULL );
      case MBID_IGNORE:
           return(ulRet);
    }
  }
  return(ulRet);
}

VOID CdpUpdateAktuell( VOID )
{
  LONG lTrack, lMinuten, lSekunden, lMaxMin, lMaxSec;

  SendStringToMci( "set cd time format tmsf wait", hwndCdplayer, 0 );
  SendStringToMci( "status cd position wait" );

  lTrack    = atol( achReturn );
  lMinuten  = atol( achReturn+3 );
  lSekunden = atol( achReturn+6 );

  lMaxMin   = atol( aCDTitel[lTrack-1].acLaenge   );
  lMaxSec   = atol( aCDTitel[lTrack-1].acLaenge+3 );

  WinSetWindowText( hwndEFAktuell, achReturn );

  if( lTrack != lCurrTrack )
  {
      CdpUpdateTrack( lTrack, CDP_KEEPTIME );
  }
  else
  {
      WinSendMsg( hwndScrollbar, SBM_SETPOS, MPFROMSHORT(lMinuten*60+lSekunden), (MPARAM) NULL );
  }

  if( lTrack == lNumOfTracks && lSekunden >= lMaxSec && lMinuten >= lMaxMin )
      WinSendMsg( hwndCdplayer, WM_COMMAND, (MPARAM) ID_PB_START, (MPARAM) NULL );

}

VOID CdpUpdateTrack( LONG lTrack, LONG lFlag )
{
  LONG lMinuten, lSekunden, lLocal;
  SHORT sSec;
  RECTL rct, vrct;
  QUERYRECORDRECT qrrct;
  
  WinSetPointer ( HWND_DESKTOP, hptrWait );

  if( bIsLinked )
      lLocal = lUnlinkedTrack = lCurrTrack = lTrack;
  else if( lFlag & CDP_KEEPTIME )
      lCurrTrack = lTrack;
  else 
      lUnlinkedTrack = lTrack;

  if( bIsLinked || lFlag & CDP_UPDATETEXTONLY )
  {
      if( !(lFlag & CDP_KEEPTIME) )
          lLocal = lUnlinkedTrack;
      else
          lLocal = lCurrTrack;
    
      WinSendMsg( hwndSpinbutton, SPBM_SETCURRENTVALUE, (MPARAM) lLocal, (MPARAM) NULL );

      if( !(lFlag & CDP_NOCNRUPDATE ) )
      {
          /* Der Datensatz wird im Container aktiviert */
          WinSendMsg( hwndContainer, CM_SETRECORDEMPHASIS, (MPARAM) aCDTitel[lLocal-1].prcCnrDaten, MPFROM2SHORT( TRUE, CRA_SELECTED | CRA_CURSORED ) );

          /* Liegt der aktive Datensatz im unsichtbaren Bereich des Containers, so mu· gescrollt werden */
          qrrct.cb = sizeof(QUERYRECORDRECT);
          qrrct.pRecord = (PRECORDCORE) aCDTitel[lLocal-1].prcCnrDaten;
          qrrct.fRightSplitWindow = FALSE;
          qrrct.fsExtent = CMA_TEXT;

          WinSendMsg( hwndContainer, CM_QUERYRECORDRECT, (MPARAM) &rct, (MPARAM) &qrrct );
          WinSendMsg( hwndContainer, CM_QUERYVIEWPORTRECT, (MPARAM) &vrct, MPFROM2SHORT( CMA_WINDOW, FALSE ) );

          if( vrct.yBottom > rct.yBottom )
              WinSendMsg( hwndContainer, CM_SCROLLWINDOW, (MPARAM) CMA_VERTICAL, (MPARAM) (vrct.yBottom-rct.yBottom) );
          else if( vrct.yTop < rct.yTop )
              WinSendMsg( hwndContainer, CM_SCROLLWINDOW, (MPARAM) CMA_VERTICAL, (MPARAM) (vrct.yTop-rct.yTop) );
      }

      WinSetWindowText( hwndEFAutor,   aCDTitel[lLocal-1].acAutor  );
      WinSetWindowText( hwndEFTitel,   aCDTitel[lLocal-1].acTitel  );
      WinSetWindowText( hwndEFLaenge,  aCDTitel[lLocal-1].acLaenge );
  }

  if( !(lFlag & CDP_UPDATETEXTONLY ) )
  {
      if( lFlag & CDP_KEEPTIME )
      {
          SendStringToMci( "set cd time format tmsf wait", hwndCdplayer, 0 );
          SendStringToMci( "status cd position wait" );

          lMinuten  = atol( achReturn+3 );
          lSekunden = atol( achReturn+6 );

          WinSetWindowText( hwndEFAktuell, achReturn );

          sSec = (SHORT)(aCDTitel[lCurrTrack-1].lMillisec / 1000);
          WinSendMsg( hwndScrollbar, SBM_SETSCROLLBAR, (MPARAM) (lMinuten*60+lSekunden), MPFROM2SHORT( 0, sSec ) );
          WinSendMsg( hwndScrollbar, SBM_SETTHUMBSIZE, MPFROM2SHORT(1, sSec), (MPARAM) NULL );
      }
      else
      {
          sSec = (SHORT)(aCDTitel[lCurrTrack-1].lMillisec / 1000);
          WinSendMsg( hwndScrollbar, SBM_SETSCROLLBAR, (MPARAM) 0, MPFROM2SHORT( 0, sSec ) );
          WinSendMsg( hwndScrollbar, SBM_SETTHUMBSIZE, MPFROM2SHORT(1, sSec), (MPARAM) NULL );
    
          SendStringToMci( "set cd time format tmsf wait", hwndCdplayer, 0 );
          sprintf( achMciSnd, "seek cd to %i:00:00:00 wait", lCurrTrack );
          SendStringToMci( achMciSnd, hwndCdplayer, 0 );
          SendStringToMci( "status cd position wait", hwndCdplayer, 0 );
          WinSetWindowText( hwndEFAktuell, achReturn );

          if( bIsPlaying )
              SendStringToMci( "play cd", hwndCdplayer, 0 );
      }
  }

  WinSetPointer ( HWND_DESKTOP, hptrArrow );
}

/* MehrDlgProc. */
MRESULT EXPENTRY MehrDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  USHORT   id;
  ULONG    ni;
  SHORT    nc;
  static LONG lEditTrack;

   switch( msg ) {
     case WM_INITDLG :
          hwndCdmehr     = hwnd;
          hwndEFMusik    = WinWindowFromID( hwnd, ID_EF_MUSIK        );
          hwndEFText     = WinWindowFromID( hwnd, ID_EF_TEXT         );
          hwndEFRichtung = WinWindowFromID( hwnd, ID_EF_RICHTUNG     );
          hwndEFTakte    = WinWindowFromID( hwnd, ID_EF_TAKTE        );
          hwndEFAusgabe  = WinWindowFromID( hwnd, ID_EF_AUSGABE      );
          hwndMLEBemerk  = WinWindowFromID( hwnd, ID_MLE_BEMERKUNGEN );
 
          WinSendMsg( hwndEFMusik   , EM_SETTEXTLIMIT,  (MPARAM)  128, (MPARAM) NULL );
          WinSendMsg( hwndEFText    , EM_SETTEXTLIMIT,  (MPARAM)  128, (MPARAM) NULL );
          WinSendMsg( hwndEFRichtung, EM_SETTEXTLIMIT,  (MPARAM)  128, (MPARAM) NULL );
          WinSendMsg( hwndEFTakte   , EM_SETTEXTLIMIT,  (MPARAM)  128, (MPARAM) NULL );
          WinSendMsg( hwndEFAusgabe , EM_SETTEXTLIMIT,  (MPARAM)   30, (MPARAM) NULL );
          WinSendMsg( hwndMLEBemerk , MLM_SETTEXTLIMIT, (MPARAM) 4096, (MPARAM) NULL );

          lEditTrack = (LONG) mp2;

          WinSetWindowText( hwndEFMusik,    aCDTitel[lEditTrack-1].acMusik     );
          WinSetWindowText( hwndEFText,     aCDTitel[lEditTrack-1].acText      );
          WinSetWindowText( hwndEFRichtung, aCDTitel[lEditTrack-1].acRichtung  );
          WinSetWindowText( hwndEFTakte,    aCDTitel[lEditTrack-1].acTakte     );
          WinSetWindowText( hwndEFAusgabe,  aCDTitel[lEditTrack-1].acAusgabe   );
          WinSetWindowText( hwndMLEBemerk,  aCDTitel[lEditTrack-1].acBemerkung );

          return( (MRESULT) 0);

     case WM_COMMAND:
          switch( SHORT1FROMMP(mp1) ){
            case ID_PB_SPEICHERN:
                 WinQueryWindowText( hwndEFMusik,    128, aCDTitel[lEditTrack-1].acMusik     );
                 WinQueryWindowText( hwndEFText,     128, aCDTitel[lEditTrack-1].acText      );
                 WinQueryWindowText( hwndEFRichtung, 128, aCDTitel[lEditTrack-1].acRichtung  );
                 WinQueryWindowText( hwndEFTakte,    128, aCDTitel[lEditTrack-1].acTakte     );
                 WinQueryWindowText( hwndEFAusgabe,   30, aCDTitel[lEditTrack-1].acAusgabe   );
                 WinQueryWindowText( hwndMLEBemerk, 4096, aCDTitel[lEditTrack-1].acBemerkung );
                 WinDismissSecondaryWindow( hwnd, lEditTrack );
                 sprintf( acBuffer, "MUSIK%i", lEditTrack );
                 PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lEditTrack-1].acMusik );
                 sprintf( acBuffer, "TEXT%i", lEditTrack );
                 PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lEditTrack-1].acText );
                 sprintf( acBuffer, "RICHTUNG%i", lEditTrack );
                 PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lEditTrack-1].acRichtung );
                 sprintf( acBuffer, "TAKTE%i", lEditTrack );
                 PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lEditTrack-1].acTakte );
                 sprintf( acBuffer, "AUSGABE%i", lEditTrack );
                 PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lEditTrack-1].acAusgabe );
                 sprintf( acBuffer, "BEMERKUNG%i", lEditTrack );
                 PrfWriteProfileString( hiniCdplayer, acHexID, acBuffer, aCDTitel[lEditTrack-1].acBemerkung );
                 break;
            case ID_PB_ABBRECHEN:
                 WinDismissSecondaryWindow( hwnd, FALSE );
                 break;
            default:
                 break;
          }
          return (MRESULT) 0;

     case WM_CLOSE :
          return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2 ) );

     default:
          return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2));
   }
}

