import { useRef } from 'react';
import { Upload, Download, PlayCircle, Undo2, Redo2, PlusCircle, SunMoon } from 'lucide-react';
import { useGraphStore } from '../state/graphStore';
import { useLibraryStore } from '../state/libraryStore';
import { useTheme } from './ThemeProvider';

export function TopBar() {
  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const {
    nodes,
    edges,
    nSteps,
    families,
    reset,
    undo,
    redo,
    recordSnapshot,
    setResults,
    setWarnings,
    setNSteps
  } = useGraphStore((state) => ({
    nodes: state.nodes,
    edges: state.edges,
    nSteps: state.nSteps,
    families: state.families,
    reset: state.reset,
    undo: state.undo,
    redo: state.redo,
    recordSnapshot: state.recordSnapshot,
    setResults: state.setResults,
    setWarnings: state.setWarnings,
    setNSteps: state.setNSteps
  }));
  const { toggle } = useTheme();
  const { importCsv } = useLibraryStore();

  const onExport = () => {
    const payload = {
      nodes,
      edges,
      nSteps,
      families
    };
    const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'progetto-sorting.json';
    a.click();
    URL.revokeObjectURL(url);
  };

  const onImportProject = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;
    const text = await file.text();
    const json = JSON.parse(text);
    recordSnapshot();
    useGraphStore.setState({
      nodes: json.nodes ?? [],
      edges: json.edges ?? [],
      nSteps: json.nSteps ?? 10,
      families: json.families ?? families
    });
    setResults({});
    setWarnings([]);
    if (Array.isArray(json.libraryItems)) {
      useLibraryStore.getState().addItems(json.libraryItems);
    }
    event.target.value = '';
  };

  const onImportLibrary = () => {
    fileInputRef.current?.click();
  };

  const onImportCsv = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;
    await importCsv(file);
    event.target.value = '';
  };

  const runSimulation = async () => {
    setWarnings([]);
    setResults({});
    const graphPayload = {
      nodes: nodes.map((node) => {
        const nodeData: any = node.data ?? {};
        const type = nodeData.type ?? node.type;
        const base = {
          id: node.id,
          type,
          data: {} as any
        };
        switch (type) {
          case 'start':
            {
              const comp = [...(nodeData.composizione ?? nodeData.c0 ?? [])];
              while (comp.length < families.length) {
                comp.push(0);
              }
            base.data = {
              Q0: nodeData.portata ?? nodeData.Q0 ?? 0,
              c0: comp,
              label: nodeData.label ?? node.id
            };
            }
            break;
          case 'machine':
            base.data = {
              throughput_max: nodeData.throughputMax ?? nodeData.throughput_max ?? 0,
              loss_factor: nodeData.lossFactor ?? nodeData.loss_factor ?? 0,
              buffer_capacity: nodeData.bufferCapacity ?? nodeData.buffer_capacity ?? 0,
              M: nodeData.matrix ?? nodeData.M ?? [],
              split: nodeData.split ?? [],
              min_Number_input: nodeData.minInputs ?? nodeData.min_Number_input,
              max_Number_input: nodeData.maxInputs ?? nodeData.max_Number_input,
              min_Number_output: nodeData.minOutputs ?? nodeData.min_Number_output,
              max_Number_output: nodeData.maxOutputs ?? nodeData.max_Number_output,
              label: nodeData.label ?? node.id
            };
            break;
          case 'sensor':
            base.data = { label: nodeData.label ?? node.id };
            break;
          case 'end':
            base.data = { label: nodeData.label ?? node.id };
            break;
          default:
            base.data = nodeData;
        }
        return base;
      }),
      edges: edges.map((edge) => ({
        id: edge.id,
        source: edge.source,
        target: edge.target,
        sourceHandle: edge.sourceHandle,
        targetHandle: edge.targetHandle
      }))
    };

    try {
      const response = await fetch('/api/simulate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          graph: graphPayload,
          families: families.map((f) => f.label),
          nSteps
        })
      });
      if (!response.ok) {
        console.error('Simulazione fallita');
        return;
      }
      const data = await response.json();
      setResults(data.timeSeries ?? {});
      setWarnings(data.warnings ?? []);
    } catch (error) {
      console.error('Errore di rete nella simulazione', error);
    }
  };

  return (
    <header className="flex items-center justify-between border-b border-slate-200 bg-white px-4 py-2 dark:border-slate-700 dark:bg-slate-800">
      <div className="flex items-center gap-2">
        <button
          type="button"
          className="flex items-center gap-2 rounded-md bg-primary px-3 py-1 text-sm font-medium text-white shadow hover:bg-indigo-600"
          onClick={() => {
            if (window.confirm('Vuoi creare un nuovo progetto?')) {
              reset();
            }
          }}
        >
          <PlusCircle className="h-4 w-4" /> Nuovo
        </button>
        <button
          type="button"
          className="flex items-center gap-2 rounded-md border border-slate-200 px-3 py-1 text-sm font-medium dark:border-slate-700"
          onClick={onExport}
        >
          <Download className="h-4 w-4" /> Esporta
        </button>
        <label className="flex cursor-pointer items-center gap-2 rounded-md border border-slate-200 px-3 py-1 text-sm font-medium dark:border-slate-700">
          <Upload className="h-4 w-4" /> Importa progetto
          <input type="file" accept="application/json" className="hidden" onChange={onImportProject} />
        </label>
        <button
          type="button"
          className="flex items-center gap-2 rounded-md border border-slate-200 px-3 py-1 text-sm font-medium dark:border-slate-700"
          onClick={onImportLibrary}
        >
          <Upload className="h-4 w-4" /> Importa CSV
        </button>
        <input ref={fileInputRef} type="file" className="hidden" accept="text/csv" onChange={onImportCsv} />
      </div>
      <div className="flex items-center gap-2">
        <label className="text-sm font-medium">
          Step
          <input
            type="number"
            value={nSteps}
            min={1}
            onChange={(event) => setNSteps(Number(event.target.value) || 1)}
            className="ml-2 w-20 rounded-md border border-slate-300 bg-transparent px-2 py-1 text-sm dark:border-slate-600"
          />
        </label>
        <button
          type="button"
          className="flex items-center gap-2 rounded-md bg-emerald-500 px-3 py-1 text-sm font-medium text-white shadow hover:bg-emerald-600"
          onClick={runSimulation}
        >
          <PlayCircle className="h-4 w-4" /> Esegui simulazione
        </button>
        <button
          type="button"
          className="flex items-center gap-2 rounded-md border border-slate-200 px-3 py-1 text-sm font-medium dark:border-slate-700"
          onClick={undo}
        >
          <Undo2 className="h-4 w-4" /> Annulla
        </button>
        <button
          type="button"
          className="flex items-center gap-2 rounded-md border border-slate-200 px-3 py-1 text-sm font-medium dark:border-slate-700"
          onClick={redo}
        >
          <Redo2 className="h-4 w-4" /> Ripeti
        </button>
        <button
          type="button"
          className="flex items-center gap-2 rounded-md border border-slate-200 px-3 py-1 text-sm font-medium dark:border-slate-700"
          onClick={toggle}
        >
          <SunMoon className="h-4 w-4" /> Tema
        </button>
      </div>
    </header>
  );
}
