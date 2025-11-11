import { aggregateInputs, buildGraphMaps, topologicalSort } from './graph';
import {
  applyCapacity,
  distributeFlow,
  ensureSquareMatrix,
  multiplyMatrixVector,
  normalizeVector,
  padVector
} from './math';
import type {
  EdgeState,
  MachineNode,
  SimulationInput,
  SimulationNode,
  SimulationOutput,
  TimePoint
} from './types';

export function simulate(input: SimulationInput): SimulationOutput {
  const { graph, families, nSteps } = input;
  if (families.length === 0) {
    throw new Error('Sono richieste almeno una famiglia di materiale');
  }
  if (nSteps <= 0) {
    throw new Error('nSteps deve essere maggiore di zero');
  }

  const familiesCount = families.length;
  const order = topologicalSort(graph.nodes, graph.edges);
  if (order.length !== graph.nodes.length) {
    throw new Error('Il grafo contiene cicli o nodi isolati');
  }
  const { incoming, outgoing } = buildGraphMaps(graph.nodes, graph.edges);
  const edgeStatePrev = new Map<string, EdgeState>();

  const timeSeries: SimulationOutput['timeSeries'] = {};
  const warnings: SimulationOutput['warnings'] = [];

  const record = (nodeId: string, point: TimePoint) => {
    if (!timeSeries[nodeId]) {
      timeSeries[nodeId] = { series: [], last: undefined };
    }
    timeSeries[nodeId].series.push(point);
    timeSeries[nodeId].last = point;
  };

  let prevEdgeState = edgeStatePrev;

  for (let step = 0; step < nSteps; step++) {
    const nextEdgeState = new Map<string, EdgeState>();

    for (const node of order) {
      const incomingEdges = incoming.get(node.id) ?? [];
      const outgoingEdges = outgoing.get(node.id) ?? [];
      const aggregated = aggregateInputs(node.id, incomingEdges, prevEdgeState, familiesCount);

      switch (node.type) {
        case 'start': {
          const composition = normalizeVector(padVector(node.data.c0 ?? [], familiesCount));
          const Q = node.data.Q0 ?? 0;
          distributeToEdges(outgoingEdges, nextEdgeState, Q, composition);
          break;
        }
        case 'machine': {
          const machine = node as MachineNode;
          const limited = applyCapacity(aggregated.flow, machine.data.throughput_max ?? 0, machine.data.loss_factor ?? 0);
          if (aggregated.flow > (machine.data.throughput_max ?? 0)) {
            warnings.push({
              node: machine.id,
              type: 'capacity',
              message: `Ingresso ${aggregated.flow.toFixed(1)} kg/h oltre throughput ${(machine.data.throughput_max ?? 0).toFixed(1)} kg/h`
            });
          }
          const matrix = ensureSquareMatrix(machine.data.M ?? [], familiesCount);
          const transformed = multiplyMatrixVector(matrix, aggregated.composition);
          const composition = normalizeVector(transformed);
          const flows = distributeFlow(limited, machine.data.split ?? [], outgoingEdges.length);
          outgoingEdges.forEach((edge, idx) => {
            nextEdgeState.set(edge.id, {
              flow: flows[idx] ?? 0,
              composition
            });
          });
          break;
        }
        case 'sensor': {
          record(node.id, { t: step, Q: aggregated.flow, c: aggregated.composition });
          distributeToEdges(outgoingEdges, nextEdgeState, aggregated.flow, aggregated.composition);
          break;
        }
        case 'end': {
          record(node.id, { t: step, Q: aggregated.flow, c: aggregated.composition });
          break;
        }
        default:
          break;
      }
    }

    prevEdgeState = nextEdgeState;
  }

  return { timeSeries, warnings };
}

function distributeToEdges(
  edges: { id: string }[],
  nextEdgeState: Map<string, EdgeState>,
  flow: number,
  composition: number[]
) {
  if (!Array.isArray(edges)) return;
  if (edges.length === 0) return;
  const flows = distributeFlow(flow, Array(edges.length).fill(1), edges.length);
  edges.forEach((edge: any, idx: number) => {
    nextEdgeState.set(edge.id, {
      flow: flows[idx] ?? 0,
      composition
    });
  });
}
