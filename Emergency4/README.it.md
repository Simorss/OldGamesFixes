# Emergency 4: Resolution & Aspect Ratio Fix

**Versione 1.0.0-beta.1 — Windows.**

Imposta una risoluzione personalizzata e corregge il campo visivo orizzontale
del gameplay, conservando quello verticale. Permette inoltre di scegliere
tra due modalita' per **menu e HUD insieme**:

- **Original**: proporzioni 4:3 originali, interfaccia centrata senza deformazione.
  Il gameplay continua a riempire lo schermo. A 3440x1440 l'interfaccia occupa
  l'area centrale 1920x1440; su schermi piu' stretti di 4:3 viene adattata alla
  larghezza, con spazio sopra e sotto.
- **Stretched**: interfaccia estesa a tutto lo schermo, con lo stiramento
  previsto dal gioco. La correzione del gameplay resta attiva.

## Compatibilita'

Supporta **una specifica build di Emergency 4 Deluxe / Edizione Oro**:

```text
Em4Deluxe.exe
SHA-256 originale:
17924F0E847FB0173DDBD5FDB5D5CBBF68F75CF1DB6FDCADB155F964A350FDC9
```

Sono riconosciuti anche i precedenti fix di questo progetto per 3440x1440.
Altri eseguibili, inclusi quelli chiamati `em4.exe`, non sono ancora supportati:
lo script si interrompe senza applicare modifiche.

La risoluzione e' parametrica e non e' limitata a 3440x1440, ma deve essere
supportata dal gioco, dal monitor e dal driver. Intervallo di input:
larghezza 320–16384, altezza 200–16384. Accettare un valore non garantisce
che il motore riesca ad avviarsi in quella modalita'.

## Come usarlo

1. Scarica lo ZIP dalla sezione **Releases** ed estrailo completamente.
2. Chiudi il gioco e fai doppio clic su **Emergency4-Fix.cmd**.
3. Scegli **Applica**, incolla il percorso della cartella del gioco, inserisci
   la risoluzione (esempio `3440x1440`) e scegli la modalita' dell'interfaccia.
4. Avvia il gioco normalmente.

Puoi anche copiare i tre file di `src` accanto a `Em4Deluxe.exe`: in questo
caso il percorso viene rilevato automaticamente. Non occorre Python,
non vengono scaricati componenti e non viene installato un launcher.
Sono necessari Windows PowerShell 5.1, normalmente incluso in Windows,
e permessi di scrittura nella cartella del gioco.

Per cambiare risoluzione o modalita', esegui di nuovo lo script. Il valore
focale viene ricalcolato dalla base iniziale, senza accumulare correzioni.
Non cambiare la risoluzione dalle vecchie opzioni del gioco dopo aver applicato
il fix: possono sovrascrivere la configurazione. Se succede, riapplica lo script.

## Backup e ripristino

Al primo utilizzo viene creata nella cartella del gioco:

```text
OldGamesFixes-Emergency4-Backup/
  Em4Deluxe.exe
  em4deluxe.cfg
  state.json
```

Il backup non viene sovrascritto nelle applicazioni successive. Non cancellarlo:
serve per cambiare impostazioni e ripristinare. La scelta **Ripristina** del menu
riporta eseguibile e configurazione alla situazione precedente al primo utilizzo
di questo script. Se erano gia' presenti i vecchi fix, torneranno quelli.
Una copia della configurazione corrente viene conservata prima del ripristino.
I vecchi backup `UltrawideFix-Backup` e `MenuHUD-Fix-Backup` restano separati.

## Uso da PowerShell

```powershell
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Width 2560 -Height 1440 -InterfaceMode Original -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Width 1920 -Height 1080 -InterfaceMode Stretched -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Width 3440 -Height 1440 -InterfaceMode Original -Preview -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Restore -NonInteractive
```

`-Preview` mostra le impostazioni e l'hash risultante senza scrivere file.

## Stato delle verifiche

Il gameplay della prima correzione a 3440x1440 e' stato confermato funzionante
dall'utente. La versione parametrica viene verificata su copie private dei file,
con test dello script e delle routine reali dell'interfaccia in emulazione x86.
Il risultato visivo della nuova interfaccia e delle altre risoluzioni deve
ancora essere confermato in partita: per questo la release e' **beta**.

Filmati, viste 3D incorporate nelle finestre e interfacce particolari delle mod
possono richiedere adattamenti separati. Non vengono modificati layout XML,
texture o DLL. Dettagli in [docs/TECHNICAL.md](docs/TECHNICAL.md) e risultati in
[docs/validation.json](docs/validation.json).

## Sorgenti e release

- `src/`: script PowerShell, avvio con doppio clic e manifest dei byte.
- `tests/`: verifiche riproducibili con file del gioco forniti localmente.
- `docs/`: dettagli tecnici e rapporto dei test.
- `build-release.ps1`: costruisce lo ZIP con i soli file distribuibili.
- `release/`: ZIP e checksum generati, da allegare a GitHub Releases.

Il pacchetto non contiene file originali o modificati del gioco.
