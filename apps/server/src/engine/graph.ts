import type { EdgeState, SimulationEdge, SimulationNode } from './types';
import { padVector } from './math';

export type GraphMaps = {
  incoming: Map<string, SimulationEdge[]>;
  outgoing: Map<string, SimulationEdge[]>;
};

export function buildGraphMaps(nodes: SimulationNode[], edges: SimulationEdge[]): GraphMaps {
  const incoming = new Map<string, SimulationEdge[]>();
  const outgoing = new Map<string, SimulationEdge[]>();
  for (const edge of edges) {
    if (!incoming.has(edge.target)) incoming.set(edge.target, []);
    incoming.get(edge.target)!.push(edge);
    if (!outgoing.has(edge.source)) outgoing.set(edge.source, []);
    outgoing.get(edge.source)!.push(edge);
  }
  for (const node of nodes) {
    if (!incoming.has(node.id)) incoming.set(node.id, []);
    if (!outgoing.has(node.id)) outgoing.set(node.id, []);
  }
  return { incoming, outgoing };
}

export function topologicalSort(nodes: SimulationNode[], edges: SimulationEdge[]): SimulationNode[] {
  const nodeMap = new Map(nodes.map((node) => [node.id, node] as const));
  const inDegree = new Map<string, number>();
  for (const node of nodes) {
    inDegree.set(node.id, 0);
  }
  for (const edge of edges) {
    inDegree.set(edge.target, (inDegree.get(edge.target) ?? 0) + 1);
  }
  const queue: SimulationNode[] = [];
  for (const node of nodes) {
    if ((inDegree.get(node.id) ?? 0) === 0) queue.push(node);
  }
  const result: SimulationNode[] = [];
  while (queue.length > 0) {
    const node = queue.shift()!;
    result.push(node);
    for (const edge of edges.filter((e) => e.source === node.id)) {
      const targetId = edge.target;
      const newVal = (inDegree.get(targetId) ?? 0) - 1;
      inDegree.set(targetId, newVal);
      if (newVal === 0) {
        const targetNode = nodeMap.get(targetId);
        if (targetNode) queue.push(targetNode);
      }
    }
  }
  return result;
}

export function aggregateInputs(
  nodeId: string,
  incomingEdges: SimulationEdge[],
  edgeState: Map<string, EdgeState>,
  familiesCount: number
): { flow: number; composition: number[] } {
  if (incomingEdges.length === 0) {
    return { flow: 0, composition: Array.from({ length: familiesCount }, () => 1 / familiesCount) };
  }
  let totalFlow = 0;
  let weighted = Array.from({ length: familiesCount }, () => 0);
  for (const edge of incomingEdges) {
    const state = edgeState.get(edge.id);
    if (!state) continue;
    totalFlow += state.flow;
    const comp = padVector(state.composition, familiesCount);
    for (let i = 0; i < familiesCount; i++) {
      weighted[i] += comp[i] * state.flow;
    }
  }
  if (totalFlow <= 0) {
    return { flow: 0, composition: Array.from({ length: familiesCount }, () => 1 / familiesCount) };
  }
  const composition = weighted.map((value) => (value <= 0 ? 0 : value / totalFlow));
  return { flow: totalFlow, composition };
}
