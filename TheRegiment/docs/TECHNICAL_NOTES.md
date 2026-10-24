# Dettagli tecnici 1.0

La build supportata incorpora staticamente il motore. Le DLL Engine/Core presenti nella cartella sono state usate come riferimento per i simboli; gli hook agiscono nell'EXE verificato per SHA256. Nessuna scansione indiscriminata o modifica di file del gioco e necessaria.

## Camera

La correzione 0.1.0 rimane sui due costruttori di scene del giocatore agli RVA `0x0006FCF0` e `0x001F9160`, con trampoline di otto byte. Dopo il costruttore originale il plugin cambia FOV a `+0x1E4` e ricostruisce le matrici tramite lo slot virtuale 13. La correzione successiva al costruttore evita una doppia applicazione nelle scene LOD annidate.

Il rapporto d'aspetto e ricavato dal render target, metodi virtuali GetWidth/GetHeight agli slot 9/10. La formula e:

```text
corrected = 2 atan(tan(incoming/2) * aspect/(4/3)
                   * tan(reference/2)/tan(70/2))
```

Disattivando HorPlus si omette il fattore del rapporto d'aspetto. Il moltiplicatore di zoom viene conservato nello spazio della tangente, entro i limiti di FOV di uscita.

## Viewmodel

Il percorso locale esaminato disegna l'arma tramite `UCanvas::DrawActor`, con FOV calcolato dal controller e dal DisplayFOV dell'arma. Il nuovo hook all'RVA `0x0015EBF0` usa i primi cinque byte completi `push ebp; mov ebp,esp; push -1`.

Si correggono solo attori della gerarchia **SASWeapon**, con ClearZ attivo, wireframe disattivo e FOV valido positivo. Il filtro percorre UObject.Class a `+0x24`, UClass.SuperField a `+0x28`, e risolve i nomi usando le funzioni native agli RVA `0x00038A40` e `0x000388B0`. Il viewport del canvas si trova a `+0x1DC`. Le dimensioni per il viewmodel provengono da SizeX/SizeY agli offset `+0x88` e `+0x8C`, letti prima della chiamata originale che li modifica temporaneamente per un passaggio 512x512. Il membro a `+0x148` e FRenderInterface*, non FRenderTarget*: non deve essere interrogato come se avesse GetWidth/GetHeight. Questo errore nella 0.2.0 poteva causare il crash in missione; la 0.2.1 non esegue chiamate virtuali per ottenere il rapporto del viewmodel.

I passaggi FOV=0 non vengono modificati: il codice locale del gioco li usa per aggiornare torcia/laser con l'arma nascosta durante lo zoom. Preview di altri attori e UI non appartenenti alla gerarchia SASWeapon mantengono la proiezione originale.

## Mouse

PlayerController.PlayerTick invoca PlayerInput tramite UnrealScript. L'hook agisce su `UObject::ProcessInternal`, RVA `0x0018BB90`, con trampoline di cinque byte. L'override si applica solo quando FFrame.Node a `+4` ha il nome PlayerInput, confrontato con il FName globale a RVA `0x009F3B0C`, e l'oggetto appartiene alla gerarchia PlayerInput. Le altre funzioni script passano direttamente all'interprete originale. Il layout dell'oggetto input e da 0x80 byte.

Il layout dei parametri filtro e verificato sul codice nativo locale: MouseSmoothingMode `+0x2C`, MouseSmoothingStrength `+0x30`, MouseAccelThreshold `+0x3C`. La sensibilita nativa a `+0x34` e esclusa: il codice di produzione non la legge, non la scrive e non la include nello snapshot RAII.

Il codice originale AccelerateMouse applica una trasformazione non lineare soltanto con soglia positiva; soglia zero la esclude. MouseSmoothingMode zero evita il filtro SmoothMouse. Il plugin impone i valori zero solo durante l'evento originale, quindi ripristina i tre parametri dei filtri. Non modifica User.ini, impostazioni Windows, inversione Y o fattore di sensibilita legato al FOV.

La sensibilita viene sempre letta ed elaborata dal gioco. Un cambio dal menu rimane valido anche se avviene durante la chiamata originale: il distruttore non ripristina quel campo. Le fixture verificano questa regressione oltre ai cambi effettuati tra due frame.

## Configurazione e verifiche

`[Main] Enabled` e controllato in Init prima della verifica dell'eseguibile, della migrazione INI e della preparazione di qualsiasi hook. Con zero si scrive solo la diagnostica nel log e si termina: nessuna patch camera, DrawActor, ProcessInternal o Direct3D, nessuna elaborazione dei tasti o scritta OSD. Non e un toggle a caldo: e richiesto un riavvio. Il loader ASI e gli altri plugin non vengono disabilitati.

Il valore mancante equivale a 1 per compatibilita. LoadSettings aggiunge la voce nei vecchi INI, anche con schema 3 gia aggiornato, mantenendo le opzioni. Non occorre cambiare ConfigVersion. La prova del loader avvia il vero ASI in processi privati con opzione assente, zero e uno, verificando il log e l'hash invariato dell'INI disabilitato.

ConfigVersion=3 mantiene i FOV e i quattro binding configurati, rimuove le tre vecchie opzioni di sensibilita e i due relativi tasti. Non esistono piu stato, passo, clamp, salvataggio o OSD della sensibilita. Le due coppie di tasti rimaste controllano soltanto i FOV. Il plugin non installa hook di tastiera di sistema e non cancella i binding nativi.

EndScene legge le dimensioni del back buffer con GetBackBuffer/GetDesc, rilasciando subito la superficie. Ogni variazione valida di larghezza o altezza, anche a rapporto invariato, arma una notifica. La prima misura inizializza il riferimento. Non e necessario agganciare Reset o conservare risorse D3D.

La notifica viene consumata da CameraFov solo con dimensioni corrispondenti all'ultimo back buffer e FOV valido: aggiorna il valore effettivo, seleziona l'OSD camera e avvia il timer. Le scene secondarie continuano a ricevere la correzione ma non sovrascrivono il FOV mostrato e non consumano la notifica. Un menu senza camera lascia la notifica in attesa del ritorno in gioco. Frame e dimensioni invariati non prolungano il timer; DurationMs=0 continua a nascondere l'OSD. Nessun valore BaseFOV o dato persistente viene modificato da una variazione di risoluzione.

L'OSD D3D9 continua a usare DrawPrimitiveUP, un font 5x7 incorporato ora esteso alle lettere maiuscole e un blocco di stato D3DSBT_ALL. Non conserva risorse D3D tra frame. Sono provati entrambi i tipi di scritta e i Reset a risoluzioni diverse.

Le fixture sintetiche eseguono gli hook reali a cinque e otto byte, verificando ABI, FOV trasmesso, ripristino e filtri mouse temporaneamente disattivati senza alterare la sensibilita. Verificano anche che una funzione script diversa su un oggetto PlayerInput non riceva l'override. Non caricano l'EXE del gioco in esecuzione e non sostituiscono la prova manuale delle nuove funzioni. Non vengono distribuiti sorgenti estratti o file binari del gioco.

Gli hook sono installati da InitializeASI; DllMain resta minimale. Se un'installazione parziale fallisce, gli entry point vengono ripristinati. Non e supportato lo scaricamento a caldo; si aggiornano i file a gioco chiuso.
