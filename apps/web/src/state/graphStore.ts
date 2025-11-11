import { create } from 'zustand';
import { devtools } from 'zustand/middleware';
import type { Edge, Node } from 'react-flow-renderer';
import { nanoid } from '../lib/nanoid';

export type MaterialFamily = {
  id: string;
  label: string;
};

export type StartNodeData = {
  type: 'start';
  label?: string;
  portata: number;
  composizione: number[];
};

export type MachineNodeData = {
  type: 'machine';
  label?: string;
  throughputMax: number;
  lossFactor: number;
  matrix: number[][];
  split: number[];
  bufferCapacity: number;
  minInputs: number;
  maxInputs: number;
  minOutputs: number;
  maxOutputs: number;
};

export type SensorNodeData = {
  type: 'sensor';
  label?: string;
};

export type EndNodeData = {
  type: 'end';
  label?: string;
};

export type NodeData = StartNodeData | MachineNodeData | SensorNodeData | EndNodeData;

export type SimulationResult = {
  [nodeId: string]: {
    last?: {
      t: number;
      Q: number;
      c: number[];
    };
    series: Array<{ t: number; Q: number; c: number[] }>;
  };
};

export type SimulationWarning = {
  node: string;
  type: string;
  message: string;
};

type Snapshot = {
  nodes: Node<NodeData>[];
  edges: Edge[];
};

export interface GraphState {
  nodes: Node<NodeData>[];
  edges: Edge[];
  families: MaterialFamily[];
  selectedNodeId?: string;
  nSteps: number;
  results: SimulationResult;
  warnings: SimulationWarning[];
  history: Snapshot[];
  future: Snapshot[];
  setNodes: (nodes: Node<NodeData>[]) => void;
  setEdges: (edges: Edge[]) => void;
  selectNode: (id?: string) => void;
  updateNodeData: (id: string, data: Partial<NodeData>) => void;
  setFamilies: (families: MaterialFamily[]) => void;
  setNSteps: (n: number) => void;
  setResults: (results: SimulationResult) => void;
  setWarnings: (warnings: SimulationWarning[]) => void;
  addNodeFromTemplate: (template: Partial<NodeData> & { type: NodeData['type']; label: string }) => void;
  removeNode: (id: string) => void;
  removeEdge: (id: string) => void;
  recordSnapshot: () => void;
  undo: () => void;
  redo: () => void;
  reset: () => void;
}

const defaultFamilies: MaterialFamily[] = [
  { id: 'carta', label: 'Carta' },
  { id: 'plastica', label: 'Plastica' },
  { id: 'metalli', label: 'Metalli' }
];

function cloneSnapshot(state: Snapshot): Snapshot {
  return {
    nodes: state.nodes.map((node) => ({ ...node, data: { ...node.data } })),
    edges: state.edges.map((edge) => ({ ...edge }))
  };
}

export const useGraphStore = create<GraphState>()(
  devtools((set, get) => ({
    nodes: [],
    edges: [],
    families: defaultFamilies,
    selectedNodeId: undefined,
    nSteps: 10,
    results: {},
    warnings: [],
    history: [],
    future: [],
    setNodes: (nodes) => set({ nodes }),
    setEdges: (edges) => set({ edges }),
    selectNode: (id) => set({ selectedNodeId: id }),
    updateNodeData: (id, data) => {
      get().recordSnapshot();
      set((state) => ({
        nodes: state.nodes.map((node) =>
          node.id === id
            ? {
                ...node,
                data: { ...node.data, ...data } as NodeData
              }
            : node
        )
      }));
    },
    setFamilies: (families) => set({ families }),
    setNSteps: (nSteps) => set({ nSteps }),
    setResults: (results) => set({ results }),
    setWarnings: (warnings) => set({ warnings }),
    addNodeFromTemplate: (template) => {
      const id = nanoid(template.type);
      const count = get().nodes.length;
      const position = { x: 100 + (count % 4) * 120, y: 100 + Math.floor(count / 4) * 120 };
      const baseNode: Node<NodeData> = {
        id,
        type: template.type,
        position,
        data: {
          ...template,
          type: template.type
        } as NodeData
      };
      get().recordSnapshot();
      set((state) => ({ nodes: [...state.nodes, baseNode] }));
    },
    removeNode: (id) => {
      get().recordSnapshot();
      set((state) => ({
        nodes: state.nodes.filter((node) => node.id !== id),
        edges: state.edges.filter((edge) => edge.source !== id && edge.target !== id),
        selectedNodeId: state.selectedNodeId === id ? undefined : state.selectedNodeId
      }));
    },
    removeEdge: (id) => {
      get().recordSnapshot();
      set((state) => ({ edges: state.edges.filter((edge) => edge.id !== id) }));
    },
    recordSnapshot: () => {
      const snapshot = cloneSnapshot({ nodes: get().nodes, edges: get().edges });
      set((state) => ({ history: [...state.history, snapshot], future: [] }));
    },
    undo: () => {
      set((state) => {
        if (state.history.length === 0) return state;
        const history = [...state.history];
        const previous = history.pop()!;
        const currentSnapshot: Snapshot = cloneSnapshot({ nodes: state.nodes, edges: state.edges });
        return {
          ...state,
          nodes: previous.nodes,
          edges: previous.edges,
          history,
          future: [...state.future, currentSnapshot]
        };
      });
    },
    redo: () => {
      set((state) => {
        if (state.future.length === 0) return state;
        const future = [...state.future];
        const next = future.pop()!;
        const currentSnapshot: Snapshot = cloneSnapshot({ nodes: state.nodes, edges: state.edges });
        return {
          ...state,
          nodes: next.nodes,
          edges: next.edges,
          history: [...state.history, currentSnapshot],
          future
        };
      });
    },
    reset: () =>
      set({
        nodes: [],
        edges: [],
        results: {},
        warnings: [],
        selectedNodeId: undefined,
        history: [],
        future: []
      })
  }))
);
