import { create } from 'zustand';
import Papa from 'papaparse';
import type { NodeData } from './graphStore';

export type LibraryItem = {
  id: string;
  label: string;
  type: NodeData['type'];
  template: Partial<NodeData> & { type: NodeData['type'] };
};

type LibraryState = {
  items: LibraryItem[];
  loadInitialLibrary: () => Promise<void>;
  importCsv: (file: File) => Promise<void>;
  addItems: (items: LibraryItem[]) => void;
};

async function fetchJsonLibrary(): Promise<LibraryItem[]> {
  const res = await fetch('/api/library');
  if (!res.ok) throw new Error('Impossibile caricare libreria');
  const data = await res.json();
  const fromMachines: LibraryItem[] = (data.machines ?? []).map((machine: any) => ({
    id: machine.id,
    label: machine.label,
    type: 'machine',
    template: {
      type: 'machine',
      label: machine.label,
      throughputMax: machine.throughput_max ?? 0,
      lossFactor: machine.loss_factor ?? 0,
      matrix: machine.M ?? [],
      split: machine.split_defaults ?? [1],
      bufferCapacity: machine.buffer_capacity ?? 0,
      minInputs: machine.min_Number_input ?? 1,
      maxInputs: machine.max_Number_input ?? 1,
      minOutputs: machine.min_Number_output ?? 1,
      maxOutputs: machine.max_Number_output ?? 1
    }
  }));
  const fromSensors: LibraryItem[] = (data.sensors ?? []).map((sensor: any) => ({
    id: sensor.id,
    label: sensor.label,
    type: 'sensor',
    template: {
      type: 'sensor',
      label: sensor.label
    }
  }));
  const baseItems: LibraryItem[] = [
    {
      id: 'start-default',
      label: 'Start',
      type: 'start',
      template: {
        type: 'start',
        label: 'Start',
        portata: 1000,
        composizione: [1]
      }
    },
    {
      id: 'end-default',
      label: 'End',
      type: 'end',
      template: {
        type: 'end',
        label: 'End'
      }
    }
  ];
  return [...baseItems, ...fromMachines, ...fromSensors];
}

export const useLibraryStore = create<LibraryState>((set, get) => ({
  items: [],
  loadInitialLibrary: async () => {
    try {
      const items = await fetchJsonLibrary();
      set({ items });
    } catch (error) {
      console.error('Errore nel caricamento della libreria', error);
    }
  },
  importCsv: async (file: File) => {
    const text = await file.text();
    const parsed = Papa.parse(text, { header: true, dynamicTyping: true });
    const newItems: LibraryItem[] = [];
    for (const row of parsed.data as any[]) {
      if (!row || !row.type) continue;
      if (row.type === 'machine') {
        newItems.push({
          id: row.id,
          label: row.label ?? row.id,
          type: 'machine',
          template: {
            type: 'machine',
            label: row.label ?? row.id,
            throughputMax: Number(row.throughput_max ?? 0),
            lossFactor: Number(row.loss_factor ?? 0),
            matrix: row.M ? JSON.parse(row.M) : [],
            split: row.split_defaults ? JSON.parse(row.split_defaults) : [1],
            bufferCapacity: Number(row.buffer_capacity ?? 0),
            minInputs: Number(row.min_Number_input ?? 1),
            maxInputs: Number(row.max_Number_input ?? 1),
            minOutputs: Number(row.min_Number_output ?? 1),
            maxOutputs: Number(row.max_Number_output ?? 1)
          }
        });
      }
      if (row.type === 'sensor') {
        newItems.push({
          id: row.id,
          label: row.label ?? row.id,
          type: 'sensor',
          template: {
            type: 'sensor',
            label: row.label ?? row.id
          }
        });
      }
    }
    get().addItems(newItems);
  },
  addItems: (items) =>
    set((state) => {
      const map = new Map(state.items.map((item) => [item.id, item] as const));
      for (const item of items) {
        map.set(item.id, item);
      }
      return { items: Array.from(map.values()) };
    })
}));
