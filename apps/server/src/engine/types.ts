export type SimulationNodeBase<T extends string, D> = {
  id: string;
  type: T;
  data: D;
};

export type StartNode = SimulationNodeBase<'start', {
  Q0: number;
  c0: number[];
  label?: string;
}>;

export type MachineNode = SimulationNodeBase<'machine', {
  throughput_max: number;
  loss_factor: number;
  buffer_capacity?: number;
  M: number[][];
  split: number[];
  min_Number_input?: number;
  max_Number_input?: number;
  min_Number_output?: number;
  max_Number_output?: number;
  label?: string;
}>;

export type SensorNode = SimulationNodeBase<'sensor', {
  label?: string;
}>;

export type EndNode = SimulationNodeBase<'end', {
  label?: string;
}>;

export type SimulationNode = StartNode | MachineNode | SensorNode | EndNode;

export type SimulationEdge = {
  id: string;
  source: string;
  target: string;
  sourceHandle?: string;
  targetHandle?: string;
};

export type SimulationGraph = {
  nodes: SimulationNode[];
  edges: SimulationEdge[];
};

export type SimulationInput = {
  graph: SimulationGraph;
  families: string[];
  nSteps: number;
};

export type EdgeState = {
  flow: number;
  composition: number[];
};

export type TimePoint = {
  t: number;
  Q: number;
  c: number[];
};

export type SimulationOutput = {
  timeSeries: Record<string, { series: TimePoint[]; last?: TimePoint }>;
  warnings: Array<{ node: string; type: string; message: string }>;
};
