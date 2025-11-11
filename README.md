# Sorting Sim MVP

Applicazione web per la progettazione e simulazione di impianti di sorting monodimensionali tramite node editor.

## Requisiti

- Node.js >= 18
- pnpm (consigliato) oppure npm compatibile con workspaces

## Installazione

```bash
pnpm install
```

## Comandi principali

| Comando | Descrizione |
| --- | --- |
| `pnpm dev` | Avvia frontend e backend in parallelo (React + Express). |
| `pnpm build` | Compila entrambi i pacchetti. |
| `pnpm start` | Avvia solo il server Express compilato (porta 4000). |
| `pnpm lint` | Esegue `tsc --noEmit` su web e server. |
| `pnpm test` | Esegue i test unitari (Vitest) per il motore. |

Dal pacchetto `apps/web` è possibile usare `pnpm --filter web dev` per avviare solo il frontend Vite sulla porta 5173.

## Struttura del monorepo

```
sorting-sim-mvp/
├── apps/
│   ├── web/      # Frontend React + Vite + Tailwind + React Flow
│   └── server/   # Backend Express + motore di simulazione DEM semplificato
├── package.json
├── pnpm-workspace.yaml
└── README.md
```

### Frontend (`apps/web`)

- Editor a nodi basato su React Flow per Start, Machine, Sensor, End.
- Libreria blocchi caricata da `/api/library` (unione di `public/block.json` + elementi utente).
- Inspector laterale per modificare proprietà (portata, composizione, matrici `M`, split, ecc.).
- Barre strumenti per Import/Export progetto JSON, import libreria CSV, undo/redo, esecuzione simulazione.
- Stato centralizzato con Zustand.
- Risultati (portata e composizione in kg/h) mostrati direttamente su Inspector e card per Sensor/End.

### Backend (`apps/server`)

- API REST Express + Socket.IO (notifica ready) sulla porta 4000.
- Motore di simulazione TypeScript puro (`src/engine`) con topological sort, gestione capacità, matrici di efficienza e split.
- Persistenza file JSON locale (`/data/projects` e `/data/library.json`).

## API

### `GET /api/library`
Restituisce la libreria unificata:

```json
{
  "machines": [
    {
      "id": "ballistic_01",
      "label": "Separatore Balistico",
      "min_Number_input": 1,
      "max_Number_input": 2,
      "min_Number_output": 2,
      "max_Number_output": 4,
      "throughput_max": 10000,
      "loss_factor": 0.02,
      "buffer_capacity": 0,
      "M": [[0.9,0.1,0],[0.1,0.8,0.1],[0,0.1,0.9]],
      "split_defaults": [0.7,0.3],
      "unit": "kg/h"
    }
  ],
  "sensors": [
    { "id": "scale_inline", "label": "Bilancia Inlinea", "unit": "kg/h" }
  ]
}
```

### `POST /api/projects`
Salva un progetto JSON. Corpo:

```json
{
  "id": "opzionale",
  "graph": { "nodes": [...], "edges": [...] },
  "families": ["Carta", "Plastica", "Metalli"],
  "nSteps": 10,
  "libraryItems": []
}
```

Risponde con `{ "id": "proj_..." }`.

### `GET /api/projects/:id`
Restituisce il payload precedentemente salvato.

### `POST /api/simulate`
Esegue la simulazione sincrona.

Richiesta:

```json
{
  "families": ["Carta", "Plastica", "Metalli"],
  "nSteps": 5,
  "graph": {
    "nodes": [
      { "id": "s1", "type": "start", "data": { "Q0": 3000, "c0": [0.33,0.33,0.34] } },
      { "id": "m1", "type": "machine", "data": { "throughput_max": 2500, "loss_factor": 0.05, "M": [[0.95,0.05,0],[0.05,0.9,0.05],[0,0.05,0.95]], "split": [0.75,0.25] } },
      { "id": "e1", "type": "end", "data": {} }
    ],
    "edges": [
      { "id": "e_s1_m1", "source": "s1", "target": "m1" },
      { "id": "e_m1_e1", "source": "m1", "target": "e1" }
    ]
  }
}
```

Risposta:

```json
{
  "timeSeries": {
    "e1": {
      "series": [
        { "t": 0, "Q": 0, "c": [0.33,0.33,0.34] },
        { "t": 1, "Q": 2375, "c": [0.34,0.32,0.34] }
      ],
      "last": { "t": 4, "Q": 2375, "c": [0.34,0.32,0.34] }
    }
  },
  "warnings": [
    { "node": "m1", "type": "capacity", "message": "Ingresso 3000.0 kg/h oltre throughput 2500.0 kg/h" }
  ]
}
```

## Formato progetto JSON

L'esportazione dal frontend produce:

```json
{
  "nodes": [...],
  "edges": [...],
  "families": [
    { "id": "carta", "label": "Carta" },
    { "id": "plastica", "label": "Plastica" },
    { "id": "metalli", "label": "Metalli" }
  ],
  "nSteps": 10,
  "libraryItems": [ ... facoltativo ... ]
}
```

## Import libreria CSV

Colonne supportate:

- `type` (`machine` | `sensor`)
- `id`, `label`
- `min_Number_input`, `max_Number_input`, `min_Number_output`, `max_Number_output`
- `throughput_max`, `loss_factor`, `buffer_capacity`
- `M` (stringa JSON), `split_defaults` (stringa JSON)

I record vengono uniti per `id` con la libreria esistente e salvati lato client.

## Matrici e split

- Le matrici `M` sono normalizzate riga per riga (fallback identità su righe vuote).
- Gli split di uscita devono sommare 100%: lo slider nell'Inspector normalizza automaticamente i valori inseriti.
- Tutte le portate sono espresse in **kg/h**.

## Test

`pnpm test` esegue i test unitari per `normalizeVector`, `multiplyMatrixVector`, `applyCapacity` e distribuzione split.

## Note

- Undo/Redo copre aggiunta/rimozione nodi/archi e modifiche Inspector.
- Il motore blocca grafi con cicli (topological sort).
- I warning di saturazione vengono mostrati come badge sull'editor.
