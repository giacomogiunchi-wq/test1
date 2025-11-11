import React, { useCallback } from 'react';
import ReactFlow, {
  addEdge,
  Background,
  BackgroundVariant,
  Connection,
  Controls,
  MiniMap,
  Node,
  OnConnect,
  ReactFlowProvider,
  useEdgesState,
  useNodesState
} from 'react-flow-renderer';
import { useGraphStore, type NodeData } from '../state/graphStore';
import { MeterCard } from './MeterCard';

const proOptions = { account: 'paid-pro', hideAttribution: true };

function EditorInner() {
  const {
    nodes,
    edges,
    setNodes,
    setEdges,
    selectNode,
    recordSnapshot,
    results,
    warnings,
    removeNode,
    removeEdge
  } = useGraphStore((state) => ({
    nodes: state.nodes,
    edges: state.edges,
    setNodes: state.setNodes,
    setEdges: state.setEdges,
    selectNode: state.selectNode,
    recordSnapshot: state.recordSnapshot,
    results: state.results,
    warnings: state.warnings,
    removeNode: state.removeNode,
    removeEdge: state.removeEdge
  }));

  const [rfNodes, setRfNodes, onNodesChange] = useNodesState<Node<NodeData>[]>(nodes);
  const [rfEdges, setRfEdges, onEdgesChange] = useEdgesState(edges);

  React.useEffect(() => {
    setRfNodes(nodes);
  }, [nodes, setRfNodes]);

  React.useEffect(() => {
    setRfEdges(edges);
  }, [edges, setRfEdges]);

  const onConnect: OnConnect = useCallback(
    (connection: Connection) => {
      recordSnapshot();
      const edgeId = `${connection.source}-${connection.sourceHandle}_${connection.target}-${connection.targetHandle}`;
      setEdges(addEdge({ ...connection, id: edgeId }, edges));
    },
    [edges, recordSnapshot, setEdges]
  );

  const onNodesChangeWithStore = useCallback(
    (changes: Parameters<typeof onNodesChange>[0]) => {
      setRfNodes((nds) => {
        const next = onNodesChange(changes, nds);
        setNodes(next);
        return next;
      });
    },
    [onNodesChange, setNodes, setRfNodes]
  );

  const onEdgesChangeWithStore = useCallback(
    (changes: Parameters<typeof onEdgesChange>[0]) => {
      setRfEdges((eds) => {
        const next = onEdgesChange(changes, eds);
        setEdges(next);
        return next;
      });
    },
    [onEdgesChange, setEdges, setRfEdges]
  );

  const onNodeClick = useCallback<NonNullable<React.ComponentProps<typeof ReactFlow>['onNodeClick']>>(
    (_, node) => selectNode(node.id),
    [selectNode]
  );

  const meterNodes = nodes.filter((n) => n.type === 'sensor' || n.type === 'end');

  return (
    <div className="h-full w-full">
      <ReactFlow
        nodes={rfNodes}
        edges={rfEdges}
        onNodesChange={onNodesChangeWithStore}
        onEdgesChange={onEdgesChangeWithStore}
        onNodesDelete={(deleted) => deleted.forEach((node) => removeNode(node.id))}
        onEdgesDelete={(deleted) => deleted.forEach((edge) => removeEdge(edge.id))}
        onConnect={onConnect}
        onNodeClick={onNodeClick}
        onPaneClick={() => selectNode(undefined)}
        nodesDraggable
        nodesConnectable
        fitView
        proOptions={proOptions}
      >
        <MiniMap pannable zoomable />
        <Controls />
        <Background variant={BackgroundVariant.Dots} gap={16} size={1} />
      </ReactFlow>
      <div className="pointer-events-none absolute bottom-4 right-4 flex max-h-[50%] w-72 flex-col gap-2 overflow-auto">
        {meterNodes.map((node) => {
          const measure = results[node.id]?.last;
          if (!measure) return null;
          return (
            <div key={node.id} className="pointer-events-auto">
              <MeterCard title={`${node.data.label ?? node.id}`} flow={measure.Q} composition={measure.c} />
            </div>
          );
        })}
      </div>
      {warnings.length > 0 && (
        <div className="pointer-events-none absolute left-4 top-4 max-w-sm space-y-2">
          {warnings.map((warning, idx) => (
            <div
              key={`${warning.node}-${idx}`}
              className="pointer-events-auto rounded-md border border-amber-400 bg-amber-100/80 px-3 py-2 text-xs text-amber-900 dark:border-amber-500 dark:bg-amber-500/20 dark:text-amber-200"
            >
              <div className="font-semibold">Nodo {warning.node}</div>
              <div>{warning.message}</div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

export function Editor() {
  return (
    <div className="flex-1 overflow-hidden">
      <ReactFlowProvider>
        <EditorInner />
      </ReactFlowProvider>
    </div>
  );
}
