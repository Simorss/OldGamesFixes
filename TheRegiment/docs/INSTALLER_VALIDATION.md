# Verifica degli script di installazione

## Release 1.0

Il 24 ottobre 2026 sono passati i test di prima installazione e disinstallazione con i quattro file runtime, compresa la licenza ThirteenAG; verificati rifiuto duplicati, conservazione del loader preesistente e conflitto con loader diversi. Passato l'aggiornamento dalla 0.2.4 con INI e loader invariati, backup .bak, manifest aggiornato, aggiornamento ripetuto e rimozione corretta. Le fixture non avviano sas.exe e le copie private sono state rimosse. Nessuna modifica al gioco reale. La suite locale completa passa 192 controlli; la modalita CI senza test grafici passa 146 controlli e segnala esplicitamente lo skip del rendering D3D9.

## Versione 0.2.4-preview

Il 24 ottobre 2026 e passato il test di aggiornamento dalla 0.2.3: ASI aggiornato, hash di INI e loader invariati, backup .bak verificati, manifest aggiornato, seconda esecuzione senza modifiche e successiva disinstallazione. La fixture non esegue sas.exe e la copia privata dell'eseguibile e stata rimossa. Le prove runtime coprono notifica OSD della risoluzione e dimensioni effettive dopo Reset D3D9. Il gioco reale non e stato modificato o avviato.

## Versione 0.2.3-preview

Il 24 ottobre 2026 e passato il test di aggiornamento dalla 0.2.2: ASI sostituito, INI e loader conservati byte per byte, backup .bak verificati, manifest aggiornato, aggiornamento ripetuto e disinstallazione corretti. Le prove usano fixture private senza eseguire sas.exe; la copia temporanea dell'eseguibile e stata rimossa. Passata anche la prova del loader reale con Main.Enabled assente, zero e uno: lo stato disabilitato non modifica l'INI e non installa hook. Nessuna modifica dell'installazione reale.

## Versione 0.2.2-preview

Il 24 ottobre 2026 sono passati i test di aggiornamento dalla 0.2.1 alla 0.2.2: ASI sostituito, INI e loader invariati, backup non caricabili come ASI, manifest aggiornato, seconda esecuzione senza modifiche e successiva disinstallazione. Passati anche i test di prima installazione, rifiuto duplicati, conservazione del loader preesistente e rifiuto di loader diversi. Le fixture private non eseguono sas.exe; le copie temporanee dell'eseguibile sono state rimosse. L'installazione reale non e stata modificata.

## Versione 0.2.1-preview

Il 24 ottobre 2026 sono passati i test di aggiornamento sia dalla 0.2.0 sia dalla 0.1.0, preservando gli hash di INI e loader, creando backup non caricabili come ASI e aggiornando il manifest. Verificati anche aggiornamento ripetuto e successiva disinstallazione. Le copie private di sas.exe sono state rimosse; il gioco reale non e stato avviato o aggiornato dall'assistente.

## Versione 0.2.0-preview

Il 24 ottobre 2026 sono stati ripetuti i test di prima installazione e rimozione, e aggiunti i test di aggiornamento dalla 0.1.0. Risultati:

- PASS: installazione nuova, rifiuto duplicati, disinstallazione, conservazione di loader preesistenti e rifiuto di loader diversi.
- PASS: aggiornamento dell'ASI 0.1.0 verificato alla 0.2.0, con hash dell'INI e del loader invariati.
- PASS: backup di ASI e INI con estensioni .bak, senza copie ASI caricabili dal loader.
- PASS: aggiornamento del manifest, seconda esecuzione senza modifiche e successiva disinstallazione.

Le prove usano copie temporanee private sotto build/, senza eseguire sas.exe. Le copie dell'eseguibile sono state rimosse. L'installazione reale non e stata aggiornata.

## Verifica precedente 0.1.0

Eseguita il 24 ottobre 2026 su una fixture privata sotto la cartella di lavoro. L'eseguibile originale è stato soltanto copiato per la verifica dell'hash, mai avviato, e la copia temporanea è stata rimossa. Nessun file della cartella reale del gioco è stato modificato.

| Caso | Risultato |
| --- | --- |
| Installazione da zero dei tre file runtime e del manifest | PASS |
| Rifiuto di una seconda installazione senza sovrascritture | PASS |
| Disinstallazione dei file aggiunti | PASS |
| Riutilizzo e conservazione di un loader identico preesistente | PASS |
| Rifiuto di un loader diverso, senza installazione parziale | PASS |

Test ripetibile: `tests/Test-Install.ps1 -GameDirectory <cartella del gioco>`. I test di gameplay rimangono a carico dell'utente; questo report non ne dichiara il superamento.
