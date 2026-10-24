# Verifica manuale 1.0

Le modifiche sono verificate con fixture automatiche. L'assistente non ha avviato o controllato il gioco. La camera 0.1.0 era stata confermata funzionante dall'utente; questa versione richiede la prova manuale seguente.

1. A gioco chiuso sostituire solo l'ASI e mantenere l'INI. Dopo l'avvio verificare `ConfigVersion=3`, i FOV conservati e la rimozione delle vecchie voci della sensibilita.
2. Caricare una missione e controllare `Ready`, `Player camera active`, `Viewmodel active` e `Mouse filters active` nel log. Provare anche cambio missione e riavvio livello.
3. Cambiare la sensibilita nel menu da bassa ad alta e tornare alla missione: la velocita deve cambiare subito secondo il menu. Ripetere in entrambe le direzioni e dopo riavvio del gioco. Il fix non deve imporre un valore proprio.
4. Premere virgola/punto: nessuna regolazione della sensibilita e nessuna scritta MOUSE dal fix. Eventuali binding nativi del gioco continuano a funzionare.
5. A parita di sensibilita del menu, risoluzione, DPI e mira, muovere il mouse della stessa distanza fisica lentamente e velocemente: confrontare la rotazione. Verificare arresti senza coda dello smoothing.
6. Controllare inversione Y, menu di azione e fattore di sensibilita durante la mira. Provare separatamente DisableAcceleration=0 e DisableSmoothing=0 con riavvio per confronto, quindi ripristinare 1.
7. F7/F8 devono regolare il FOV del mondo con OSD, senza cambiare il viewmodel; F11/F12 devono regolare solo armi/mano, senza cambiare l'inquadratura del mondo. Provare pressioni singole e prolungate.
8. Provare armi diverse, pistola, granata, cambio arma, ricarica e ottiche. Verificare clipping, allineamento del mirino e ritorno alla visuale normale. Torcia/laser durante le ottiche non devono far ricomparire l'arma nascosta.
9. Confrontare 4:3, 16:9 e 3440x1440. Hor+ deve mantenere l'apertura verticale; verificare indipendentemente il viewmodel con HorPlus=0 e 1.
10. Provare Alt+Tab, coppie di tasti premute insieme e Ctrl/Alt/Shift. Il fix non deve cambiare valori con un'altra applicazione in primo piano.
11. Uscire e riaprire: i FOV devono persistere; la sensibilita deve seguire il normale salvataggio del gioco. Provare anche SaveFOV=0 per ciascuna sezione.
12. Cambiare i quattro tasti FOV nell'INI e riavviare, poi provare cambi di risoluzione e OSD. Verificare leggibilita delle due scritte e rendering integro.

13. A gioco chiuso impostare `[Main] Enabled=0` e riavviare: nessuna correzione dei FOV, nessuna modifica dei filtri mouse, nessun comando o OSD del fix. Il log deve riportare `Disabled by [Main] Enabled=0. No hooks installed`; i valori nell'INI devono restare conservati.
14. Chiudere, rimettere Enabled=1 e riavviare: i FOV e i tasti salvati devono tornare attivi, insieme ai filtri mouse configurati. La sensibilita deve continuare a seguire il menu in entrambi i casi.

15. Attendere la scomparsa dell'OSD, quindi cambiare risoluzione senza premere i tasti FOV. Tornando alla visuale del giocatore deve comparire in alto il FOV effettivo per DurationMs. Ripetere 4:3 -> 16:9 -> 21:9 -> 4:3 e tra due risoluzioni dello stesso rapporto: in quest'ultimo caso il valore puo restare uguale. La scritta deve sparire dopo la durata prevista e non riapparire ogni frame. Provare cambi consecutivi, cambio dal menu principale, mira e ritorno alla visuale normale. Con DurationMs=0 o Main.Enabled=0 non deve comparire.

Per segnalare un problema fornire risoluzione, arma/stato di mira, passaggi per riprodurlo, INI e log del plugin. Non occorrono file originali del gioco.
