# Aggiornamento alla release 1.0

A gioco chiuso sostituire soltanto `system/scripts/TheRegiment.FOVFix.asi` con il nuovo ASI. Conservare il proprio INI e il loader `dinput8.dll`.

La 1.0 mantiene le funzioni della 0.2.4 e riordina l'INI. Lo ZIP include solo loader, ASI, INI e system/scripts/ThirteenAG-LICENSE.txt. Documentazione e screenshot sono su GitHub.

La 0.2.4 mostra automaticamente in alto il FOV effettivo dopo ogni cambio di risoluzione, appena viene aggiornata la camera del giocatore. Non richiede nuove opzioni: usa OSD.DurationMs e rispetta DurationMs=0 e Main.Enabled=0. Se il rapporto d'aspetto resta uguale, anche il FOV puo restare uguale.

La 0.2.3 aggiunge `[Main] Enabled=1` se assente, anche negli INI con ConfigVersion=3. Impostare `Enabled=0` e riavviare per disabilitare completamente camera, viewmodel, filtri mouse, comandi e OSD. Quando disabilitato, il plugin lascia intatto l'INI.

Al prossimo avvio attivo il plugin mantiene i FOV e i quattro tasti gia configurati. Elimina `Mouse.Sensitivity`, `Mouse.StepPercent`, `Mouse.SaveSensitivity` e i due tasti `SensitivityDecrease/Increase`, poi imposta `ConfigVersion=3`. Queste voci non vengono piu usate, anche se il file non e scrivibile. La sensibilita segue esclusivamente il menu del gioco; accelerazione e smoothing restano disattivati con le impostazioni predefinite.

| Controllo | Diminuisce | Aumenta |
| --- | --- | --- |
| FOV camera | F7 | F8 |
| FOV viewmodel | F11 | F12 |

Virgola e punto non controllano piu la sensibilita. Restano configurabili i quattro comandi FOV. La correzione del crash della 0.2.0 al caricamento missione e mantenuta.

In alternativa, dalla cartella del progetto:

```powershell
powershell -NoProfile -File .\tools\Update.ps1 -GameDirectory 'E:\Giochi\The Regiment'
```

Lo script riconosce le build 0.1.0, 0.2.0, 0.2.1, 0.2.2, 0.2.3 e 0.2.4 verificate, salva ASI e INI con estensioni `.bak`, sostituisce il plugin e conserva l'INI byte per byte. La migrazione dell'INI avviene all'avvio del plugin. I backup non vengono caricati come ASI. Il manifest eventualmente presente viene aggiornato per permettere la successiva disinstallazione.

Lo ZIP contiene anche un INI predefinito per nuove installazioni; usarlo al posto del proprio riporta i due FOV a 70 gradi di riferimento. L'assistente non ha modificato l'installazione reale o avviato il gioco. La prova del menu e del gameplay resta manuale.
