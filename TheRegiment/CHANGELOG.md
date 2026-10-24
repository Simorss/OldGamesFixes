# Changelog

## 1.0

- First public release: Hor+ camera correction, independent weapon FOV, mouse filter removal, runtime hotkeys and FOV display after resolution changes.
- Reordered and simplified the INI, with mouse controls directly below the master switch.
- Minimal four-file ZIP: loader, ASI, INI and one ThirteenAG license beside the plugin.
- Release page, project catalogue and original before/after screenshots.

## 0.2.4-preview

- OSD automatico del FOV camera al cambio di risoluzione, dopo la conferma delle nuove dimensioni nella camera del giocatore.
- Usate le dimensioni effettive del back buffer per distinguere il cambio risoluzione dai passaggi off-screen/LOD.
- Il timer parte una volta sola e usa DurationMs; un cambio a rapporto invariato mostra il FOV effettivo anche se numericamente uguale.
- Nessuna nuova opzione INI. Conservati interruttore globale, FOV indipendenti e sensibilita del menu.
- Test per cambi di rapporto, cambi rapidi, scene secondarie, proiezione senza Hor+, OSD nascosto e dimensioni reali dopo Reset D3D9.

## 0.2.3-preview

- Aggiunto Main.Enabled in cima all'INI per attivare o disattivare completamente il fix al successivo avvio.
- Con Enabled=0 nessun hook, modifica dei FOV/filtri mouse, comando, OSD o migrazione dell'INI.
- Gli INI precedenti mantengono il comportamento attivo e acquisiscono la nuova voce senza perdere impostazioni.
- Verificati avvio disabilitato, riattivazione e compatibilita con INI privi dell'opzione tramite loader reale.

## 0.2.2-preview

- Rimosso completamente l'override della sensibilita: la regola il menu del gioco.
- Il campo di sensibilita non viene piu incluso nel ripristino dopo l'input, preservando anche cambi effettuati durante l'evento originale.
- Eliminati i tasti virgola/punto, l'OSD e le opzioni di sensibilita del plugin; migrazione INI senza perdere i FOV.
- Mantenuti disattivazione di accelerazione/smoothing, F7/F8, F11/F12 e correzione del crash viewmodel della 0.2.1.
- Aggiunti test di regressione per sensibilita nativa, variazioni del menu e rimozione delle opzioni obsolete.

## 0.2.1-preview

- Corretto il crash al caricamento missione: il viewmodel legge SizeX/SizeY dal viewport senza scambiare FRenderInterface per FRenderTarget.
- Aggiunta una prova di regressione che fallisce con il codice 0.2.0 e passa con la correzione.
- Preservati i sei tasti, il mouse senza filtri e l'INI gia configurato.
- Lo script Update accetta anche la build 0.2.0 riconosciuta.

## 0.2.0-preview

- F7/F8 regolano la camera; F11/F12 il viewmodel; virgola/punto la sensibilita.
- FOV delle armi indipendente dalla camera, con Hor+ e salvataggio dedicati.
- Accelerazione e smoothing del gioco disattivati durante l'input.
- Sensibilita assoluta regolabile da 0,001 a 20, passi percentuali e sei decimali salvati.
- OSD specifico per ogni regolazione; sei tasti configurabili e controllo dei duplicati.
- Migrazione del vecchio INI conservando il FOV camera gia scelto.
- Script di aggiornamento con backup e conservazione dell'INI.

## 0.1.0-preview

- Prima correzione Hor+ della camera con regolazione in gioco e OSD.
- Funzionamento della camera confermato dall'utente nel gioco.
